#!/usr/bin/env python3

# Postprocesses output from a PSAP run. Call this script pointing at the
# appropriate output directory (it will search for the appropriate files).

import os
import graphviz as gv
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

# This script gets unhappy if some of the files are missing.
filePaths = {"anneal_ops": "anneal_ops-{}.csv",
             "final_a_h_graph": "final_a_h_graph.csv",
             "final_a_to_h_map": "final_a_to_h_map.csv",
             "fitness": "reliable_fitness_values.csv",
             "h_graph": "h_graph.csv",
             "h_node_loading": "h_node_loading.csv",
             "h_nodes": "h_nodes.csv",
             "initial_a_h_graph": "initial_a_h_graph.csv",
             "initial_a_to_h_map": "initial_a_to_h_map.csv",
             "wallclock": "wallclock.txt"}


def check_expected_files(inputDir):
    """Checks that all expected data files can be found in inputDir."""
    rc = 0
    for expectedFilePath in filePaths.values():
        if ("{}" in expectedFilePath):
            continue
        fullPath = os.path.join(inputDir, expectedFilePath)
        if (not os.path.exists(fullPath)):
            rc = 1
            print("Error: File not found: {}.".format(fullPath))
    return rc


def colour_from_loadings(loadings, maxLoading=None, baseColor="#FF0000"):
    """Computes colors given loading values.

    Given an array of loading values (loadings), returns an array of
    colors that graphviz can understand that can be used to colour the
    nodes. The node with the greatest loading uses baseColor, and a node
    with zero loading uses white (#FFFFFF).

    This is achieved through clever sneaky use of the alpha channel."""
    if maxLoading is None:
        maxLoading = max(loadings)
    return [baseColor + hex(int(loading / maxLoading * 255))[2:]
            for loading in loadings]


def doit(inputDir, state="initial"):
    """Loads data and draws graphs."""

    # Load hardware graph data
    hGraphData = pd.read_csv(os.path.join(inputDir, filePaths["h_graph"]))

    # Load hardware node data
    hNodeData = pd.read_csv(os.path.join(inputDir, filePaths["h_nodes"]))

    # Load mapping data
    aMapData = pd.read_csv(os.path.join(inputDir,
                                        filePaths[state + "_a_to_h_map"]))

    # Load initial application node mapping - defines application edges.
    aToHGraphData = pd.read_csv(os.path.join(inputDir,
                                       filePaths[state + "_a_h_graph"]))

    # Load simulated annealing fitness data
    fitnessData = pd.read_csv(os.path.join(inputDir, filePaths["fitness"]))

    # All hardware nodes
    nodeHs = hNodeData["Hardware node name"].values

    # Hardware node positional data
    explicitPositions = not ((hNodeData["Horizontal position"] == -1).any() or
                             (hNodeData["Vertical position"] == -1).any())

    # All hardware edges (we don't care about weight)
    edgeHs = hGraphData.values.tolist()

    # Hardware node loading - derive node colours from that loading.
    nodeHLoad = [len(aMapData[aMapData["Hardware node name"] == nodeH])
                 for nodeH in nodeHs]

    # Compute maximum loading (assume that the maximum loading exists in the
    # initial placement, but if that's not true, you can always change it
    # here).
    maxNodeHLoad = max(nodeHLoad)
    nodeColours = colour_from_loadings(nodeHLoad, maxNodeHLoad)

    # For each edge in the AH graph, store it in the edgeAHs array if it is not
    # a duplicate.
    edgeAHs = []
    for index, edge in aToHGraphData.iterrows():  # Sorry Pandas purists
        nodes = [edge["Hardware node name (first)"],
                 edge["Hardware node name (second)"]]
        primary = min(*nodes)
        secondary = max(*nodes)
        edgeAHs.append(nodes + [edge["Loading"]])

    # Draw pretty picture.
    graph = gv.Graph("G", filename="map.gv", strict=True,
                     engine="neato" if explicitPositions else "circo")
    graph.attr(margin="0")
    graph.attr("node", shape="square", style="filled", label="",
               fillcolor="#000000", margin="0")

    # Hardware nodes
    for nodeIndex in range(len(nodeHs)):
        nodePos = None if not explicitPositions else \
            "{},{}!".format(hNodeData.iloc[nodeIndex]["Horizontal position"],
                            hNodeData.iloc[nodeIndex]["Vertical position"])
        graph.node(nodeHs[nodeIndex], fillcolor=nodeColours[nodeIndex],
                   label=str(nodeHLoad[nodeIndex]), pos=nodePos)

    # Application-hardware edges
    edgeAHColour = "#FF0000"
    for edgeIndex in range(len(edgeAHs)):
        graph.edge(*edgeAHs[edgeIndex][:2], color=edgeAHColour,
                   label=str(edgeAHs[edgeIndex][2]), fontcolor=edgeAHColour,
                   constraint="false")

    # Hardware edges
    for edgeIndex in range(len(edgeHs)):
        graph.edge(*edgeHs[edgeIndex], color="#000000", label="")

    graph.render()

    ## Grab collision statistics for each thread.
    iterationsPerThread = []
    selectionCollisionsPerThread = []
    reliableFitnessPerThread = []
    threadIndex = 0
    while True:
        annealOpsPath = os.path.join(
            inputDir, filePaths["anneal_ops"].format(threadIndex))

        # If it doesn't exist, then we have analysed the data for each thread.
        if not os.path.exists(annealOpsPath):
            break

        # Otherwise, grab metrics.
        annealOps = pd.read_csv(annealOpsPath)
        iterationsPerThread.append(annealOps.shape[0])
        selectionCollisionsPerThread.append(
            annealOps["Number of selection collisions"].sum())
        reliableFitnessPerThread.append(
            annealOps["Fitness computation is reliable"].sum())

        # Make life simpler for later
        del annealOps

        threadIndex += 1

    # Aggregate
    selectionCollisions = sum(selectionCollisionsPerThread)
    reliableFitnessPercent = sum(reliableFitnessPerThread) /\
        sum(iterationsPerThread) * 100

    ## Draw pretty fitness figure
    fitnessData = pd.read_csv(os.path.join(inputDir, filePaths["fitness"]))
    figure, axes = plt.subplots()

    # Draw relaxation data
    axes.plot(fitnessData["Iteration"], fitnessData["Fitness"], 'kx', alpha=1)

    # Special points and their lines
    minIteration = 0  # Hack
    maxIteration = fitnessData["Iteration"].values[-1]
    firstFitness = -2.34684e+10  # Hack
    firstFitness = -18224  # Hack
    firstFitness = -182
    lastFitness = fitnessData["Fitness"].values[-1]
    axes.plot(minIteration, firstFitness, 'rx')
    axes.plot(maxIteration, lastFitness, 'bx')
    axes.plot([minIteration, maxIteration],
              [firstFitness, firstFitness], 'r--')
    axes.plot([minIteration, maxIteration],
              [lastFitness, lastFitness], 'b--')

    # Aggregate counter data
    aggregateText = "Selection Collisions: {:.3e}\n"\
                    "Reliable Fitness Computation Rate: {:2.3f}%"\
                    .format(selectionCollisions, reliableFitnessPercent)
    axes.text(0.35, 0.5, aggregateText, transform=axes.transAxes)

    # Formatting
    axes.ticklabel_format(axis="x", scilimits=(0, 0), style="sci")
    axes.set_xlim(0, maxIteration * 1.05)
    axes.set_xlabel("Iteration")
    axes.set_ylabel("Fitness")
    axes.set_title("Simulated Annealing Relaxation")
    axes.spines["right"].set_visible(False)
    axes.spines["top"].set_visible(False)
    axes.yaxis.set_ticks_position("left")
    axes.xaxis.set_ticks_position("bottom")
    figure.tight_layout()
    figure.savefig("fitness.pdf")


if __name__ == "__main__":
    rc = check_expected_files(sys.argv[1])
    if (rc != 0):
        sys.exit(rc)
    doit(sys.argv[1], state="final")  # Sorry
