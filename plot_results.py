import json
import matplotlib.pyplot as plt
import os
import numpy as np
import subprocess
import sys


def run_all():
    exe_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "build", "src", "Release"
    )

    exe_files = [os.path.join(exe_path, f) for f in os.listdir(exe_path)]

    # run all executables
    for exe_file in exe_files:
        print(f"Running {exe_file}...")
        if not os.path.isfile(exe_file):
            continue

        # Check if the file is an executable
        if not os.access(exe_file, os.X_OK):
            continue

        # Run the executable and save the output to a JSON file
        subprocess.run(
            [exe_file],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )


def plot_all():
    json_prefix = "perf_results_block_"
    perf_results = [
        f for f in os.listdir(".") if f.startswith(json_prefix) and f.endswith(".json")
    ]
    perf_results = sorted(perf_results)

    fig = plt.figure(figsize=(10, 6))
    ax = fig.subplots()

    for perf_result in perf_results:
        with open(perf_result, "r", encoding="utf8") as f:
            data = json.load(f)

        results = data["results"]

        values = []

        for result in results:
            name = result["name"]
            median = result["median(elapsed)"] * 1e9
            median /= result["batch"]
            block_size = name.split("_")[-1]
            block_size = int(block_size)

            values.append((block_size, median))

        values = sorted(values, key=lambda x: x[0])
        batch_sizes = [v[0] for v in values]
        times = [v[1] for v in values]
        ax.plot(
            batch_sizes,
            times,
            label=perf_result[len(json_prefix) : -5],
            marker="o",
            markersize=5,
            linewidth=2,
        )
    ax.set_ylabel("Time (ns)")
    ax.set_xlabel("Batch Size")
    ax.set_xscale("log")
    ax.legend(loc="upper right")
    ax.set_xticks(batch_sizes)
    ax.set_xticklabels(batch_sizes, rotation=45)

    # Pad y limit to make room for the legend
    y_min, y_max = ax.get_ylim()
    if sys.platform == "win32":
        ax.set_ylim(y_min, 30)

    platform_name = sys.platform
    if platform_name == "linux":
        platform_name = "Linux"
    elif platform_name == "darwin":
        platform_name = "MacOS"
    elif platform_name == "win32":
        platform_name = "Windows"

    ax.set_title(f"Performance Results for {platform_name}")
    ax.grid()
    fig.tight_layout()

    OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results")
    filename = "perf_results_" + sys.platform + ".png"
    OUT_FILE = os.path.join(OUT_DIR, filename)
    fig.savefig(OUT_FILE, dpi=300)
    # plt.show()


def plot_stage():
    json_prefix = "perf_results_stage_"
    perf_results = [
        f for f in os.listdir(".") if f.startswith(json_prefix) and f.endswith(".json")
    ]

    perf_results = sorted(perf_results)

    fig = plt.figure(figsize=(10, 6))
    ax = fig.subplots()

    for perf_result in perf_results:
        with open(perf_result, "r", encoding="utf8") as f:
            data = json.load(f)

        results = data["results"]

        values = []

        for result in results:
            name = result["name"]
            median = result["median(elapsed)"] * 1e9
            median /= result["batch"]
            block_size = name.split("_")[-1]
            block_size = int(block_size)

            values.append((block_size, median))

        values = sorted(values, key=lambda x: x[0])
        num_stage = [v[0] for v in values]
        times = [v[1] for v in values]
        ax.plot(
            num_stage,
            times,
            label=perf_result[len(json_prefix) : -5],
            marker="o",
            markersize=5,
            linewidth=2,
        )

    ax.set_ylabel("Time (ns)")
    ax.set_xlabel("Num Stages")
    ax.legend(loc="upper left")

    # Pad y limit to make room for the legend
    y_min, y_max = ax.get_ylim()
    if sys.platform == "win32":
        ax.set_ylim(y_min, 30)

    platform_name = sys.platform
    if platform_name == "linux":
        platform_name = "Linux"
    elif platform_name == "darwin":
        platform_name = "MacOS"
    elif platform_name == "win32":
        platform_name = "Windows"

    ax.set_title(f"Performance Results for {platform_name}")
    ax.grid()
    fig.tight_layout()

    OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results")
    filename = "perf_results_stage" + sys.platform + ".png"
    OUT_FILE = os.path.join(OUT_DIR, filename)
    fig.savefig(OUT_FILE, dpi=300)
    # plt.show()


if __name__ == "__main__":

    run_all()

    plot_all()
    plot_stage()

    plt.show()
    exit()

    # # Plot bar graph for batch size 1, 4, 8, 64 and 512
    # plot_batch = [1, 4, 8, 64, 512]
    # for batch in plot_batch:

    #     fig = plt.figure(figsize=(10, 6))
    #     ax = fig.subplots()
    #     for name, times in result_dict.items():
    #         batch_index = sorted(batch_sizes).index(batch)
    #         bar = ax.barh(
    #             name,
    #             times[batch_index],
    #             label=name,
    #         )
    #         ax.bar_label(bar, fmt="%.2f", label_type="edge", fontsize=10)
    #     ax.set_xlabel("Time (ns)")
    #     # ax.tick_params(axis="x", rotation=55)
    #     ax.set_title(f"Performance Results for Batch Size {batch}")
    #     fig.tight_layout()
    #     fig.savefig(f"perf_results_{batch}.png", dpi=300)

    # plt.show()
