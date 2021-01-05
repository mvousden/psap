#!/usr/bin/env python3

# Postprocessing for data aggregated from many individual annealing
# procedures. The input CSV file should have these headings:
#
#  - Problem Size: String
#  - Synchronisation: String
#  - Number of Compute Workers: Degree of parallelism (int64).
#  - Runtime: Execution time in units of your choice - it's all
#        normalised (int64).
#  - Reliable Fitness Rate: Fraction of fitness computations that are
#        reliable (float64).

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

matplotlib.rc("axes", linewidth=1)
matplotlib.rc("figure", figsize=(4.0, 3.3))
matplotlib.rc("font", family="serif", size=10)
matplotlib.rc("legend", frameon=False)
matplotlib.rc("xtick.major", width=1)
matplotlib.rc("ytick.major", width=1)

# Load data
if len(sys.argv) < 2:
    fPath = "aggregate_data.csv"
else:
    fPath = sys.argv[1]
df = pd.read_csv(fPath).sort_values(by="Number of Compute Workers")

# Draw speedup data for the small problem
figure, axes = plt.subplots()
for synchronisation in df["Synchronisation"].unique():
    subFrame = df[(df["Synchronisation"] == synchronisation) &
                  (df["Problem Size"] == "Small")]
    serialTime = subFrame[subFrame["Number of Compute Workers"] == 0]\
        ["Runtime"].values[0]

    # Points (the 1: slice removes n=0)
    axes.plot(subFrame["Number of Compute Workers"][1:],
              serialTime / subFrame["Runtime"][1:],
              "k." if synchronisation == "Asynchronous" else "k1",
              label=synchronisation + " Annealer")

# Draw linear speedup line
maxHoriz = df["Number of Compute Workers"].max()
# axes.plot([1, maxHoriz], [1, maxHoriz], alpha=0.2, color="k", linestyle="--")

# Other stuff for speedup graph
axisBuffer = 1
axes.set_xlim(1 - axisBuffer, maxHoriz + axisBuffer)
axes.set_ylim(0, 10 + axisBuffer)
axes.set_xlabel("Number of Compute Workers ($n$)")
axes.set_ylabel("Serial-Relative Speedup ($t_0/t_n$)")
axes.spines["top"].set_visible(False)
axes.spines["right"].set_visible(False)
axes.xaxis.set_ticks_position("bottom")
axes.yaxis.set_ticks_position("left")
labels = [2**power for power in [0, 4, 5, 6]]
axes.xaxis.set_ticks(labels)
axes.xaxis.set_ticklabels(["1", "16", "32", "64"])
axes.yaxis.set_ticks([0, 2, 4, 6, 8, 10])
axes.set_title("Runtime Scaling (Small Problem)")
plt.legend(handletextpad=0)
figure.tight_layout()
figure.savefig("speedup.pdf")

# Draw fitness error rate data
figure, axes = plt.subplots()
for problemSize in df["Problem Size"].unique():
    subFrame = df[(df["Synchronisation"] == "Asynchronous") &
                  (df["Problem Size"] == problemSize) &
                  (df["Number of Compute Workers"] != 0)]
    # Points
    axes.plot(subFrame["Number of Compute Workers"],
              (1 - subFrame["Reliable Fitness Rate"]) * 100,
              "kx" if problemSize == "Large" else "k+",
              label=problemSize + " Problem")

# Other stuff for fitness error rate graph
horizAxisBuffer = 1
axes.set_xlim(1 - horizAxisBuffer, maxHoriz + horizAxisBuffer)
axes.set_ylim(-2, 60)
axes.set_xlabel("Number of Compute Workers")
axes.set_ylabel("Collision Rate (percentage of\nunreliable fitness "
                "calculations)")
axes.spines["top"].set_visible(False)
axes.spines["right"].set_visible(False)
axes.xaxis.set_ticks_position("bottom")
axes.yaxis.set_ticks_position("left")
axes.xaxis.set_ticks([2**power for power in [0, 4, 5, 6]])
axes.xaxis.set_ticklabels(["1", "16", "32", "64"])
axes.yaxis.set_ticks([0, 50])
axes.set_title("Asynchronous Annealing Collision Rate")
plt.legend(handletextpad=0)
figure.tight_layout()
figure.savefig("error_rate.pdf")
