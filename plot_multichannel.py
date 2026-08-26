import json
import os
import re
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, ScalarFormatter


def platform_name(platform):
    names = {
        "linux": "Linux",
        "darwin": "MacOS",
        "win32": "Windows",
    }
    return names.get(platform, platform)


def parse_test_name(name):
    if "_ch" not in name:
        raise ValueError(f"Unexpected benchmark name: {name!r}")
    backend, channel_str = name.rsplit("_ch", 1)
    return backend, int(channel_str)


def parse_multichannel_jsons(base_dir):
    files = [
        f
        for f in sorted(os.listdir(base_dir))
        if f.startswith("perf_results_mc_st") and f.endswith(".json")
    ]
    if not files:
        raise FileNotFoundError("No multichannel perf_results_mc_*.json files found in the project root.")

    results_by_combo = {}
    for filename in files:
        match = re.fullmatch(r"perf_results_mc_st(\d+)_blk(\d+)\.json", filename)
        if match is None:
            continue

        stages = int(match.group(1))
        block = int(match.group(2))
        with open(os.path.join(base_dir, filename), "r", encoding="utf8") as f:
            data = json.load(f)

        for result in data.get("results", []):
            name = result.get("name")
            if not name:
                continue
            backend, channels = parse_test_name(name)
            elapsed = float(result["median(elapsed)"])
            batch = int(result["batch"])
            ns_per_sample = (elapsed * 1e9) / batch
            results_by_combo.setdefault((stages, block), {}).setdefault(backend, {})[channels] = ns_per_sample

    return results_by_combo


def print_ascii_tables(results_by_combo):
    for (stages, block), channel_map in sorted(results_by_combo.items()):
        backend_names = sorted(channel_map)

        print(f"=== stages={stages} block={block} ===")
        print(f"{'ch':>3}  " + "  ".join(f"{backend:<18}" for backend in backend_names))

        for channel in sorted({channel for backend_data in channel_map.values() for channel in backend_data}):
            row = [f"{channel:>3}"]
            for backend in backend_names:
                value = channel_map[backend].get(channel)
                if value is None:
                    row.append("-".rjust(18))
                else:
                    row.append(f"{value:10.3f}".rjust(18))
            print("  ".join(row))
        print()


def plot_multichannel(results_by_combo, output_path):
    keys = sorted(results_by_combo)
    fig, axes = plt.subplots(2, 4, figsize=(18, 10), squeeze=False)
    fig.suptitle(f"Multi-channel biquad benchmark ({platform_name(sys.platform)})", fontsize=14)

    for index, key in enumerate(keys):
        stages, block = key
        ax = axes[index // 4, index % 4]
        backend_data = results_by_combo[key]
        channel_counts = sorted({channel for backend in backend_data.values() for channel in backend})
        positions = list(range(len(channel_counts)))

        for backend, values_by_channel in sorted(backend_data.items()):
            values = [values_by_channel[channel] for channel in channel_counts]
            linewidth = 2.5 if backend.startswith("SimdBiquadBank") else 1.5
            ax.plot(positions, values, label=backend, marker="o", linewidth=linewidth)

        ax.set_title(f"stages={stages}, block={block}")
        ax.grid(True, which="both", alpha=0.25)
        ax.set_yscale("log")
        ax.yaxis.set_major_locator(LogLocator(base=10.0, subs=(1.0,), numticks=12))
        ax.yaxis.set_minor_locator(LogLocator(base=10.0, subs=(0.2, 0.3, 0.5, 0.7, 2.0, 3.0, 5.0, 7.0), numticks=12))
        major_fmt = ScalarFormatter()
        major_fmt.set_scientific(False)
        minor_fmt = ScalarFormatter()
        minor_fmt.set_scientific(False)
        ax.yaxis.set_major_formatter(major_fmt)
        ax.yaxis.set_minor_formatter(minor_fmt)
        ax.set_xlabel("Channels")
        ax.set_ylabel("ns/sample/channel")
        ax.set_xticks(positions)
        ax.set_xticklabels([str(c) for c in channel_counts])
        ax.tick_params(axis="x", labelbottom=True, labelsize=9)
        ax.tick_params(axis="y", which="major", labelsize=8)
        ax.tick_params(axis="y", which="minor", labelsize=7)

    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=7, frameon=True)
    fig.tight_layout(rect=(0, 0.08, 1, 0.97))
    fig.savefig(output_path, dpi=300)
    plt.close(fig)


def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    results_by_combo = parse_multichannel_jsons(base_dir)
    print_ascii_tables(results_by_combo)

    out_dir = os.path.join(base_dir, "results")
    os.makedirs(out_dir, exist_ok=True)
    output_path = os.path.join(out_dir, f"multichannel_{sys.platform}.png")
    plot_multichannel(results_by_combo, output_path)
    print(f"Saved plot to {output_path}")


if __name__ == "__main__":
    main()
