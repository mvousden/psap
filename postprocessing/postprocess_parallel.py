#!/usr/bin/env python3

# Postprocesses output from a PSAP run. Call this script pointing at the
# appropriate output directory (it will search for the appropriate files).

import os
import numpy as np
import pandas as pd
import sys

import postprocessing
from postprocessing import filePathsParallel as filePaths


def doit(inputDir, outputDir="./"):
    """Loads data and draws graphs to an output directory."""

    os.makedirs(outputDir)

    postprocessing.draw_map(
        inputDir, os.path.join(outputDir, "initial_map.pdf"),
        filePaths, initialState=True)
    postprocessing.draw_map(
        inputDir, os.path.join(outputDir, "final_map.pdf"),
        filePaths, initialState=False)

    # Draw relaxation data
    fitnessData = pd.read_csv(os.path.join(inputDir, filePaths["fitness"]))
    figure, axes = postprocessing.plot_fitness(
        list(fitnessData["Iteration"]),
        list(fitnessData["Clustering Fitness"]),
        list(fitnessData["Locality Fitness"]))
    figure.savefig(os.path.join(outputDir, "fitness.pdf"))

    # Grab statistics for each thread.
    iterationsPerThread = []
    selectionCollisionsPerThread = []
    reliableFitnessPerThread = []
    insufficientlyDeterminedOps = []
    threadIndex = 0
    maxIteration = 0
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
        insufficientlyDeterminedOps = insufficientlyDeterminedOps + \
            list(annealOps[annealOps["Determination"] == 0]["Iteration"])
        maxIteration = max(maxIteration, annealOps["Iteration"].max())

        # Make life simpler for later
        del annealOps

        threadIndex += 1

    # Aggregate
    selectionCollisionPercent = sum(selectionCollisionsPerThread) /\
        sum(iterationsPerThread) * 100
    reliableFitnessPercent = sum(reliableFitnessPerThread) /\
        sum(iterationsPerThread) * 100

    # Degree data
    aDegrees = pd.read_csv(os.path.join(inputDir, filePaths["a_degrees"]))

    # Write statistics to file
    with open(
            os.path.join(outputDir, "postprocessing_statistics.ini"),
            "w") as collisionsFile:
        collisionsFile.write(
            "[postprocessing]\n"
            "compute_threads = {}\n"
            "hardware_nodes = {}\n"
            "application_nodes = {}\n"
            "mean_application_node_degree = {}\n"
            "selection_collision_rate = {:2.3f}%\n"
            "reliable_fitness_computation_rate = {:2.3f}%\n"
            .format(
                threadIndex,
                pd.read_csv(os.path.join(inputDir,
                                         filePaths["h_nodes"])).shape[0],
                aDegrees.shape[0],
                aDegrees["Degree"].mean(),
                selectionCollisionPercent,
                reliableFitnessPercent))

    # Draw determination data
    figure, axes = postprocessing.plot_determination_histogram(
        insufficientlyDeterminedOps, maxIteration)
    figure.savefig(os.path.join(outputDir, "determination.pdf"))

    # Draw hardware node loading data
    loadingData = pd.read_csv(
        os.path.join(inputDir, filePaths["h_node_loading"]))
    figure, axes = postprocessing.plot_loading_histogram(
        loadingData["Number of contained application nodes"])
    figure.savefig(os.path.join(outputDir, "loading.pdf"))


if __name__ == "__main__":
    rc = postprocessing.check_expected_files(sys.argv[1], filePaths.values())
    if (not rc):
        sys.exit(1)
    doit(sys.argv[1], os.path.basename(sys.argv[1]))
