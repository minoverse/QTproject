#!/usr/bin/env python3

import argparse
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider


ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "A": ROOT / "dynamic_analysis/results/kmap_cut_A_c3eec1e507/K_map_skip_5000.dat",
    "B": ROOT / "dynamic_analysis/results/kmap_cut_B_8b68478d99/K_map_skip_5000.dat",
    "C": ROOT / "dynamic_analysis/results/kmap_cut_C_6e392b4adc/K_map_skip_5000.dat",
    "D": ROOT / "dynamic_analysis/results/kmap_cut_D_4329eb2081/K_map_skip_5000.dat",
    "E": ROOT / "dynamic_analysis/results/kmap_cut_E_739a4dd2da/K_map_skip_5000.dat",
}

parser = argparse.ArgumentParser()

parser.add_argument(
    "--cut",
    choices=["A", "B", "C", "D", "E"],
    required=True
)

parser.add_argument(
    "--size",
    type=float,
    default=40.0,
    help="Initial K-map point size"
)

args = parser.parse_args()

path = FILES[args.cut]

if not path.exists():
    raise SystemExit(f"File not found: {path}")

data = np.loadtxt(path)

if data.ndim != 2 or data.shape[1] < 3:
    raise SystemExit(
        f"Expected at least 3 columns: alpha2 nu K, got {data.shape}"
    )

alpha2 = data[:, 0]
nu = data[:, 1]
K = data[:, 2]

mask = (
    np.isfinite(alpha2)
    & np.isfinite(nu)
    & np.isfinite(K)
)

alpha2 = alpha2[mask]
nu = nu[mask]
K = K[mask]

print(f"Cut {args.cut}")
print(f"Data: {path}")
print(f"Valid K points: {len(K)}")


fig, ax = plt.subplots(figsize=(12, 7))
plt.subplots_adjust(bottom=0.19, right=0.88)

points = ax.scatter(
    alpha2,
    nu,
    c=K,
    s=args.size,
    marker="o",
    vmin=0,
    vmax=1
)

cbar = fig.colorbar(points, ax=ax)
cbar.set_label("K")

ax.set_xlabel(r"$\alpha_2$")
ax.set_ylabel(r"$\nu$")

ax.set_title(
    f"0-1 Chaos Test K-map — Cut {args.cut}"
)

ax.grid(True, alpha=0.25)

# Show actual parameter values
unique_alpha = np.unique(alpha2)
unique_nu = np.unique(nu)

ax.set_xticks(unique_alpha)
ax.set_yticks(unique_nu)

original_xlim = (
    alpha2.min() - 0.5,
    alpha2.max() + 0.5
)

original_ylim = (
    nu.min() - 0.05,
    nu.max() + 0.05
)

ax.set_xlim(original_xlim)
ax.set_ylim(original_ylim)


# -----------------------------------------
# Point-size slider
# -----------------------------------------

slider_ax = plt.axes(
    [0.18, 0.07, 0.60, 0.035]
)

size_slider = Slider(
    slider_ax,
    "point size",
    1,
    200,
    valinit=args.size,
    valstep=1
)


def change_size(value):
    points.set_sizes(
        np.full(len(K), float(value))
    )
    fig.canvas.draw_idle()


size_slider.on_changed(change_size)


# -----------------------------------------
# Mouse-wheel zoom
# -----------------------------------------

def scroll_zoom(event):

    if event.inaxes != ax:
        return

    if event.xdata is None or event.ydata is None:
        return

    factor = 1.3

    if event.button == "up":
        scale = 1.0 / factor
    elif event.button == "down":
        scale = factor
    else:
        return

    xmin, xmax = ax.get_xlim()
    ymin, ymax = ax.get_ylim()

    x = event.xdata
    y = event.ydata

    width = (xmax - xmin) * scale
    height = (ymax - ymin) * scale

    rx = (x - xmin) / (xmax - xmin)
    ry = (y - ymin) / (ymax - ymin)

    ax.set_xlim(
        x - rx * width,
        x + (1 - rx) * width
    )

    ax.set_ylim(
        y - ry * height,
        y + (1 - ry) * height
    )

    fig.canvas.draw_idle()


fig.canvas.mpl_connect(
    "scroll_event",
    scroll_zoom
)


# -----------------------------------------
# Reset zoom
# -----------------------------------------

def key_press(event):

    if event.key in ("home", "r"):
        ax.set_xlim(original_xlim)
        ax.set_ylim(original_ylim)
        fig.canvas.draw_idle()


fig.canvas.mpl_connect(
    "key_press_event",
    key_press
)


print("")
print("Controls:")
print("  Point-size slider : change K-map dot size")
print("  Mouse wheel       : zoom in/out")
print("  Toolbar Pan       : move around after zoom")
print("  Home / R          : reset view")

plt.show()
