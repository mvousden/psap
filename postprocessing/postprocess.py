#!/usr/bin/env python3

# Postprocesses output from a PSAP run. Call this script pointing at the
# appropriate output directory (it will search for the appropriate files).

import os
import graphviz as gv
import numpy as np
import pandas as pd
import sys

# This script gets unhappy if some of the files are missing.
filePaths = {"anneal_ops": "anneal_ops.csv",
             "final_a_h_graph": "final_a_h_graph.csv",
             "final_a_to_h_map": "final_a_to_h_map.csv",
             "h_graph": "h_graph.csv",
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


def colour_from_loadings(loadings, baseColor="#FF0000"):
    """Computes colors given loading values.

    Given an array of loading values (loadings), returns an array of
    colors that graphviz can understand that can be used to colour the
    nodes. The node with the greatest loading uses baseColor, and a node
    with zero loading uses white (#FFFFFF).

    This is achieved through clever sneaky use of the alpha channel."""
    maxLoading = max(loadings)
    return [baseColor + hex(int(loading / maxLoading * 255))[2:]
            for loading in loadings]


def doit(inputDir):
    """Loads data and draws graphs."""

    # Load hardware graph data
    graphH = pd.read_csv(os.path.join(inputDir, filePaths["h_graph"]))

    # All hardware nodes
    nodeHs = np.unique(graphH[graphH.columns].values)

    # All hardware edges (we don't care about weight)
    edgeHs = graphH.values.tolist()

    # Load initial mapping data
    mapAToH = pd.read_csv(os.path.join(inputDir,
                                       filePaths["initial_a_to_h_map"]))

    # Initial hardware node loading - derive node colours from that loading.
    nodeHLoad = [len(mapAToH[mapAToH["Hardware node name"] == nodeH])
                 for nodeH in nodeHs]

    nodeColours = colour_from_loadings(nodeHLoad)

    # Initial application node mapping - defines edge weights. <!>
    graphAH = pd.read_csv(os.path.join(inputDir,
                                       filePaths["initial_a_h_graph"]))

    # Draw pretty picture.
    graph = gv.Graph("G", filename="postprocess.gv", engine="circo")
    graph.attr(margin="0")
    graph.attr("node", shape="square", style="filled", label="",
               fillcolor="#000000", margin="0")

    # Hardware nodes
    for nodeIndex in range(len(nodeHs)):
        graph.node(nodeHs[nodeIndex], fillcolor=nodeColours[nodeIndex],
                   label=str(nodeHLoad[nodeIndex]))

    # Hardware edges
    for edgeIndex in range(len(edgeHs)):
        graph.edge(*edgeHs[edgeIndex])
    graph.render()


if __name__ == "__main__":
    rc = check_expected_files(sys.argv[1])
    if (rc != 0):
        sys.exit(rc)
    doit(sys.argv[1])  # Sorry
