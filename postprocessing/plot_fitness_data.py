#!/usr/bin/env python3

# Postprocessing for fitness data aggregated from many individual annealing
# procedures. The input CSV file should have these headings:
#
#  - Number of Compute Workers: 0 (if serial), 64 (if parallel, with 64 workers).
#  - Iteration: (int64).
#  - Clustering Fitness: (float64).
#  - Locality Fitness: (float64).

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

matplotlib.rc("axes", linewidth=1)
matplotlib.rc("figure", dpi=400, figsize=(4.8, 3.6))
matplotlib.rc("font", size=12)
matplotlib.rc("legend", frameon=False)
matplotlib.rc("xtick.major", width=1)
matplotlib.rc("ytick.major", width=1)

# Colors
clusterColour = "#AAAAFF"
localityColour = "#88FF88"
edgeColour = "#000044"

# Load data
if len(sys.argv) < 2:
    fPath = "fitness_data.csv"
else:
    fPath = sys.argv[1]
df = pd.read_csv(fPath).sort_values(
    by=["Number of Compute Workers", "Iteration"])

# Draw fitness data for the serial solver using lines.
figure, axes = plt.subplots()
subFrame = df[df["Number of Compute Workers"] == 0]

#axes.plot(subFrame["Iteration"], subFrame["Clustering Fitness"], '--',
#          color=edgeColour)
axes.fill_between(subFrame["Iteration"], 0, subFrame["Clustering Fitness"],
                  color=clusterColour, label="Serial: Clustering Fitness")

axes.plot(subFrame["Iteration"],
          subFrame["Clustering Fitness"] + subFrame["Locality Fitness"],
          color=edgeColour, label="Serial: Total Fitness")
axes.fill_between(subFrame["Iteration"], subFrame["Clustering Fitness"],
                  subFrame["Clustering Fitness"] +
                      subFrame["Locality Fitness"],
                  color=localityColour, label="Serial: Locality Fitness")

# Draw fitness data for the asynchronous parallel solver using points.
subFrame = df[df["Number of Compute Workers"] == 64]
axes.plot(subFrame["Iteration"],
          subFrame["Clustering Fitness"] + subFrame["Locality Fitness"], 'k.',
          label="Asynchronous Parallel: Total Fitness")

# Draw fitness data for the synchronous parallel solver using points.
subFrame = df[df["Number of Compute Workers"] == 0]
axes.plot(subFrame["Iteration"][::25],
          subFrame["Clustering Fitness"][::25] +
          subFrame["Locality Fitness"][::25], 'k1',
          label="Synchronous Parallel: Total Fitness")


# Other stuff
axes.ticklabel_format(axis="x", scilimits=(0, 0), style="sci")
axes.set_xlim(0, 5e9)
axes.set_ylim(-5e10, 0)
axes.set_xlabel("Iteration")
axes.set_ylabel("Fitness")
axes.set_title("Relaxation (Large Problem)")
axes.spines["right"].set_visible(False)
axes.spines["top"].set_visible(False)
axes.yaxis.set_ticks_position("left")
axes.xaxis.set_ticks_position("bottom")
plt.legend()
figure.tight_layout()
figure.savefig("fitness.pdf")
