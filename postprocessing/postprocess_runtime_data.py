#!/usr/bin/env python3

# Postprocessing for runtime data. Input CSV file should have these headings,
# each with int64-compatible data:
#
#  - Hardware Size: Number of hardware nodes used in placement, total.
#  - Application Size: Number of application nodes along one edge.
#  - Number of Compute Workers: Degree of parallelism.
#  - Runtime: Execution time in units of your choice - it's all normalised.
#
# The graph contains one line for each (hardware size, application size) pair,
# as well as a linear speedup "objective" line.

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

# Load data
if len(sys.argv) < 2:
    fPath = "fitnesses.csv"
else:
    fPath = sys.argv[1]
df = pd.read_csv(fPath).sort_values(by="Number of Compute Workers")

figure, axes = plt.subplots()

# Draw data
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

# Other stuff
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
axes.yaxis.set_ticks(labels)
axes.set_title("Runtime Scaling with Number of Compute Workers\n"
               "for Varying Problem Size")
plt.legend(title="$|N_A|,|N_H|$")
figure.tight_layout()
figure.savefig("runtimes.pdf")
