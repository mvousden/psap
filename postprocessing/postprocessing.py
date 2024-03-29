# Postprocessing logic common to parallel and serial data sets. Not callable.

import graphviz as gv
import matplotlib.pyplot as plt
import numpy as np
import os
import pandas as pd


# File paths to postprocess. This script gets unhappy if some of the files are
# missing.
filePathsCommon = {"a_degrees": "a_degrees.csv",
                   "final_a_h_graph": "final_a_h_graph.csv",
                   "final_a_to_h_map": "final_a_to_h_map.csv",
                   "h_graph": "h_graph.csv",
                   "h_node_loading": "h_node_loading.csv",
                   "h_nodes": "h_nodes.csv",
                   "initial_a_h_graph": "initial_a_h_graph.csv",
                   "initial_a_to_h_map": "initial_a_to_h_map.csv",
                   "wallclock": "wallclock.txt"}
filePathsParallel = {}
filePathsSerial = {}
filePathsParallel.update(filePathsCommon)
filePathsSerial.update(filePathsCommon)
filePathsParallel["anneal_ops"] = "anneal_ops-{}.csv"
filePathsParallel["fitness"] = "reliable_fitness_values.csv"
filePathsSerial["anneal_ops"] = "anneal_ops.csv"


def check_expected_files(inputDir, expectedFilePath):
    """Checks that a directory contains all expected files.

    Returs True if so and False otherwise. Prints the missing files
    also. If a single-level format specifier is present in a path, it is
    expanded to '0'. Arguments:

     - inputDir: String path (absolute or relative) to directory to test.

     - expectedFilePath: List of string file names to test for.
"""
    rc = True
    for expectedFilePath in expectedFilePath:
        fullPath = os.path.join(inputDir, expectedFilePath)
        if "{}" in fullPath:
            fullPath = fullPath.format(0)
        if (not os.path.exists(fullPath)):
            rc = False
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


def draw_map(inputDir, outPath, filePaths, state="final",
             maxNodeHLoadArg=None):
    """Draws a map showing device placement on a hardware graph for a given
    state.

    Loads various files, and draws the (dot) graph to outPath. Arguments:

     - inputDir: String path (absolute or relative) to directory to read data
           from.

     - outPath: String path (absolute or relative) to write the map to.

     - filePaths: Dictionary (ala the top of this file) containing the name of
           various files of interest.

     - state: String denoting whether to draw the map for the "initial"
           state, the "final" state (default), or no application data (via
           None).

     - maxNodeHLoadArg: Integer or None, forcing a maximum node loading on the
           map. If None (default), impose no maximum. Not used if the
           application map is ignored using state=None.

    Returns nothing."""

    # Load hardware graph data
    hGraphData = pd.read_csv(os.path.join(inputDir, filePaths["h_graph"]))

    # Load hardware node data
    hNodeData = pd.read_csv(os.path.join(inputDir, filePaths["h_nodes"]))

    # Load application node mapping - defines application edges.
    if state is not None:
        aToHGraphData = pd.read_csv(
            os.path.join(inputDir, filePaths[state + "_a_h_graph"]))

    # Load mapping data
    if state is not None:
        aMapData = pd.read_csv(
            os.path.join(inputDir, filePaths[state + "_a_to_h_map"]))

    # All hardware nodes
    nodeHs = hNodeData["Hardware node name"].values

    # Hardware node positional data
    explicitPositions = not ((hNodeData["Horizontal position"] == -1).any() or
                             (hNodeData["Vertical position"] == -1).any())

    # Hardware node loading - derive node colours from that loading.
    if state is not None:
        nodeHLoad = [len(aMapData[aMapData["Hardware node name"] == nodeH])
                     for nodeH in nodeHs]

        # Compute maximum loading present in the graph.
        if maxNodeHLoadArg is not None:
            maxNodeHLoad = max(nodeHLoad)
        else:
            maxNodeHLoad = maxNodeHLoadArg

        nodeColours = colour_from_loadings(nodeHLoad, maxNodeHLoad)

        # For each edge in the AH graph, store it in the edgeAHs array if it is
        # not a duplicate.
        edgeAHs = []
        for index, edge in aToHGraphData.iterrows():  # Sorry Pandas purists
            nodes = [edge["Hardware node name (first)"],
                     edge["Hardware node name (second)"]]
            primary = min(*nodes)
            secondary = max(*nodes)
            edgeAHs.append(nodes + [edge["Loading"]])

    # Start drawing pretty picture.
    graph = gv.Graph("G", strict=True,
                     engine="neato" if explicitPositions else "circo")
    graph.attr(margin="0")
    graph.attr("node", shape="square", style="filled", label="",
               fillcolor="#000000", margin="0")

    # Hardware nodes
    for nodeIndex in range(len(nodeHs)):
        nodePos = None if not explicitPositions else \
            "{},{}!".format(hNodeData.iloc[nodeIndex]["Horizontal position"] * 2,
                            hNodeData.iloc[nodeIndex]["Vertical position"] * 2)
        if state is not None:
            fillcolor = nodeColours[nodeIndex]
            label = str(nodeHLoad[nodeIndex])
        else:
            fillcolor = "#FFFFFF"
            label = nodeHs[nodeIndex]

        graph.node(nodeHs[nodeIndex], fillcolor=fillcolor, label=label,
                   pos=nodePos)

    # Application-hardware edges
    if state is not None:
        edgeAHColour = "#FF0000"
        for edgeIndex in range(len(edgeAHs)):
            graph.edge(*edgeAHs[edgeIndex][:2], color=edgeAHColour,
                       label=str(edgeAHs[edgeIndex][2]),
                       fontcolor=edgeAHColour, constraint="false")

    # All hardware edges (we don't care about weight)
    edgeHs = hGraphData.values.tolist()

    # Hardware edges
    for edgeIndex in range(len(edgeHs)):
        graph.edge(*edgeHs[edgeIndex], color="#000000", label="")

    # And exhale
    fileName, extension = os.path.splitext(os.path.basename(outPath))
    graph.render(filename=fileName, directory=os.path.dirname(outPath),
                 cleanup=True, format=extension.split(".")[-1])


def plot_loading_histogram(loadings, numBins=10):
    """Draws a histogram showing the distribution of how hardware nodes are
    loaded with application nodes.

    Arguments:

     - loadings: Iterable containing, for each hardware node, an integer
         denoting the number of application nodes mapped onto it.

     - numBins: Integer number of bins to draw with.

    Returns the figure and axes as a tuple (for your editing/saving needs)."""

    # Plot data
    figure, axes = plt.subplots()
    axes.hist(loadings, bins=numBins, color="r", edgecolor="#550000")

    # Formatting
    numberOfHardwareNodes = len(loadings)

    # This suppresses a UserWarning raised when limits are set to the same
    # value.
    axes.set_xlim(min(loadings), max(loadings +
                  (0 if min(loadings) != max(loadings) else 1e-2)))

    # The formatting continues...
    axes.set_ylim(0, len(loadings))
    axes.xaxis.get_major_locator().set_params(integer=True)
    axes.yaxis.get_major_locator().set_params(integer=True)
    axes.set_xlabel("Number of application nodes placed on hardware nodes")
    axes.set_ylabel("Occurences (total={})".format(numberOfHardwareNodes))
    axes.set_title("Hardware Node Loading")
    axes.spines["right"].set_visible(False)
    axes.spines["top"].set_visible(False)
    axes.yaxis.set_ticks_position("left")
    axes.xaxis.set_ticks_position("bottom")
    figure.tight_layout()

    return figure, axes


def plot_determination_histogram(underdeterminedIterations, maxIteration,
                                 numBins=25):
    """Draws a histogram showing how the determination rate of the annealing
    operation changes with iteration (and by extension, cooling schedule).

    Arguments:

     - underdeterminedIterations: Iterable containing iterations that failed
         the determination check.

     - maxIteration: Integer denoting the final iteration (regardless of
         determination); this is used to align the histogram.

     - numBins: Integer number of bins to draw with.

    Returns the figure and axes as a tuple (for your editing/saving needs)."""

    # Compute histogram data
    occs, edges = np.histogram(underdeterminedIterations, bins=numBins)
    occs = occs / (maxIteration / numBins) * 100  # Percentage

    # Plot data
    figure, axes = plt.subplots()
    axes.bar(edges[:-1], occs, width=np.diff(edges), edgecolor="#005500",
             align="edge", color="g")
    axes.grid(axis="y", color="k", alpha=0.5, linestyle="--")

    # Formatting
    axes.ticklabel_format(axis="x", scilimits=(0, 0), style="sci")
    axes.set_xlim(0, maxIteration * 1.05)
    axes.set_ylim(0, 100)
    axes.set_xlabel("Iteration")
    axes.set_ylabel("Insufficiently-Determined Selection Operation Rate (%)")
    axes.set_title("Determination Histogram")
    axes.spines["right"].set_visible(False)
    axes.spines["top"].set_visible(False)
    axes.yaxis.set_ticks_position("left")
    axes.xaxis.set_ticks_position("bottom")
    figure.tight_layout()

    return figure, axes


def plot_fitness(iterations, clusteringFitnesses, localityFitnesses):
    """Draws fitness data onto a matplotlib figure/axes pair.

    Arguments:

     - iterations: Iterable, indexable containing iteration values for each
         fitness value in 'fitnesses'.

     - clusteringFitnesses: Iterable, indexable containing clustering fitness
         data; one for each iteration in 'iterations'.

     - localityFitnesses: Iterable, indexable containing locality fitness
         data; one for each iteration in 'iterations'.

    Returns the figure and axes as a tuple (for your editing/saving needs)."""

    figure, axes = plt.subplots()

    clusterColour = "#AAAAFF"
    localityColour = "#88FF88"
    edgeColour = "#000044"

    # Below 30 iterations, draw a bar chart. Above, draw a line instead.
    if len(iterations) < 30:
        widths = list(np.diff(iterations))
        widths.append(widths[-1])  # It's gotta go somewhere
        axes.bar(iterations, clusteringFitnesses, alpha=0.8,
                 edgeColor=edgeColour, color=clusterColour,
                 label="Clustering Fitness", width=widths)
        axes.bar(iterations, localityFitnesses, bottom=clusteringFitnesses,
                 alpha=0.8, edgeColor=edgeColour, color=localityColour,
                 label="Locality Fitness", width=widths)

        axes.set_xlim(0 - widths[0] / 2, iterations[-1] * 1.05)

    else:
        totalFitnesses = [clusteringFitnesses[zI] + localityFitnesses[zI]
                          for zI in range(len(iterations))]
        axes.plot(iterations, clusteringFitnesses, color=edgeColour)
        axes.plot(iterations, totalFitnesses, color=edgeColour)
        axes.plot([0, iterations[-1]], [0, 0], color=edgeColour)
        axes.fill_between(iterations, 0, clusteringFitnesses,
                          color=clusterColour, label="Clustering Fitness")
        axes.fill_between(iterations, clusteringFitnesses, totalFitnesses,
                          color=localityColour, label="Locality Fitness")

        axes.set_xlim(0, iterations[-1] * 1.05)

    # Legend
    axes.legend(frameon=False, loc=4)

    # Formatting
    axes.ticklabel_format(axis="x", scilimits=(0, 0), style="sci")
    axes.set_xlabel("Iteration")
    axes.set_ylabel("Fitness")
    axes.set_title("Simulated Annealing Relaxation Profile")
    axes.spines["right"].set_visible(False)
    axes.spines["top"].set_visible(False)
    axes.yaxis.set_ticks_position("left")
    axes.xaxis.set_ticks_position("bottom")
    figure.tight_layout()

    return figure, axes
