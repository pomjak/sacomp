import tools
import parser
from metrics import ToolStats, evaluate
from pathlib import Path
import sys


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
        for tool in ["cppcheck", "clang", "infer"]:
            stat = stats[tool]
            raw = tools.run(tool, str(test_file))
            out = parser.process(tool, raw)
            evaluate(out, test_file, stats[tool], tool)

    for tool, stat in stats.items():
        print(tool)

        print("TP:", stat.overall.tp)
        print("FP:", stat.overall.fp)
        print("FN:", stat.overall.fn)

        print("Precision:", stat.overall.precision())
        print("Recall:", stat.overall.recall())
        print("F1:", stat.overall.f1())

        for category, category_stat in sorted(stat.by_category.items()):
            print(f"  {category}")
            print("    TP:", category_stat.tp)
            print("    FP:", category_stat.fp)
            print("    FN:", category_stat.fn)
            print("    Precision:", category_stat.precision())
            print("    Recall:", category_stat.recall())
            print("    F1:", category_stat.f1())

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
