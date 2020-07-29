#!/usr/bin/env python3

# Postprocessing for data aggregated from many individual annealing
# procedures. The input CSV file should have these headings, each with
# int64-compatible data:
#
#  - Hardware Size: Number of hardware nodes used in placement, total (int64).
#  - Application Size: Number of application nodes along one edge (int64).
#  - Number of Compute Workers: Degree of parallelism (int64).
#  - Runtime: Execution time in units of your choice - it's all
#        normalised (int64).
#  - Reliable Fitness Rate: Fraction of fitness computations that are
#        reliable (float64).
#
# The graph contains one line for each (hardware size, application size) pair,
# as well as a linear speedup "objective" line.

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

# Load data
if len(sys.argv) < 2:
    fPath = "aggregate_data.csv"
else:
    fPath = sys.argv[1]
df = pd.read_csv(fPath).sort_values(by="Number of Compute Workers")

# Draw speedup data
figure, axes = plt.subplots()
for appSize in df["Application Size"].unique():
    for hwSize in df["Hardware Size"].unique():
        subFrame = df[(df["Application Size"] == appSize) &
                      (df["Hardware Size"] == hwSize)]
        axes.plot(subFrame["Number of Compute Workers"],
                  subFrame["Runtime"][0] / subFrame["Runtime"],
                  "x", label="${}^2, {}$".format(appSize, hwSize))

# Draw linear speedup line
maxHoriz = df["Number of Compute Workers"].max()
axes.plot([1, maxHoriz], [1, maxHoriz], alpha=0.2, color="k", linestyle="--")

# Other stuff for speedup graph
axisBuffer = 1
axes.set_xlim(1 - axisBuffer, maxHoriz + axisBuffer)
axes.set_ylim(1 - axisBuffer, maxHoriz + axisBuffer)
axes.set_xlabel("Number of Compute Workers ($n$)")
axes.set_ylabel("Relative Speedup ($t_1/t_n$)")
axes.spines["top"].set_visible(False)
axes.spines["right"].set_visible(False)
axes.xaxis.set_ticks_position("bottom")
axes.yaxis.set_ticks_position("left")
labels = [2**power for power in [0, 2, 4, 5, 6]]
axes.xaxis.set_ticks(labels)
axes.xaxis.set_ticklabels(["1", "4", "16", "32\n(physical core limit)", "64"])
axes.yaxis.set_ticks(labels)
axes.set_title("Runtime Scaling with Number of Compute Workers\n"
               "for Varying Problem Sizes")
plt.legend(title="$|N_A|,|N_H|$")
figure.tight_layout()
figure.savefig("speedup.pdf")

# Draw fitness rate data
figure, axes = plt.subplots()
for appSize in df["Application Size"].unique():
    for hwSize in df["Hardware Size"].unique():
        subFrame = df[(df["Application Size"] == appSize) &
                      (df["Hardware Size"] == hwSize)]
        axes.plot(subFrame["Number of Compute Workers"],
                  subFrame["Reliable Fitness Rate"] * 100,
                  "x", label="${}^2, {}$".format(appSize, hwSize))

# Other stuff for fitness rate graph
horizAxisBuffer = 1
axes.set_xlim(1 - horizAxisBuffer, maxHoriz + horizAxisBuffer)
axes.set_ylim(0, 105)
axes.set_xlabel("Number of Compute Workers ($n$)")
axes.set_ylabel("Percentage of Fitness Calculations\nMade with Reliable Data")
axes.spines["top"].set_visible(False)
axes.spines["right"].set_visible(False)
axes.xaxis.set_ticks_position("bottom")
axes.yaxis.set_ticks_position("left")
axes.xaxis.set_ticks([2**power for power in [0, 2, 4, 5, 6]])
axes.xaxis.set_ticklabels(["1", "4", "16", "32\n(physical core limit)", "64"])
axes.yaxis.set_ticks([0, 50, 100])
axes.set_title("Reliable Fitness Computation Proportion againse Number of\n"
               "Compute Workers for Varying Problem Sizes")
plt.legend(title="$|N_A|,|N_H|$")
figure.tight_layout()
figure.savefig("error_rate.pdf")
