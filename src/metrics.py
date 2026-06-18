## metrics and evaluation

from collections import defaultdict
from dataclasses import dataclass
from dataclasses import field


def evaluate(findings, test_path, stats, tool):
    expected = test_path.parent.name
    expected_stats = stats.by_category[expected]

    if not findings:
        print(test_path)
        print(f"{tool}: No findings, Expected: {expected}\n")
        stats.overall.fn += 1
        expected_stats.fn += 1
        return

    detected_correct = False

    for finding in findings:
        if finding.category == expected:
            detected_correct = True
        else:
            print(test_path)
            print(f"{tool}: FAIL: {finding.category}, Expected: {expected}\n")
            stats.overall.fp += 1
            stats.by_category[finding.category].fp += 1

    if detected_correct:
        stats.overall.tp += 1
        expected_stats.tp += 1
    else:
        stats.overall.fn += 1
        expected_stats.fn += 1


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
