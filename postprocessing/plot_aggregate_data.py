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

# Draw speedup data for the small problem
figure, axes = plt.subplots()
for synchronisation in df["Synchronisation"].unique():
    subFrame = df[(df["Synchronisation"] == synchronisation) &
                  (df["Problem Size"] == "Small")]
    serialTime = subFrame[subFrame["Number of Compute Workers"] == 0]\
        ["Runtime"].values[0]
    # Points
    axes.plot(subFrame["Number of Compute Workers"],
              serialTime / subFrame["Runtime"], "kx", label=None)
    # Line
    axes.plot(subFrame["Number of Compute Workers"],
              serialTime / subFrame["Runtime"],
              "k-" if synchronisation == "Asynchronous" else "k--",
              alpha=0.7,
              label=synchronisation)

# Draw linear speedup line
maxHoriz = df["Number of Compute Workers"].max()
axes.plot([1, maxHoriz], [1, maxHoriz], alpha=0.2, color="k", linestyle="--")

# Other stuff for speedup graph
axisBuffer = 1
axes.set_xlim(1 - axisBuffer, maxHoriz + axisBuffer)
axes.set_ylim(1 - axisBuffer, maxHoriz + axisBuffer)
axes.set_xlabel("Number of Compute Workers ($n$)")
axes.set_ylabel("Relative Speedup ($t_0/t_n$)")
axes.spines["top"].set_visible(False)
axes.spines["right"].set_visible(False)
axes.xaxis.set_ticks_position("bottom")
axes.yaxis.set_ticks_position("left")
labels = [2**power for power in [0, 2, 4, 5, 6]]
axes.xaxis.set_ticks(labels)
axes.xaxis.set_ticklabels(["1", "4", "16", "32\n(physical core limit)", "64"])
axes.yaxis.set_ticks(labels)
axes.set_title("Runtime Scaling with Number of Compute Workers\n"
               "for the Small problem.")
plt.legend(title="Algorithm")
figure.tight_layout()
figure.savefig("speedup.pdf")

# Draw fitness error rate data
figure, axes = plt.subplots()
for problemSize in df["Problem Size"].unique():
    subFrame = df[(df["Synchronisation"] == "Asynchronous") &
                  (df["Problem Size"] == problemSize)]
    # Points
    axes.plot(subFrame["Number of Compute Workers"],
              (1 - subFrame["Reliable Fitness Rate"]) * 100,
              "kx", label=None)
    # Line
    axes.plot(subFrame["Number of Compute Workers"],
              (1 - subFrame["Reliable Fitness Rate"]) * 100,
              "k-" if problemSize == "Large" else "k--",
              alpha=0.7,
              label=problemSize)

# Other stuff for fitness error rate graph
horizAxisBuffer = 1
axes.set_xlim(1 - horizAxisBuffer, maxHoriz + horizAxisBuffer)
axes.set_ylim(-2, 100)
axes.set_xlabel("Number of Compute Workers ($n$)")
axes.set_ylabel("Collision Rate (percentage of fitness\ncalculations made with"
                "unreliable data)")
axes.spines["top"].set_visible(False)
axes.spines["right"].set_visible(False)
axes.xaxis.set_ticks_position("bottom")
axes.yaxis.set_ticks_position("left")
axes.xaxis.set_ticks([2**power for power in [0, 2, 4, 5, 6]])
axes.xaxis.set_ticklabels(["1", "4", "16", "32\n(physical core limit)", "64"])
axes.yaxis.set_ticks([0, 50, 100])
axes.set_title("Collision Rate against Number of\n"
               "Compute Workers (Asynchronous Algorithm)")
plt.legend(title="Problem Size")
figure.tight_layout()
figure.savefig("error_rate.pdf")
