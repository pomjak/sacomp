import tools
import parser
import metrics
from pathlib import Path
import sys


def find_cpp_files(folder: Path) -> list[Path]:
    return [path for path in folder.rglob("*.cpp") if path.is_file()]


def main(argv):
    if len(argv) != 2:
        print(f"Usage: {argv[0]} <folder>")
        return 1

    folder = Path(argv[1])
    if not folder.is_dir():
        print(f"Error: {folder} is not a folder")
        return 1

    for test_file in find_cpp_files(folder):
        raw_cpp = tools.run_cppcheck(str(test_file))
        raw_clang = tools.run_clang_sa(str(test_file))
        raw_infer = tools.run_infer(str(test_file))

        cpp_out = parser.process_cpp(raw_cpp)
        clang_out = parser.process_clang(raw_clang)
        infer_out = parser.process_infer(raw_infer)

        category = test_file.parent.name

        metrics.check_findings("Cppcheck", cpp_out, category)

        metrics.check_findings("Clang SA", clang_out, category)

        metrics.check_findings("Infer", infer_out, category)
        print("-------------------------")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
