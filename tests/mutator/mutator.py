"""
mutator.py  —  Python/libclang source-level mutator for static analysis test cases.

Produces complexity layers L0–L5 for each input .cpp file:

  L0  base              — verbatim copy
  L1  interprocedural   — defect body extracted into a _bad_impl() helper
  L2  aliasing          — defective pointer re-routed through AliasContainer field
  L3  opaque_branch     — body wrapped in if(getenv("MUTATOR_TRIGGER")!=nullptr)
  L4  loop_wrap         — body wrapped in for(_i=0;_i<1;++_i)
  L5  combined          — L2 → L1 → L3 chained
"""

import os
import re
import sys
import argparse
from clang.cindex import Index, CursorKind, TranslationUnit, Diagnostic


def find_matching_brace(text: str, start_index: int) -> int:
    depth = 0
    for i in range(start_index, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def build_out_path(out_dir: str, level: int, input_path: str) -> str:
    filename = os.path.basename(input_path)
    return os.path.join(out_dir, f"L{level}", filename)


def split_argv_on_dashdash(argv: list) -> tuple:
    if "--" in argv:
        idx = argv.index("--")
        return argv[:idx], argv[idx + 1 :]
    return argv, []

def extract_bad_func_info(node) -> dict | None:
    """Walk the AST and return info about the first function whose name ends in '_bad'

    """
    if node.kind == CursorKind.FUNCTION_DECL and node.is_definition():
        func_name = node.spelling
        if func_name and func_name.endswith("_bad"):
            body_node = next(
                (c for c in node.get_children() if c.kind == CursorKind.COMPOUND_STMT),
                None,
            )
            if body_node:
                args = list(node.get_arguments())
                param_list = (
                    "("
                    + ", ".join(f"{p.type.spelling} {p.spelling}" for p in args)
                    + ")"
                )
                param_names = ", ".join(p.spelling for p in args)

                return {
                    "func_name": func_name,
                    "return_type": node.result_type.spelling,
                    "param_list": param_list,
                    "param_names": param_names,
                    "body_begin_offset": body_node.extent.start.offset,
                    "body_end_offset": body_node.extent.end.offset + 1,  # BUG 3 fix
                    "func_begin_offset": node.extent.start.offset,
                }

    for child in node.get_children():
        result = extract_bad_func_info(child)
        if result:
            return result

    return None


def emit_L0(src: str, info: dict) -> str:
    """L0: Verbatim copy — no transformation."""
    return src


def emit_L1(src: str, info: dict) -> str:
    """L1: Inter-procedural split.

    """
    body_text = src[info["body_begin_offset"] : info["body_end_offset"]]

    helper = (
        "/* L1: defect moved into helper — inter-procedural boundary */\n"
        f"static {info['return_type']} {info['func_name']}_impl{info['param_list']}\n"
        f"{body_text}\n\n"
    )

    call = (
        f"{info['func_name']}_impl({info['param_names']})"
        if info["param_names"]
        else f"{info['func_name']}_impl()"
    )
    new_body = f"{{\n    {call};\n}}"

    before = src[: info["func_begin_offset"]]
    middle = src[info["func_begin_offset"] : info["body_begin_offset"]]
    after = src[info["body_end_offset"] :]

    return before + helper + middle + new_body + after


def emit_L2(src: str, info: dict) -> str:
    """L2: Aliasing."""
    body = src[info["body_begin_offset"] : info["body_end_offset"]]

    ptr_var_name = ""
    ptr_type_name = ""

    for line in body.split("\n"):
        code = line.split("//")[0]  # strip line comments
        if "malloc(" not in code and "new " not in code and "new[" not in code:
            continue
        # Match:  SomeType  *  varname  =
        m = re.search(
            r"([A-Za-z_][A-Za-z0-9_:]*)\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=", code
        )
        if m:
            ptr_type_name, ptr_var_name = m.groups()
            break

    if not ptr_var_name:
        open_b = body.find("{")
        new_body = (
            body[: open_b + 1]
            + "\n    /* L2: alias transform skipped — no pointer decl found */\n"
            + body[open_b + 1 :]
        )
        return (
            src[: info["body_begin_offset"]] + new_body + src[info["body_end_offset"] :]
        )

    alias_struct = (
        "    /* L2: alias introduced through struct field */\n"
        "    struct AliasContainer {\n"
        f"        {ptr_type_name} *ptr = nullptr;\n"
        "    } _alias_container;\n"
    )

    lines = body.split("\n")
    modified_lines: list[str] = []
    found_decl = False

    for line in lines:
        code = line.split("//")[0]
        is_decl = (
            not found_decl
            and ptr_var_name in code
            and ("*" in code or "malloc" in code or "new" in code)
        )
        if is_decl:
            modified_lines.append(line)
            # Assignment uses the original var name (not yet renamed)
            modified_lines.append(f"    _alias_container.ptr = {ptr_var_name};")
            found_decl = True
        elif found_decl:
            # Replace whole-word occurrences only (avoid mangling e.g. 'buf_size')
            new_line = re.sub(
                rf"\b{re.escape(ptr_var_name)}\b", "_alias_container.ptr", line
            )
            modified_lines.append(new_line)
        else:
            modified_lines.append(line)

    modified_body = "\n".join(modified_lines)

    # Inject AliasContainer declaration right after the opening '{'
    open_b = modified_body.find("{")
    if open_b != -1:
        modified_body = (
            modified_body[: open_b + 1]
            + "\n"
            + alias_struct
            + modified_body[open_b + 1 :]
        )

    return (
        src[: info["body_begin_offset"]]
        + modified_body
        + src[info["body_end_offset"] :]
    )


def _strip_outer_braces(body: str) -> str:
    """Return the content between the outermost { } of a CompoundStmt body."""
    first = body.find("{")
    if first == -1:
        return body
    last = find_matching_brace(body, first)
    if last == -1:
        return body[first + 1 :]
    return body[first + 1 : last]


def emit_L3(src: str, info: dict) -> str:
    """L3: Opaque control flow."""
    body = src[info["body_begin_offset"] : info["body_end_offset"]]
    inner = _strip_outer_braces(body)

    new_body = (
        "{\n"
        "    /* L3: defect wrapped in opaque runtime condition */\n"
        "    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */\n"
        '    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))\n'
        "    {\n"
        f"{inner}\n"
        "    }\n"
        "}"
    )

    result = (
        src[: info["body_begin_offset"]] + new_body + src[info["body_end_offset"] :]
    )

    # Ensure <cstdlib> is present for getenv()
    if "<cstdlib>" not in result and "<stdlib.h>" not in result:
        first_include = result.find("#include")
        if first_include != -1:
            result = (
                result[:first_include]
                + "#include <cstdlib> /* L3: for getenv */\n"
                + result[first_include:]
            )

    return result


def emit_L4(src: str, info: dict) -> str:
    """L4: Loop wrapping."""
    body = src[info["body_begin_offset"] : info["body_end_offset"]]
    inner = _strip_outer_braces(body)

    new_body = (
        "{\n"
        "    /* L4: defect wrapped in loop — tests loop-aware reasoning */\n"
        "    const int _loop_iters = 1;\n"
        "    for (int _i = 0; _i < _loop_iters; ++_i)\n"
        "    {\n"
        f"{inner}\n"
        "    }\n"
        "}"
    )

    return src[: info["body_begin_offset"]] + new_body + src[info["body_end_offset"] :]


def emit_L5(src: str, info: dict) -> str:
    """L5: Combined transformation — L2 → L1 → L3."""
    s1 = emit_L2(src, info)

    # Re-locate the _bad function body in the transformed source s1.
    # Use a regex to find the function signature and skip the _impl helper.

    func_name = info["func_name"]
    ret_type = info["return_type"]

    # Pattern: optional "static " prefix, then return_type, whitespace, func_name
    # NOT followed by "_impl", then a word boundary.
    pattern = (
        rf"(?:static\s+)?{re.escape(ret_type)}\s+" rf"{re.escape(func_name)}(?!_impl)\b"
    )
    m = re.search(pattern, s1)
    if not m:
        return s1  # fallback: could not re-locate function

    fpos = m.start()
    brace_start = s1.find("{", fpos)
    if brace_start == -1:
        return s1
    brace_end = find_matching_brace(s1, brace_start) + 1  # +1: include '}'

    info2 = info.copy()
    info2["func_begin_offset"] = fpos
    info2["body_begin_offset"] = brace_start
    info2["body_end_offset"] = brace_end

    s2 = emit_L1(s1, info2)

    m2 = re.search(pattern, s2)
    if not m2:
        return s2

    fpos2 = m2.start()
    brace_start2 = s2.find("{", fpos2)
    if brace_start2 == -1:
        return s2
    brace_end2 = find_matching_brace(s2, brace_start2) + 1

    info3 = info.copy()
    info3["body_begin_offset"] = brace_start2
    info3["body_end_offset"] = brace_end2

    return emit_L3(s2, info3)


EMITTERS = {0: emit_L0, 1: emit_L1, 2: emit_L2, 3: emit_L3, 4: emit_L4, 5: emit_L5}
LABELS = {
    0: "L0_base",
    1: "L1_interprocedural",
    2: "L2_aliasing",
    3: "L3_opaque_branch",
    4: "L4_loop_wrap",
    5: "L5_combined",
}


def main() -> None:
    tool_argv, compiler_args = split_argv_on_dashdash(sys.argv[1:])

    parser = argparse.ArgumentParser(
        description="Python/libclang source mutator for static analysis test cases.",
        epilog="Separate mutator args from compiler args with --:  mutator.py f.cpp -- -std=c++14",
    )
    parser.add_argument(
        "--outdir", default="./mutated", help="Output directory (default: ./mutated)"
    )
    parser.add_argument(
        "--levels",
        default="0,1,2,3,4,5",
        help="Comma-separated level numbers to generate",
    )
    parser.add_argument("files", nargs="+", help="Input .cpp source files")

    args = parser.parse_args(tool_argv)

    levels: set[int] = set()
    for tok in args.levels.split(","):
        tok = tok.strip()
        if tok.isdigit():
            levels.add(int(tok))

    print(f"[mutator] Output directory : {args.outdir}")
    print(f"[mutator] Levels requested : {sorted(levels)}")
    print(f"[mutator] Compiler args    : {compiler_args}")

    index = Index.create()

    for input_file in args.files:
        if not os.path.isfile(input_file):
            print(f"[mutator] Cannot read {input_file} — skipping.")
            continue

        with open(input_file, encoding="utf-8") as fh:
            src = fh.read()

        # Parse with libclang; forward compiler flags for accurate type info
        tu = index.parse(
            input_file,
            args=compiler_args,
            options=TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD,
        )

        diag_warned = False
        for diag in tu.diagnostics:
            if diag.severity >= Diagnostic.Warning:
                if not diag_warned:
                    print(f"  [clang diag] {input_file}:")
                    diag_warned = True
                print(f"    {diag.severity}: {diag.spelling}")

        info = extract_bad_func_info(tu.cursor)

        if info is None:
            print(f"[mutator] No _bad() function found in {input_file} — skipping.")
            continue

        print(f'[mutator] Found: {info["func_name"]}  in {input_file}')

        for lvl in sorted(levels):
            if lvl not in EMITTERS:
                continue

            transformed = EMITTERS[lvl](src, info)

            header = (
                f"/* === MUTATOR: {LABELS[lvl]} ===\n"
                f" * Source  : {input_file}\n"
                f' * Function: {info["func_name"]}\n'
                " * ======================================================= */\n"
            )
            transformed = header + transformed

            out_path = build_out_path(args.outdir, lvl, input_file)
            os.makedirs(os.path.dirname(out_path), exist_ok=True)

            with open(out_path, "w", encoding="utf-8") as fh:
                fh.write(transformed)

            print(f"  -> {out_path}")


if __name__ == "__main__":
    main()
