import json
import matplotlib.pyplot as plt
import os
import numpy as np


if __name__ == "__main__":

    # Check if the JSON file exists
    if not os.path.exists("perf_results.json"):
        print("No results found. Please run the performance test first.")
        exit(1)

    with open("perf_results.json", "r", encoding="utf8") as f:
        data = json.load(f)

    results = data["results"]

    result_dict = {}
    batch_sizes = set()

    for result in results:
        name = result["name"]
        median = result["median(elapsed)"] * 1e9  # Convert to microseconds
        batch = result["batch"]
        median /= batch

        batch_sizes.add(batch)

        if name not in result_dict:
            result_dict[name] = []
        result_dict[name].append(median)

    fig = plt.figure(figsize=(10, 6))
    ax = fig.subplots()

    for name, times in result_dict.items():
        ax.plot(
            sorted(batch_sizes),
            times,
            label=name,
            marker="o",
            markersize=5,
            linewidth=2,
        )

    ax.set_ylabel("Time (ns)")
    ax.set_xlabel("Batch Size")
    ax.set_xscale("log")

    # Set the x-ticks to be the batch sizes
    ax.set_xticks(sorted(batch_sizes))
    ax.set_xticklabels(sorted(batch_sizes), rotation=45)
    ax.legend()
    ax.set_title("Performance Results")

    fig.savefig("perf_results_all.png", dpi=300)

    # ax.set_xlim((8, max(batch_sizes)))
    # ax.set_ylim((0, 100))
    # fig.savefig("perf_results_zoomed.png", dpi=300)

    # Plot bar graph for batch size 1, 4, 8, 64 and 512
    plot_batch = [1, 4, 8, 64, 512]
    for batch in plot_batch:

        fig = plt.figure(figsize=(10, 6))
        ax = fig.subplots()
        for name, times in result_dict.items():
            batch_index = sorted(batch_sizes).index(batch)
            bar = ax.barh(
                name,
                times[batch_index],
                label=name,
            )
            ax.bar_label(bar, fmt="%.2f", label_type="edge", fontsize=10)
        ax.set_xlabel("Time (ns)")
        # ax.tick_params(axis="x", rotation=55)
        ax.set_title(f"Performance Results for Batch Size {batch}")
        fig.tight_layout()
        fig.savefig(f"perf_results_{batch}.png", dpi=300)

    plt.show()
