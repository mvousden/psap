#!/usr/bin/env python3

# Postprocessing for fitness data aggregated from many individual annealing
# procedures. The input CSV file should have these headings:
#
#  - Number of Compute Workers: 0 (if serial), 64 (if parallel, with 64 workers).
#  - Iteration: (int64).
#  - Repeat: Repeat number, beginning from 1. Serial data should be 1 (int64).
#  - Clustering Fitness: (float64).
#  - Locality Fitness: (float64).

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

matplotlib.rc("axes", linewidth=1)
matplotlib.rc("figure", figsize=(2.7, 2.7), dpi=1)
matplotlib.rc("font", family="serif", size=7)
matplotlib.rc("legend", frameon=False)
matplotlib.rc("xtick.major", width=1)
matplotlib.rc("ytick.major", width=1)

# Colors
clusterColour = "#AAAAFF"
localityColour = "#88FF88"
edgeColour = "#000044"

# Load data
if len(sys.argv) < 2:
    fPath = "fitness_data_large.csv"
else:
    fPath = sys.argv[1]
problem = "Small" if "small" in fPath else "Large"

df = pd.read_csv(fPath).sort_values(
    by=["Number of Compute Workers", "Repeat", "Iteration"])

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
iterations = []
fitnesses = []
for iteration in set(subFrame["Iteration"]):
    iterations.append(iteration)
    subFrameAvg = subFrame[subFrame["Iteration"] == iteration].mean()
    fitnesses.append(subFrameAvg["Clustering Fitness"] +
                     subFrameAvg["Locality Fitness"])
axes.plot(iterations, fitnesses, 'k.',
          label="Asynchronous Parallel: Total Fitness")

# Draw fitness data for the synchronous parallel solver using points.
subFrame = df[df["Number of Compute Workers"] == 0]
spacing = 25 if problem == "Large" else 5
axes.plot(subFrame["Iteration"][::spacing],
          subFrame["Clustering Fitness"][::spacing] +
          subFrame["Locality Fitness"][::spacing], 'k1',
          label="Synchronous Parallel: Total Fitness")

# Other stuff
axes.set_xlim(0, 5.1e9)
axes.set_ylim(-5e10 if problem == "Large" else -2e10, 0)
axes.set_xlabel(r"Iteration $\left(\times10^9\right)$")
if problem == "Large":
    axes.set_ylabel(r"Fitness $\left(\times10^{10}\right)$")
axes.set_title("Relaxation ({} Problem)".format(problem))
axes.spines["right"].set_visible(False)
axes.spines["top"].set_visible(False)
axes.yaxis.set_ticks_position("left")
axes.xaxis.set_ticks_position("bottom")
axes.xaxis.set_ticks([0, 1e9, 2e9, 3e9, 4e9, 5e9])
axes.xaxis.set_ticklabels(["0", "1", "2", "3", "4", "5"])
if problem == "Large":
    axes.yaxis.set_ticks([0, -1e10, -2e10, -3e10, -4e10, -5e10])
    axes.yaxis.set_ticklabels(["0", "-1", "-2", "-3", "-4", "-5"])
else:
    axes.yaxis.set_ticks([0, -5e9, -1e10, -1.5e10, -2e10])
    axes.yaxis.set_ticklabels(["0", "-0.5", "-1", "-1.5", "-2"])

if problem == "Large":
    plt.legend()
figure.tight_layout()
figure.savefig("".join(fPath.split(".")[:-1]) + ".pdf",
               bbox_inches="tight", pad_inches=1e-2)
plt.close()
