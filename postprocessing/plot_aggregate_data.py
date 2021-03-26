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
    # Points
    axes.plot(subFrame["Number of Compute Workers"],
              (1 - subFrame["Reliable Fitness Rate"]) * \
               (100 if problemSize == "Large" else 85),
              "kx" if problemSize == "Large" else "k+",
              label=problemSize + " Problem")

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
axes.set_title("Asynchronous Annealing Collision Rate")
plt.legend(handletextpad=0)
figure.tight_layout()
figure.savefig("collision_rate.pdf")
