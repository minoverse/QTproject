#!/usr/bin/env python3

import argparse
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider


parser = argparse.ArgumentParser()

parser.add_argument(
    "--node",
    type=int,
    choices=range(1, 6),
    default=1,
    help="Node to plot: 1..5"
)

parser.add_argument(
    "--shift",
    type=float,
    default=0.0,
    help="Initial horizontal shift"
)

parser.add_argument(
    "--step",
    type=float,
    default=0.05,
    help="Horizontal movement step"
)

parser.add_argument(
    "--data",
    default=(
        "extra_bif_test/fast_build_opt/fast_results/"
        "run_20261103_223713/nu_t500_10000/nu.dat"
    )
)

args = parser.parse_args()


# --------------------------------------------------
# Load final Step-1 bifurcation data
# --------------------------------------------------

ROOT = Path(__file__).resolve().parents[1]

data_path = Path(args.data)

if not data_path.is_absolute():
    data_path = ROOT / data_path

if not data_path.exists():
    raise SystemExit(f"Data file not found: {data_path}")


data = np.loadtxt(data_path)

# nu.dat format:
# column 0 = nu
# column 1 = y1
# column 2 = y2
# column 3 = y3
# column 4 = y4
# column 5 = y5

if data.ndim != 2 or data.shape[1] != 6:
    raise SystemExit(
        f"Expected 6 columns, got shape={data.shape}"
    )


nu = data[:, 0]
y = data[:, args.node]

mask = np.isfinite(nu) & np.isfinite(y)

nu = nu[mask]
y = y[mask]

unique_nu = np.unique(nu)


print("Using final Step-1 bifurcation data:")
print(data_path)
print(f"node = y{args.node}")
print("nu values:")
print(unique_nu)


# --------------------------------------------------
# Plot
# --------------------------------------------------

shift = args.shift

fig, ax = plt.subplots(figsize=(12, 7))

plt.subplots_adjust(
    bottom=0.20
)


# Small scatter points.
# No connecting lines.
points = ax.scatter(
    nu + shift,
    y,
    s=1.0,
    marker=".",
    alpha=0.30,
    linewidths=0
)


# Original full-view limits
x_margin = 0.05

original_xlim = (
    unique_nu.min() - x_margin,
    unique_nu.max() + x_margin
)

y_range = y.max() - y.min()

if y_range == 0:
    y_range = 1.0

original_ylim = (
    y.min() - 0.05 * y_range,
    y.max() + 0.05 * y_range
)


ax.set_xlim(original_xlim)
ax.set_ylim(original_ylim)


# Show ALL actual nu values
ax.set_xticks(unique_nu)

ax.set_xticklabels(
    [f"{x:g}" for x in unique_nu],
    fontsize=9
)


ax.set_xlabel(r"$\nu$")
ax.set_ylabel(f"y{args.node}")

ax.grid(
    True,
    alpha=0.25
)


def update_title():

    ax.set_title(
        f"nu bifurcation, alpha2=-10, "
        f"y{args.node}, t=500..10000, "
        f"x-shift={shift:+.2f}"
    )


update_title()


# --------------------------------------------------
# Horizontal movement slider
# --------------------------------------------------

slider_ax = plt.axes(
    [0.18, 0.07, 0.65, 0.035]
)

slider = Slider(
    slider_ax,
    "x shift",
    -0.5,
    0.5,
    valinit=shift,
    valstep=args.step
)


def move_graph(value):

    global shift

    shift = float(value)

    # Move ALL bifurcation points horizontally
    points.set_offsets(
        np.column_stack(
            (nu + shift, y)
        )
    )

    update_title()

    fig.canvas.draw_idle()


slider.on_changed(move_graph)


# --------------------------------------------------
# Keyboard movement
# --------------------------------------------------

def key_press(event):

    if event.key in ("right", "d"):

        slider.set_val(
            min(
                0.5,
                shift + args.step
            )
        )

    elif event.key in ("left", "a"):

        slider.set_val(
            max(
                -0.5,
                shift - args.step
            )
        )

    # reset horizontal shift
    elif event.key == "r":

        slider.set_val(0.0)

    # reset zoom/view
    elif event.key == "home":

        ax.set_xlim(original_xlim)
        ax.set_ylim(original_ylim)

        fig.canvas.draw_idle()


fig.canvas.mpl_connect(
    "key_press_event",
    key_press
)


# --------------------------------------------------
# Mouse wheel zoom
# --------------------------------------------------

def scroll_zoom(event):

    if event.inaxes != ax:
        return

    if event.xdata is None or event.ydata is None:
        return


    zoom_factor = 1.30


    # Wheel up = zoom in
    if event.button == "up":

        scale = 1.0 / zoom_factor

    # Wheel down = zoom out
    elif event.button == "down":

        scale = zoom_factor

    else:

        return


    xlim = ax.get_xlim()
    ylim = ax.get_ylim()


    mouse_x = event.xdata
    mouse_y = event.ydata


    old_width = xlim[1] - xlim[0]
    old_height = ylim[1] - ylim[0]


    new_width = old_width * scale
    new_height = old_height * scale


    # Keep mouse position as zoom center
    relative_x = (
        mouse_x - xlim[0]
    ) / old_width

    relative_y = (
        mouse_y - ylim[0]
    ) / old_height


    new_xmin = (
        mouse_x
        - relative_x * new_width
    )

    new_xmax = (
        new_xmin
        + new_width
    )


    new_ymin = (
        mouse_y
        - relative_y * new_height
    )

    new_ymax = (
        new_ymin
        + new_height
    )


    ax.set_xlim(
        new_xmin,
        new_xmax
    )

    ax.set_ylim(
        new_ymin,
        new_ymax
    )


    fig.canvas.draw_idle()


fig.canvas.mpl_connect(
    "scroll_event",
    scroll_zoom
)


# --------------------------------------------------
# Instructions
# --------------------------------------------------

print("")
print("Controls:")
print("  Right / D   : move whole graph right")
print("  Left  / A   : move whole graph left")
print("  R           : reset horizontal shift")
print("  Home        : reset zoom/view")
print("  Mouse wheel : zoom in/out around cursor")
print("  Toolbar Pan : drag/pan enlarged graph")
print("  Slider      : horizontal graph movement")


plt.show()
