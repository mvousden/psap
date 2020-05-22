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
filePaths = {"anneal_ops": "anneal_ops.csv",
             "final_a_h_graph": "final_a_h_graph.csv",
             "final_a_to_h_map": "final_a_to_h_map.csv",
             "h_graph": "h_graph.csv",
             "h_nodes": "h_nodes.csv",
             "initial_a_h_graph": "initial_a_h_graph.csv",
             "initial_a_to_h_map": "initial_a_to_h_map.csv"}


def check_expected_files(inputDir):
    """Checks that all expected data files can be found in inputDir."""
    rc = 0
    for expectedFilePath in filePaths.values():
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
    graphH = pd.read_csv(os.path.join(inputDir, filePaths["h_graph"]))

    # All hardware nodes
    nodeHs = np.unique(graphH[graphH.columns].values)

    # All hardware edges (we don't care about weight)
    edgeHs = graphH.values.tolist()

    # Load mapping data
    mapAToH = pd.read_csv(os.path.join(inputDir,
                                       filePaths[state + "_a_to_h_map"]))

    # Initial hardware node loading - derive node colours from that loading.
    nodeHLoad = [len(mapAToH[mapAToH["Hardware node name"] == nodeH])
                 for nodeH in nodeHs]

    # Compute maximum loading (assume that the maximum loading exists in the
    # initial placement, but if that's not true, you can always change it
    # here).
    maxNodeHLoad = max(nodeHLoad)

    nodeColours = colour_from_loadings(nodeHLoad, maxNodeHLoad)

    # Initial application node mapping - defines application edges.
    graphAH = pd.read_csv(os.path.join(inputDir,
                                       filePaths[state + "_a_h_graph"]))

    # For each edge in the AH graph, store it in the edgeAHs array if it is not
    # a duplicate.
    edgeAHs = []
    for index, edge in graphAH.iterrows():  # Sorry Pandas purists
        nodes = [edge["Hardware node name (first)"],
                 edge["Hardware node name (second)"]]
        primary = min(*nodes)
        secondary = max(*nodes)
        edgeAHs.append(nodes + [edge["Loading"]])

    # Draw pretty picture.
    graph = gv.Graph("G", filename="postprocess.gv", engine="circo",
                     strict=True)
    graph.attr(margin="0")
    graph.attr("node", shape="square", style="filled", label="",
               fillcolor="#000000", margin="0")

    # Hardware nodes
    for nodeIndex in range(len(nodeHs)):
        graph.node(nodeHs[nodeIndex], fillcolor=nodeColours[nodeIndex],
                   label=str(nodeHLoad[nodeIndex]))

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

    # Load simulated annealing transition data
    ops = pd.read_csv(os.path.join(inputDir, filePaths["anneal_ops"]))
    fitnessChanges = ops.loc[ops["Determination"] == 1]\
      ["Transformed Fitness"].to_dict()

    # Draw pretty figure
    figure, axes = plt.subplots()
    axes.plot(list(fitnessChanges.keys()), list(fitnessChanges.values()),
              'rx')
    axes.set_xlabel("Iteration")
    axes.set_ylabel("Fitness")
    axes.set_title("Simulated Annealing Relaxation")
    figure.savefig("fitness.pdf")


if __name__ == "__main__":
    rc = check_expected_files(sys.argv[1])
    if (rc != 0):
        sys.exit(rc)
    doit(sys.argv[1], state="final")  # Sorry
