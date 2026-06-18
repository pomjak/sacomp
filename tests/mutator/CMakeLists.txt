cmake_minimum_required(VERSION 3.20)
project(mutator CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # handy for clangd

# ── Find LLVM / Clang ─────────────────────────────────────────────────────────
# On Arch Linux with llvm / clang packages the cmake modules live in:
#   /usr/lib/cmake/llvm/   and   /usr/lib/cmake/clang/
#
# If they are installed elsewhere, pass:
#   cmake -DLLVM_DIR=/path/to/lib/cmake/llvm \
#         -DClang_DIR=/path/to/lib/cmake/clang  ..

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "LLVM version : ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM headers : ${LLVM_INCLUDE_DIRS}")
message(STATUS "Clang headers: ${CLANG_INCLUDE_DIRS}")

# ── Include dirs ──────────────────────────────────────────────────────────────
include_directories(
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

# LLVM may have been built with RTTI off; match that to avoid link errors
if(NOT LLVM_ENABLE_RTTI)
    add_compile_options(-fno-rtti)
endif()

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(mutator mutator.cpp)

# Clang LibTooling libraries required:
#   clangTooling           — ClangTool, CommonOptionsParser, FrontendActionFactory
#   clangFrontend          — CompilerInstance, FrontendAction
#   clangAST               — ASTContext, Decl, Stmt, RecursiveASTVisitor
#   clangASTMatchers       — (optional but often needed transitively)
#   clangBasic             — SourceManager, SourceLocation, LangOptions
#   clangLex               — Lexer
#   clangRewrite           — Rewriter
#   clangSerialization     — PCH support (required by clangFrontend)
#   clangDriver            — Driver (required by clangTooling on some builds)
#
# The LLVM cmake helpers expose llvm_map_components_to_libnames for LLVM libs.
# For Clang libraries we list them explicitly.

llvm_map_components_to_libnames(LLVM_LIBS
    support
    core
    option          # command-line option parsing used by clangTooling
)

target_link_libraries(mutator
    PRIVATE
    # Clang libraries (order matters for static linking)
    clangTooling
    clangToolingCore
    clangFrontend
    clangFrontendTool
    clangDriver
    clangSerialization
    clangParse
    clangSema
    clangAnalysis
    clangEdit
    clangAST
    clangASTMatchers
    clangLex
    clangBasic
    clangRewrite
    # LLVM libraries
    ${LLVM_LIBS}
)
