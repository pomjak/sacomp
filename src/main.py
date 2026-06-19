import subprocess
import tools
import parser
import graphs
from metrics import *
from pathlib import Path
import sys


def cleanup():
    subprocess.run("rm -rf infer-out/ *.o", shell=True)


def find_cpp_files(folder: Path) -> list[Path]:
    return [path for path in folder.rglob("*.cpp") if path.is_file()]


def main(argv):
    if len(argv) != 2:
        print(f"Usage: {argv[0]} <folder>")
        return 1

    stats = {
        "cppcheck": ToolStats(),
        "clang": ToolStats(),
        "infer": ToolStats(),
    }

    folder = Path(argv[1])
    if not folder.is_dir():
        print(f"Error: {folder} is not a folder")
        return 1

    for test_file in find_cpp_files(folder):
        print(test_file)
        for tool in ["cppcheck", "clang", "infer"]:
            raw = tools.run(tool, str(test_file))
            out = parser.process(tool, raw)
            evaluate(out, test_file, stats[tool], tool)

    print_all_stats(stats)
    graphs.plot_benchmark_results(stats)

    cleanup()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
