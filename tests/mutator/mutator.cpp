/**
 * mutator.cpp
 *
 * Clang LibTooling-based source-level mutator for static analysis test cases.
 *
 * Produces complexity layers L0–L5 for each input .cpp file:
 *
 *   L0  baseline          — original file, verbatim copy
 *   L1  inter-procedural  — split defect across a helper function call
 *   L2  aliasing          — route the defective pointer through an alias /
 *                           struct field before the defect fires
 *   L3  opaque control    — wrap defect in a branch whose condition is an
 *                           opaque runtime value (argc, getenv) that static
 *                           analyzers cannot resolve
 *   L4  loop wrapping     — place the defect body inside a for-loop so the
 *                           tool must reason about loop iterations
 *   L5  combined          — inter-procedural + aliasing + opaque branch
 *
 * Build:  see README_mutator.md
 * Usage:
 *   mutator [--levels 0,1,2,3,4,5] [--outdir ./mutated] \
 *           <source.cpp> [<source2.cpp> ...]  \
 *           -- -std=c++14
 *
 * The tool uses the Clang AST to locate the _bad() function in each
 * translation unit, extracts the body statements that constitute the
 * defect, then rewrites a copy of the file with the appropriate
 * transformation applied.
 *
 * Architecture overview
 * ─────────────────────
 *  ClangTool
 *    └─ MutatorAction (FrontendAction)
 *         └─ MutatorConsumer (ASTConsumer)
 *              └─ DefectVisitor (RecursiveASTVisitor)
 *                   detects _bad() function, records its body range
 *              └─ LevelEmitter
 *                   takes the recorded AST info and writes L1-L5 variants
 *                   using Rewriter (text-level) transformations
 */

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// ─────────────────────────────────────────────────────────────────────────────
// Command-line options
// ─────────────────────────────────────────────────────────────────────────────

static cl::OptionCategory MutatorCategory("mutator options");

static cl::opt<std::string> OutDir(
    "outdir",
    cl::desc("Output directory for mutated files (default: ./mutated)"),
    cl::init("./mutated"),
    cl::cat(MutatorCategory));

static cl::opt<std::string> LevelsOpt(
    "levels",
    cl::desc("Comma-separated list of levels to generate (default: 0,1,2,3,4,5)"),
    cl::init("0,1,2,3,4,5"),
    cl::cat(MutatorCategory));

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::set<int> parseLevels(const std::string &s) {
    std::set<int> result;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        try { result.insert(std::stoi(tok)); } catch (...) {}
    }
    return result;
}

// Read entire file into string
static std::string readFile(const std::string &path) {
    std::ifstream f(path);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

// Write string to file, creating directories as needed
static bool writeFile(const std::string &path, const std::string &content) {
    llvm::StringRef dir = llvm::sys::path::parent_path(path);
    if (!dir.empty()) {
        std::error_code ec = llvm::sys::fs::create_directories(dir);
        if (ec) { llvm::errs() << "Cannot create dir " << dir << ": " << ec.message() << "\n"; return false; }
    }
    std::ofstream f(path);
    if (!f) { llvm::errs() << "Cannot write " << path << "\n"; return false; }
    f << content;
    return true;
}

// Build output path:  <outdir>/L<level>/<basename>
static std::string buildOutPath(const std::string &outDir, int level,
                                const std::string &inputPath) {
    llvm::SmallString<256> p(outDir);
    llvm::sys::path::append(p, "L" + std::to_string(level));
    llvm::sys::path::append(p, llvm::sys::path::filename(inputPath));
    return std::string(p);
}

// ─────────────────────────────────────────────────────────────────────────────
// Information collected from the AST about the _bad() function
// ─────────────────────────────────────────────────────────────────────────────

struct BadFuncInfo {
    bool         found        = false;
    std::string  funcName;          // e.g. "double_free_bad"
    std::string  returnType;        // e.g. "void"
    std::string  paramList;         // e.g. "(int flag)"
    std::string  paramNames;        // e.g. "flag"  (for forwarding)
    std::string  bodyText;          // full original body including braces
    unsigned     bodyBeginOffset = 0;
    unsigned     bodyEndOffset   = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// AST visitor — finds the first function whose name ends in "_bad"
// ─────────────────────────────────────────────────────────────────────────────

class DefectVisitor : public RecursiveASTVisitor<DefectVisitor> {
public:
    ASTContext  &Ctx;
    BadFuncInfo &Info;

    DefectVisitor(ASTContext &ctx, BadFuncInfo &info)
        : Ctx(ctx), Info(info) {}

    bool VisitFunctionDecl(FunctionDecl *FD) {
        if (Info.found) return true;                   // already have one
        if (!FD->hasBody()) return true;               // declaration only
        if (FD->isImplicit()) return true;

        std::string name = FD->getNameAsString();
        if (name.size() < 4) return true;
        if (name.substr(name.size() - 4) != "_bad") return true;

        // Collect parameter text
        std::string paramList, paramNames;
        {
            llvm::raw_string_ostream pl(paramList), pn(paramNames);
            pl << "(";
            for (unsigned i = 0, n = FD->getNumParams(); i < n; ++i) {
                ParmVarDecl *pv = FD->getParamDecl(i);
                if (i) { pl << ", "; pn << ", "; }
                QualType qt = pv->getType();
                pl << qt.getAsString() << " " << pv->getNameAsString();
                pn << pv->getNameAsString();
            }
            pl << ")";
        }

        // Extract body text
        SourceManager &SM   = Ctx.getSourceManager();
        Stmt          *body = FD->getBody();
        SourceRange    sr   = body->getSourceRange();

        // Use getExpansionLoc to get past macro expansions
        SourceLocation begin = SM.getExpansionLoc(sr.getBegin());
        SourceLocation end   = SM.getExpansionLoc(sr.getEnd());

        // Clang's end location points to the first character of the last token,
        // so we need to advance past it.
        end = Lexer::getLocForEndOfToken(end, 0, SM, Ctx.getLangOpts());

        bool invalid = false;
        const char *bPtr = SM.getCharacterData(begin, &invalid);
        if (invalid) return true;
        const char *ePtr = SM.getCharacterData(end, &invalid);
        if (invalid) return true;

        Info.found          = true;
        Info.funcName       = name;
        Info.returnType     = FD->getReturnType().getAsString();
        Info.paramList      = paramList;
        Info.paramNames     = paramNames;
        Info.bodyText       = std::string(bPtr, ePtr);
        Info.bodyBeginOffset = SM.getFileOffset(begin);
        Info.bodyEndOffset   = SM.getFileOffset(end);
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Level emitters — produce transformed source strings
// ─────────────────────────────────────────────────────────────────────────────

/*
 * Each emitter receives:
 *   src     — the full original source text
 *   info    — AST info about the _bad function
 *
 * and returns the complete transformed source string.
 *
 * Strategy: rather than AST rewriting (fragile for complex transforms),
 * we do structured text surgery:
 *   1. Everything before the _bad body  → kept verbatim
 *   2. The _bad body                    → replaced / wrapped
 *   3. Everything after the _bad body   → kept verbatim
 *   + optional new helper functions injected before _bad
 */

// ── L0: verbatim copy ────────────────────────────────────────────────────────
static std::string emitL0(const std::string &src, const BadFuncInfo &) {
    return src;
}

// ── L1: inter-procedural split ───────────────────────────────────────────────
//
// Wraps the entire _bad body inside a new helper function
// `<name>_impl(...)` and replaces the original body with a call to it.
// The defect now crosses a function boundary.
//
static std::string emitL1(const std::string &src, const BadFuncInfo &info) {
    // Build helper: same signature, same body
    std::string helper;
    helper  = "/* L1: defect moved into helper — inter-procedural boundary */\n";
    helper += "static " + info.returnType + " " + info.funcName + "_impl"
            + info.paramList + "\n";
    helper += info.bodyText + "\n\n";

    // Build replacement body for original _bad
    std::string newBody = "{\n";
    if (info.paramNames.empty())
        newBody += "    " + info.funcName + "_impl();\n";
    else
        newBody += "    " + info.funcName + "_impl(" + info.paramNames + ");\n";
    newBody += "}";

    // Splice: insert helper before the _bad function's body, replace body
    // Find start of _bad function (search backwards from body for the return type)
    // Simpler: insert helper text just before bodyBeginOffset, then replace body.
    std::string before = src.substr(0, info.bodyBeginOffset);
    std::string after  = src.substr(info.bodyEndOffset);

    // Find the function declaration line to insert helper before it
    // (search backwards in `before` for the last '\n' before the function keyword)
    size_t insertPos = before.rfind('\n');
    // Walk back further to find the blank line / comment before function
    if (insertPos != std::string::npos) {
        size_t tmp = before.rfind('\n', insertPos - 1);
        if (tmp != std::string::npos) insertPos = tmp + 1;
        else insertPos++;
    } else {
        insertPos = 0;
    }

    std::string result;
    result.reserve(src.size() + helper.size() + 64);
    result  = src.substr(0, insertPos);
    result += helper;
    result += src.substr(insertPos, info.bodyBeginOffset - insertPos);
    result += newBody;
    result += after;
    return result;
}

// ── L2: aliasing ─────────────────────────────────────────────────────────────
//
// Scans the _bad body for the first pointer/array variable declared via
// malloc/new/address-of and introduces an alias through a struct field.
// The body is rewritten so the defect operates through `container.ptr`
// rather than the direct variable.
//
// Because we only want a simple text transform (not full type inference),
// we apply a conservative heuristic:
//   - find "TYPE *NAME = " or "TYPE* NAME =" in the body
//   - wrap body in a lambda-like struct that holds a copy of that pointer
//
// This produces readable, compilable C++ that adds one level of indirection.
//
static std::string emitL2(const std::string &src, const BadFuncInfo &info) {
    const std::string &body = info.bodyText;

    // Heuristic: find first "* varname" declaration in body
    // Pattern: look for lines containing "malloc(" or "new " or "= &"
    std::string ptrVarName;
    std::string ptrTypeName;
    {
        std::istringstream ss(body);
        std::string line;
        while (std::getline(ss, line)) {
            // Skip comments
            size_t commentPos = line.find("//");
            std::string code  = (commentPos != std::string::npos)
                                ? line.substr(0, commentPos) : line;

            bool hasAlloc = (code.find("malloc(") != std::string::npos ||
                             code.find("new ")     != std::string::npos ||
                             code.find("new[")     != std::string::npos);
            if (!hasAlloc) continue;

            // Extract: look for pattern "*  identifier  ="
            size_t star = code.rfind('*');
            if (star == std::string::npos) continue;
            size_t nameStart = star + 1;
            while (nameStart < code.size() && std::isspace((unsigned char)code[nameStart]))
                nameStart++;
            size_t nameEnd = nameStart;
            while (nameEnd < code.size() &&
                   (std::isalnum((unsigned char)code[nameEnd]) || code[nameEnd] == '_'))
                nameEnd++;
            if (nameEnd <= nameStart) continue;
            ptrVarName = code.substr(nameStart, nameEnd - nameStart);

            // Extract type: everything before the '*'
            size_t typeEnd   = star;
            while (typeEnd > 0 && std::isspace((unsigned char)code[typeEnd - 1]))
                typeEnd--;
            // Find type start: after last space/newline before typeEnd
            size_t typeStart = 0;
            for (size_t i = typeEnd; i-- > 0;) {
                if (std::isspace((unsigned char)code[i])) { typeStart = i + 1; break; }
            }
            ptrTypeName = code.substr(typeStart, typeEnd - typeStart);
            break;
        }
    }

    if (ptrVarName.empty()) {
        // Fallback: emit body unchanged with a comment
        std::string newBody = "{\n    /* L2: alias transform skipped — no pointer decl found */\n";
        newBody += body.substr(1); // drop leading '{'
        return src.substr(0, info.bodyBeginOffset) + newBody + src.substr(info.bodyEndOffset);
    }

    // Build wrapper struct + aliased access
    std::string aliasStruct =
        "    /* L2: alias introduced through struct field */\n"
        "    struct AliasContainer {\n"
        "        " + ptrTypeName + " *ptr = nullptr;\n"
        "    } _alias_container;\n";

    // In the body, replace the first occurrence of ptrVarName with a local
    // that is also assigned to _alias_container.ptr, and then replace
    // subsequent uses of ptrVarName with _alias_container.ptr
    // We do this at text level: insert assignment after declaration line,
    // then rename uses.

    std::string modifiedBody = body;

    // Find the declaration line of ptrVarName in body and append alias assignment
    {
        std::string marker = "* " + ptrVarName;
        size_t pos = modifiedBody.find(marker);
        if (pos == std::string::npos) {
            marker = "*" + ptrVarName;
            pos = modifiedBody.find(marker);
        }
        if (pos != std::string::npos) {
            size_t lineEnd = modifiedBody.find('\n', pos);
            if (lineEnd == std::string::npos) lineEnd = modifiedBody.size();
            std::string assignment = "\n    _alias_container.ptr = " + ptrVarName + ";";
            modifiedBody.insert(lineEnd, assignment);
        }
    }

    // Replace uses of ptrVarName (not in comments, not in declaration)
    // with _alias_container.ptr — simple pass after the first declaration line
    {
        size_t declLine = modifiedBody.find(ptrVarName);
        if (declLine != std::string::npos) {
            // skip the declaration line itself
            size_t afterDecl = modifiedBody.find('\n', declLine);
            if (afterDecl == std::string::npos) afterDecl = modifiedBody.size();
            afterDecl++; // past newline

            std::string tail = modifiedBody.substr(afterDecl);
            std::string out;
            size_t searchFrom = 0;
            while (true) {
                size_t found = tail.find(ptrVarName, searchFrom);
                if (found == std::string::npos) { out += tail.substr(searchFrom); break; }

                // Only replace if surrounded by non-identifier chars
                bool prevOk = (found == 0) ||
                              !std::isalnum((unsigned char)tail[found - 1]) && tail[found - 1] != '_';
                size_t after = found + ptrVarName.size();
                bool nextOk  = (after >= tail.size()) ||
                               !std::isalnum((unsigned char)tail[after]) && tail[after] != '_';

                if (prevOk && nextOk) {
                    out += tail.substr(searchFrom, found - searchFrom);
                    out += "_alias_container.ptr";
                    searchFrom = after;
                } else {
                    out += tail.substr(searchFrom, found - searchFrom + 1);
                    searchFrom = found + 1;
                }
            }
            modifiedBody = modifiedBody.substr(0, afterDecl) + out;
        }
    }

    // Insert aliasStruct after the opening brace of the body
    size_t openBrace = modifiedBody.find('{');
    if (openBrace != std::string::npos)
        modifiedBody.insert(openBrace + 1, "\n" + aliasStruct);

    return src.substr(0, info.bodyBeginOffset) + modifiedBody + src.substr(info.bodyEndOffset);
}

// ── L3: opaque control flow ──────────────────────────────────────────────────
//
// Wraps the entire _bad body inside:
//
//   if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1)) { <body> }
//
// The condition is always true at runtime (we pass the env var when testing)
// but static analyzers typically cannot resolve getenv() returns, so many
// will consider the branch unreachable or uncertain.
//
static std::string emitL3(const std::string &src, const BadFuncInfo &info) {
    const std::string &body = info.bodyText;

    // Strip outer braces to re-wrap
    std::string inner = body;
    size_t ob = inner.find('{');
    size_t cb = inner.rfind('}');
    if (ob != std::string::npos && cb != std::string::npos && cb > ob)
        inner = inner.substr(ob + 1, cb - ob - 1);

    std::string newBody =
        "{\n"
        "    /* L3: defect wrapped in opaque runtime condition */\n"
        "    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */\n"
        "    if (__builtin_expect(getenv(\"MUTATOR_TRIGGER\") != nullptr, 1))\n"
        "    {\n"
        + inner +
        "    }\n"
        "}";

    // Ensure <cstdlib> is included (getenv)
    std::string result = src.substr(0, info.bodyBeginOffset) + newBody + src.substr(info.bodyEndOffset);
    if (result.find("<cstdlib>") == std::string::npos &&
        result.find("<stdlib.h>") == std::string::npos) {
        size_t firstInclude = result.find("#include");
        if (firstInclude != std::string::npos)
            result.insert(firstInclude, "#include <cstdlib> /* L3: for getenv */\n");
    }
    return result;
}

// ── L4: loop wrapping ────────────────────────────────────────────────────────
//
// Wraps the body inside a loop that iterates MUTATOR_LOOP_ITERS times
// (default 1).  The defect now occurs inside a loop; many analyzers
// stop unrolling after a small fixed bound.
//
static std::string emitL4(const std::string &src, const BadFuncInfo &info) {
    const std::string &body = info.bodyText;

    std::string inner = body;
    size_t ob = inner.find('{');
    size_t cb = inner.rfind('}');
    if (ob != std::string::npos && cb != std::string::npos && cb > ob)
        inner = inner.substr(ob + 1, cb - ob - 1);

    std::string newBody =
        "{\n"
        "    /* L4: defect wrapped in loop — tests loop-aware reasoning */\n"
        "    const int _loop_iters = 1; /* keep to 1 to avoid multiple crashes */\n"
        "    for (int _i = 0; _i < _loop_iters; ++_i)\n"
        "    {\n"
        + inner +
        "    }\n"
        "}";

    return src.substr(0, info.bodyBeginOffset) + newBody + src.substr(info.bodyEndOffset);
}

// ── L5: combined (L1 + L2 + L3) ─────────────────────────────────────────────
//
// Applies inter-procedural split on the already-aliased body, then wraps
// the caller in an opaque branch.  Produces the deepest transformation.
//
static std::string emitL5(const std::string &src, const BadFuncInfo &info) {
    // Step 1: apply aliasing transform to get intermediate source + updated info
    std::string s1 = emitL2(src, info);

    // Step 2: re-parse info from s1 is not possible without another AST pass,
    // so we do a text-level extraction of the (already modified) _bad body.
    // Find the function again by name in s1.
    std::string marker = info.returnType + " " + info.funcName;
    size_t fpos = s1.find(marker);
    if (fpos == std::string::npos) {
        // fallback: just apply L3 on top of L2 result
        BadFuncInfo info2 = info;
        // body offsets shifted; redo offset search
        size_t bstart = s1.find(info.funcName);
        if (bstart != std::string::npos) {
            size_t brace = s1.find('{', bstart);
            if (brace != std::string::npos) {
                info2.bodyBeginOffset = (unsigned)brace;
                // find matching close brace
                int depth = 0; size_t p = brace;
                for (; p < s1.size(); ++p) {
                    if (s1[p] == '{') depth++;
                    else if (s1[p] == '}') { depth--; if (!depth) break; }
                }
                info2.bodyEndOffset = (unsigned)(p + 1);
                info2.bodyText = s1.substr(brace, p + 1 - brace);
                return emitL3(s1, info2);
            }
        }
        return s1;
    }

    // Find matching brace block for _bad in s1
    size_t braceStart = s1.find('{', fpos);
    if (braceStart == std::string::npos) return s1;
    int depth = 0; size_t p = braceStart;
    for (; p < s1.size(); ++p) {
        if (s1[p] == '{') depth++;
        else if (s1[p] == '}') { if (!--depth) break; }
    }
    size_t braceEnd = p + 1;

    BadFuncInfo info2      = info;
    info2.bodyText         = s1.substr(braceStart, braceEnd - braceStart);
    info2.bodyBeginOffset  = (unsigned)braceStart;
    info2.bodyEndOffset    = (unsigned)braceEnd;

    // Step 2: apply inter-procedural on s1
    std::string s2 = emitL1(s1, info2);

    // Step 3: find _bad body in s2 (now just a forwarder) and wrap in L3
    size_t fpos2 = s2.find(info.funcName);
    if (fpos2 == std::string::npos) return s2;
    // skip _impl occurrence
    size_t realFpos = s2.find(info.funcName, fpos2);
    // find the non-_impl occurrence (the original wrapper)
    while (realFpos != std::string::npos) {
        size_t after = realFpos + info.funcName.size();
        if (s2.substr(after, 5) != "_impl") break;
        realFpos = s2.find(info.funcName, after);
    }
    if (realFpos == std::string::npos) return s2;

    size_t braceStart2 = s2.find('{', realFpos);
    if (braceStart2 == std::string::npos) return s2;
    depth = 0; p = braceStart2;
    for (; p < s2.size(); ++p) {
        if (s2[p] == '{') depth++;
        else if (s2[p] == '}') { if (!--depth) break; }
    }
    size_t braceEnd2 = p + 1;

    BadFuncInfo info3     = info;
    info3.bodyText        = s2.substr(braceStart2, braceEnd2 - braceStart2);
    info3.bodyBeginOffset = (unsigned)braceStart2;
    info3.bodyEndOffset   = (unsigned)braceEnd2;

    return emitL3(s2, info3);
}

// ─────────────────────────────────────────────────────────────────────────────
// ASTConsumer + FrontendAction
// ─────────────────────────────────────────────────────────────────────────────

class MutatorConsumer : public ASTConsumer {
    ASTContext      &Ctx;
    std::string      InputFile;
    std::set<int>    Levels;
    std::string      OutDirPath;

public:
    MutatorConsumer(ASTContext &ctx, std::string inputFile,
                    std::set<int> levels, std::string outDir)
        : Ctx(ctx), InputFile(std::move(inputFile)),
          Levels(std::move(levels)), OutDirPath(std::move(outDir)) {}

    void HandleTranslationUnit(ASTContext &ctx) override {
        BadFuncInfo info;
        DefectVisitor visitor(ctx, info);
        visitor.TraverseDecl(ctx.getTranslationUnitDecl());

        if (!info.found) {
            llvm::errs() << "[mutator] No _bad() function found in "
                         << InputFile << " — skipping.\n";
            return;
        }

        llvm::outs() << "[mutator] Found: " << info.funcName
                     << "  in " << InputFile << "\n";

        std::string src = readFile(InputFile);
        if (src.empty()) {
            llvm::errs() << "[mutator] Cannot read " << InputFile << "\n";
            return;
        }

        using EmitterFn = std::string (*)(const std::string &, const BadFuncInfo &);
        const EmitterFn emitters[] = { emitL0, emitL1, emitL2, emitL3, emitL4, emitL5 };
        const char *labels[] = { "L0_base", "L1_interprocedural",
                                  "L2_aliasing", "L3_opaque_branch",
                                  "L4_loop_wrap", "L5_combined" };

        for (int lvl : Levels) {
            if (lvl < 0 || lvl > 5) continue;
            std::string transformed = emitters[lvl](src, info);

            // Insert a header comment identifying level
            std::string header =
                "/* === MUTATOR: " + std::string(labels[lvl]) + " ===\n"
                " *   Source  : " + InputFile + "\n"
                " *   Function: " + info.funcName + "\n"
                " * ======================================================= */\n";
            transformed = header + transformed;

            std::string outPath = buildOutPath(OutDirPath, lvl, InputFile);
            if (writeFile(outPath, transformed))
                llvm::outs() << "  -> " << outPath << "\n";
        }
    }
};

class MutatorAction : public ASTFrontendAction {
    std::set<int> Levels;
    std::string   OutDirPath;

public:
    MutatorAction(std::set<int> levels, std::string outDir)
        : Levels(std::move(levels)), OutDirPath(std::move(outDir)) {}

    std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        return std::make_unique<MutatorConsumer>(
            CI.getASTContext(), file.str(), Levels, OutDirPath);
    }
};

// Factory so ClangTool can create action instances per TU
class MutatorActionFactory : public FrontendActionFactory {
    std::set<int> Levels;
    std::string   OutDirPath;

public:
    MutatorActionFactory(std::set<int> levels, std::string outDir)
        : Levels(std::move(levels)), OutDirPath(std::move(outDir)) {}

    std::unique_ptr<FrontendAction> create() override {
        return std::make_unique<MutatorAction>(Levels, OutDirPath);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, const char **argv) {
    auto expectedParser =
        CommonOptionsParser::create(argc, argv, MutatorCategory);
    if (!expectedParser) {
        llvm::errs() << llvm::toString(expectedParser.takeError());
        return 1;
    }
    CommonOptionsParser &op = expectedParser.get();

    std::set<int>  levels  = parseLevels(LevelsOpt);
    std::string    outDir  = OutDir;

    llvm::outs() << "[mutator] Output directory : " << outDir << "\n";
    llvm::outs() << "[mutator] Levels requested :";
    for (int l : levels) llvm::outs() << " " << l;
    llvm::outs() << "\n";

    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    MutatorActionFactory factory(levels, outDir);
    return tool.run(&factory);
}
