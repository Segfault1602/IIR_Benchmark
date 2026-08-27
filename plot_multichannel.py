import json
import os
import re
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, NullFormatter, ScalarFormatter


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


# The bank is the subject of the benchmark, so it gets a reserved colour and a heavier line.
# Everything else draws from a palette with that colour removed. Leaving the bank on the default
# cycle meant its colour shifted whenever the backend set changed (a backend being removed, or IPP
# and the SSE/AVX variants existing only on x86), which made the per-platform plots inconsistent.
BANK_COLOUR = "#d62728"
OTHER_COLOURS = [
    "#1f77b4", "#ff7f0e", "#2ca02c", "#9467bd", "#8c564b",
    "#e377c2", "#7f7f7f", "#bcbd22", "#17becf",
]


def backend_colours(backend_names):
    """Assigns a stable colour per backend, reserving BANK_COLOUR for the bank variants."""
    colours = {}
    index = 0
    for name in backend_names:
        if name.startswith("SimdBiquadBank"):
            colours[name] = BANK_COLOUR
        else:
            colours[name] = OTHER_COLOURS[index % len(OTHER_COLOURS)]
            index += 1
    return colours


def plot_multichannel(results_by_combo, output_path):
    keys = sorted(results_by_combo)
    # Rows share a y-axis: within a row only the block size changes, so a common scale makes the
    # four panels directly comparable and gives every panel the same tick values. Without this,
    # each panel picked its own range and its own irregular ticks, which made the axis hard to
    # read and left it ambiguous whether the scale was linear or logarithmic.
    fig, axes = plt.subplots(2, 4, figsize=(18, 10), squeeze=False, sharey="row")
    fig.suptitle(f"Multi-channel biquad benchmark ({platform_name(sys.platform)})", fontsize=14)

    for index, key in enumerate(keys):
        stages, block = key
        row, column = index // 4, index % 4
        ax = axes[row, column]
        backend_data = results_by_combo[key]
        channel_counts = sorted({channel for backend in backend_data.values() for channel in backend})
        positions = list(range(len(channel_counts)))

        colours = backend_colours(sorted(backend_data))
        for backend, values_by_channel in sorted(backend_data.items()):
            values = [values_by_channel[channel] for channel in channel_counts]
            is_bank = backend.startswith("SimdBiquadBank")
            # The bank is the subject of the benchmark, so it keeps a fixed colour and a heavier
            # line. Leaving it on the default cycle meant its colour changed whenever the set of
            # backends changed (removing one, or IPP and the SSE/AVX variants only existing on
            # x86), which made the per-platform plots inconsistent with each other.
            ax.plot(
                positions,
                values,
                label=backend,
                marker="o",
                linewidth=2.5 if is_bank else 1.5,
                color=colours[backend],
                zorder=3 if is_bank else 2)

        ax.set_title(f"stages={stages}, block={block}")

        ax.grid(True, which="major", alpha=0.35)

        ax.set_xlabel("Channels")
        if column == 0:
            ax.set_ylabel("ns/sample/channel")
        ax.set_xticks(positions)
        ax.set_xticklabels([str(c) for c in channel_counts])
        ax.tick_params(axis="x", labelbottom=True, labelsize=9)
        ax.tick_params(axis="y", which="major", labelsize=9)

    # Rows share a y-axis, so the limit has to come from the whole row. Setting it per-axis
    # clipped the tallest series: the row limit ended up driven by whichever panel was drawn
    # first, hiding SimdBiquadBank's single-channel point in the other panels.
    for row in range(axes.shape[0]):
        row_keys = [k for i, k in enumerate(keys) if i // 4 == row]
        if not row_keys:
            continue
        row_max = max(
            value
            for key in row_keys
            for backend in results_by_combo[key].values()
            for value in backend.values()
        )
        axes[row, 0].set_ylim(0, row_max * 1.05)

    # Backend count varies by platform and build (for example IPP only on x86, and separate
    # SSE/AVX compilations of the bank), so the legend width is derived rather than hard-coded.
    handles, labels = axes[0, 0].get_legend_handles_labels()
    ncol = min(len(labels), 7) if labels else 1
    fig.legend(handles, labels, loc="lower center", ncol=ncol, frameon=True)
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
