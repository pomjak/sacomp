import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns


def plot_benchmark_results(stats):
    overall_data = []
    level_data = []
    cat_data = []

    for tool, stat in stats.items():
        overall_data.append(
            {
                "Tool": tool,
                "Precision": stat.overall.precision(),
                "Recall": stat.overall.recall(),
                "F1": stat.overall.f1(),
                "TP": stat.overall.tp,
                "FP": stat.overall.fp,
                "FN": stat.overall.fn,
            }
        )

        for cat, cstat in stat.by_category.items():
            cat_data.append({"Tool": tool, "Category": cat, "F1": cstat.f1()})

        level_totals = {}
        for cat, levels in stat.by_category_level.items():
            for lvl, lstat in levels.items():
                if lvl not in level_totals:
                    level_totals[lvl] = {"tp": 0, "fp": 0, "fn": 0}
                level_totals[lvl]["tp"] += lstat.tp
                level_totals[lvl]["fp"] += lstat.fp
                level_totals[lvl]["fn"] += lstat.fn

        for lvl, counts in level_totals.items():
            denom = 2 * counts["tp"] + counts["fp"] + counts["fn"]
            f1 = (2 * counts["tp"] / denom) if denom > 0 else 0.0
            level_data.append({"Tool": tool, "Level": lvl, "F1": f1})

    df_overall = pd.DataFrame(overall_data)
    df_level = pd.DataFrame(level_data).sort_values(by="Level")
    df_cat = pd.DataFrame(cat_data)

    sns.set_theme(style="whitegrid")
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))

    sns.lineplot(
        data=df_level,
        x="Level",
        y="F1",
        hue="Tool",
        marker="o",
        linewidth=2.5,
        markersize=8,
        ax=axes[0, 0],
    )
    axes[0, 0].set_title(
        "Complexity Degradation (F1 Score vs. Level)", fontsize=14, pad=10
    )
    axes[0, 0].set_ylim(-0.05, 1.05)
    axes[0, 0].set_ylabel("F1 Score")

    df_overall_melted = df_overall.melt(
        id_vars="Tool",
        value_vars=["Precision", "Recall", "F1"],
        var_name="Metric",
        value_name="Score",
    )
    sns.barplot(
        data=df_overall_melted, x="Tool", y="Score", hue="Metric", ax=axes[0, 1]
    )
    axes[0, 1].set_title("Overall Tool Performance", fontsize=14, pad=10)
    axes[0, 1].set_ylim(0, 1.05)

    df_errors = df_overall.set_index("Tool")[["TP", "FP", "FN"]]
    df_errors.plot(
        kind="bar",
        stacked=True,
        ax=axes[1, 0],
        color=["#2ca02c", "#d62728", "#ff7f0e"],
        alpha=0.85,
    )
    axes[1, 0].set_title("Result Distribution (TP vs FP vs FN)", fontsize=14, pad=10)
    axes[1, 0].set_ylabel("Number of Test Cases")
    axes[1, 0].tick_params(axis="x", rotation=0)

    # Graph D: Category Specialization (Bottom Right)
    sns.barplot(data=df_cat, x="Category", y="F1", hue="Tool", ax=axes[1, 1])
    axes[1, 1].set_title(
        "Category Specialization (F1 Score by CWE)", fontsize=14, pad=10
    )
    axes[1, 1].set_ylim(-0.05, 1.05)
    axes[1, 1].set_ylabel("F1 Score")
    # Rotate category labels so they don't overlap if they are long
    axes[1, 1].tick_params(axis="x", rotation=45)

    # --- 4. Finalize and Save ---
    plt.tight_layout()
    plt.savefig("benchmark_results.png", dpi=300, bbox_inches="tight")
    print("Saved benchmark plots to 'benchmark_results.png'")
