## metrics and evaluation

from collections import defaultdict
from dataclasses import dataclass
from dataclasses import field

def print_all_stats(stats):
    for tool, stat in stats.items():
        print(tool)

        print("Overall:")
        print_stats(stat.overall, indent=2)

        print("By category:")
        print_category_stats(stat)

        print("By category and level:")
        print_category_level_stats(stat)

def print_stats(stat, indent=0):
    prefix = " " * indent

    print(f"{prefix}TP:", stat.tp)
    print(f"{prefix}FP:", stat.fp)
    print(f"{prefix}FN:", stat.fn)
    print(f"{prefix}Precision:", stat.precision())
    print(f"{prefix}Recall:", stat.recall())
    print(f"{prefix}F1:", stat.f1())


def print_category_stats(stat, indent=2):
    for category, category_stat in sorted(stat.by_category.items()):
        print(" " * indent + category)
        print_stats(category_stat, indent + 2)


def print_category_level_stats(stat, indent=2):
    for category, level_stats in sorted(stat.by_category_level.items()):
        print(" " * indent + category)

        for level, level_stat in sorted(level_stats.items()):
            print(" " * (indent + 2) + f"Level {level}")
            print_stats(level_stat, indent + 4)

def evaluate(findings, test_path, stats, tool):
    expected = test_path.parent.parent.name
    level = test_path.parent.name
    expected_stats = stats.by_category[expected]
    level_stats = stats.by_level[level]
    category_level_stats = stats.by_category_level[expected][level]

    if not findings:
        print(f"{tool}: \033[91mNo findings\033[0m, Expected: {expected}")
        stats.overall.fn += 1
        expected_stats.fn += 1
        level_stats.fn += 1
        category_level_stats.fn += 1
        return

    detected_correct = False

    for finding in findings:
        if finding.category == expected:
            detected_correct = True
        else:
            print(f"{tool}: \033[93mNoise\033[0m: {finding.category}, Expected: {expected}")
            stats.overall.fp += 1
            stats.by_category[finding.category].fp += 1
            stats.by_level[level].fp += 1
            stats.by_category_level[finding.category][level].fp += 1

    if detected_correct:
        stats.overall.tp += 1
        expected_stats.tp += 1
        level_stats.tp += 1
        category_level_stats.tp += 1
    else:
        stats.overall.fn += 1
        expected_stats.fn += 1
        level_stats.fn += 1
        category_level_stats.fn += 1


@dataclass
class Stats:
    tp: int = 0
    fp: int = 0
    fn: int = 0

    def precision(self):
        if self.tp + self.fp == 0:
            return 0
        return self.tp / (self.tp + self.fp)

    def recall(self):
        if self.tp + self.fn == 0:
            return 0
        return self.tp / (self.tp + self.fn)

    def f1(self):
        p = self.precision()
        r = self.recall()

        if p + r == 0:
            return 0

        return 2 * p * r / (p + r)


@dataclass
class ToolStats:
    overall: Stats = field(default_factory=Stats)

    by_category: dict[str, Stats] = field(default_factory=lambda: defaultdict(Stats))

    by_level: dict[int, Stats] = field(default_factory=lambda: defaultdict(Stats))

    by_category_level: dict[str, dict[int, Stats]] = field(
        default_factory=lambda: defaultdict(lambda: defaultdict(Stats))
    )
