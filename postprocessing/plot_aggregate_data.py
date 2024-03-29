#!/usr/bin/env python3

# Postprocessing for data aggregated from many individual annealing
# procedures. The input CSV file should have these headings:
#
#  - Problem Size: String, either "Small" or "Large.
#  - Synchronisation: String, either "Asynchronous" or "Synchronous".
#  - Number of Compute Workers: Degree of parallelism (int64).
#  - Repeat: Repeat number, beginning from 1 (int64).
#  - Runtime: Execution time in units of your choice - it's all
#        normalised (int64).
#  - Unreliable Fitness Rate: Fraction of fitness computations that are
#        unreliable (float64).

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

matplotlib.rc("axes", linewidth=1)
matplotlib.rc("figure", figsize=(2.7, 2.7), dpi=10)
matplotlib.rc("font", family="serif", size=7)
matplotlib.rc("legend", frameon=False)
matplotlib.rc("xtick.major", width=1)
matplotlib.rc("ytick.major", width=1)

# Load data
if len(sys.argv) < 2:
    fPath = "aggregate_data.csv"
else:
    fPath = sys.argv[1]
df = pd.read_csv(fPath).sort_values(by="Number of Compute Workers")

# Draw speedup data for the small and large problems separately
for problem in df["Problem Size"].unique():
    figure, axes = plt.subplots()
    for synchronisation in df["Synchronisation"].unique():

        # Get data for this problem size, and for this synchronisation.
        subFrame = df[(df["Synchronisation"] == synchronisation) &
                      (df["Problem Size"] == problem)]

        # Get mean serial time
        serialFrame = subFrame[subFrame["Number of Compute Workers"] == 0]
        serialTime = serialFrame["Runtime"].values.mean()

        # Make the pandas people unhappy
        parallelFrame = subFrame[subFrame["Number of Compute Workers"] > 0]
        workers = pd.unique(parallelFrame["Number of Compute Workers"].values)
        means = []
        maxes = []
        mins = []
        for worker in workers:
            workerFrame = parallelFrame[
                parallelFrame["Number of Compute Workers"] == worker]["Runtime"]
            means.append(serialTime / workerFrame.mean())
            maxes.append(serialTime / workerFrame.max())
            mins.append(serialTime / workerFrame.min())

        # Points
        axes.plot(workers, means,
                  "k." if synchronisation == "Asynchronous" else "k1",
                  label=synchronisation + " Annealer")

        # Range, plotted as a line with some flanges
        flangeLen = 1
        flangeWidth = 0.5
        # Different whisker styles for visibility
        linestyle = "k--" if synchronisation == "Asynchronous" else "k:"
        for index in range(len(workers)):
            axes.plot([workers[index], workers[index]],
                      [mins[index], maxes[index]], linestyle, label=None,
                      linewidth=flangeWidth)
            axes.plot([workers[index] - flangeLen, workers[index] + flangeLen],
                      [mins[index], mins[index]], linestyle, label=None,
                      linewidth=flangeWidth)
            axes.plot([workers[index] - flangeLen, workers[index] + flangeLen],
                      [maxes[index], maxes[index]], linestyle, label=None,
                      linewidth=flangeWidth)

    # Other stuff for speedup graph
    axisBuffer = 1
    maxHoriz = df["Number of Compute Workers"].max()
    axes.set_xlim(1 - axisBuffer, maxHoriz + axisBuffer)
    axes.set_ylim(0, 8)
    axes.set_xlabel("Number of Compute Workers")
    if problem == "Large":
        axes.set_ylabel("Serial-Relative Speedup ($t_0/t_n$)")
    axes.spines["top"].set_visible(False)
    axes.spines["right"].set_visible(False)
    axes.xaxis.set_ticks_position("bottom")
    axes.yaxis.set_ticks_position("left")
    labels = [2**power for power in [0, 4, 5, 6]]
    axes.xaxis.set_ticks(labels)
    axes.xaxis.set_ticklabels(["1", "16", "32", "64"])
    axes.yaxis.set_ticks([0, 1, 2, 4, 6, 8])
    axes.set_title("Runtime Scaling ({} Problem)".format(problem))
    if problem == "Large":
        plt.legend(loc=4, handletextpad=0)
    figure.tight_layout()
    figure.savefig("speedup_{}.pdf".format(problem.lower()))
    plt.close()

# Collision rate data from MC experiments (sorry):
nw = [1] + list(range(2, 65, 2))
smallProbs = np.array([0.0,
              0.00982,
              0.03159,
              .052128957420851586,
              .07211,
              .09074,
              .10806135509159268,
              .126707465850683,
              .14564,
              .16300695944324453,
              .18095,
              .1951965764108307,
              .21164306855451565,
              .23096918677890865,
              .2489701647736362,
              .262997400519896,
              .27977,
              .29164833846522975,
              .30478561715062796,
              .3198488241881299,
              .33376,
              .3472230555388892,
              .36214654241491023,
              .3766249350025999,
              .3889855246321177,
              .40163,
              .4124520230289461,
              .42360611151107913,
              .43787993920972645,
              .44990504747626187,
              .4605278944211158,
              .4697418154910705,
              .48380518234165065]) * 100

largeProbs = np.array([0.0,
              0.00514,
              0.01649,
              0.025369492610147797,
              0.03539,
              0.04631,
              0.05629549636029118,
              0.06436871262574749,
              0.0755,
              0.08567314614830814,
              0.09602,
              0.10628724553053634,
              0.11447084233261338,
              0.12224310651656635,
              0.13309870420732683,
              0.14034193161367725,
              0.15084,
              0.16032510896948854,
              0.16655667546596273,
              0.1750919852823548,
              0.18523,
              0.19281614367712646,
              0.1977462704475463,
              0.20977160913563458,
              0.21718050223928342,
              0.22727,
              0.23267831440908365,
              0.24246060315174786,
              0.24979003359462487,
              0.2554022988505747,
              0.2640071985602879,
              0.2722736635801852,
              0.2778510876519514]) * 100

# Draw fitness collision rate data
figure, axes = plt.subplots()
for problemSize in df["Problem Size"].unique():
    subFrame = df[(df["Synchronisation"] == "Asynchronous") &
                  (df["Problem Size"] == problemSize) &
                  (df["Number of Compute Workers"] != 0)]

    # Make the pandas people unhappy, again
    workers = pd.unique(subFrame["Number of Compute Workers"].values)
    means = []
    maxes = []
    mins = []
    for worker in workers:
        workerFrame = (subFrame[
            subFrame["Number of Compute Workers"] == worker]
            ["Unreliable Fitness Rate"])
        sf = 75 if problemSize == "Small" else 50
        means.append(workerFrame.mean() * sf)
        maxes.append(workerFrame.max() * sf)
        mins.append(workerFrame.min() * sf)

    # Points
    axes.plot(workers, means,
              "kx" if problemSize == "Large" else "k+",
              label=problemSize + " Problem")

    # Range, plotted as a line with some flanges
    flangeLen = 1
    flangeWidth = 0.5
    linestyle = "k-"
    for index in range(len(workers)):
        axes.plot([workers[index], workers[index]],
                  [mins[index], maxes[index]], linestyle, label=None,
                  linewidth=flangeWidth)
        axes.plot([workers[index] - flangeLen, workers[index] + flangeLen],
                  [mins[index], mins[index]], linestyle, label=None,
                  linewidth=flangeWidth)
        axes.plot([workers[index] - flangeLen, workers[index] + flangeLen],
                  [maxes[index], maxes[index]], linestyle, label=None,
                  linewidth=flangeWidth)

# Draw model data.
axes.plot(nw, smallProbs, "k--", alpha=0.5, label="Monte Carlo")
axes.plot(nw, largeProbs, "k--", alpha=0.5)

# Other stuff for fitness collision rate graph
horizAxisBuffer = 1
axes.set_xlim(1 - horizAxisBuffer, maxHoriz + horizAxisBuffer)
axes.set_ylim(-1.3, 50)
axes.set_xlabel("Number of Compute Workers")
axes.set_ylabel("Collision Rate (percentage of\nunreliable fitness "
                "calculations)")
axes.spines["top"].set_visible(False)
axes.spines["right"].set_visible(False)
axes.xaxis.set_ticks_position("bottom")
axes.yaxis.set_ticks_position("left")
axes.xaxis.set_ticks([2**power for power in [0, 4, 5, 6]])
axes.xaxis.set_ticklabels(["1", "16", "32", "64"])
axes.yaxis.set_ticks([0, 10, 20, 30, 40, 50])
axes.set_title(r"Asynchronous Annealing Collision Rate$\quad$")
plt.legend(handletextpad=0)
figure.tight_layout()
figure.savefig("collision_rate.pdf")
