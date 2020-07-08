#!/usr/bin/env python3

# Postprocesses output from a serial PSAP run. Call this script pointing at the
# appropriate output directory (it will search for the appropriate files).

import os
import numpy as np
import pandas as pd
import sys

import postprocessing
from postprocessing import filePathsSerial as filePaths


def doit(inputDir):
    """Loads data and draws graphs."""

    # Load simulated annealing transition data
    postprocessing.draw_map(inputDir, "initial_map_serial.pdf", filePaths,
                            initialState=True)
    postprocessing.draw_map(inputDir, "final_map_serial.pdf", filePaths,
                            initialState=False)

    # Draw relaxation data
    opsData = pd.read_csv(os.path.join(inputDir, filePaths["anneal_ops"]))
    determinedData = opsData[opsData["Determination"] == 1]
    figure, axes = postprocessing.plot_fitness(
        list(determinedData.index),
        list(determinedData["Transformed Clustering Fitness"]),
        list(determinedData["Transformed Locality Fitness"]))
    del determinedData
    figure.savefig("fitness_serial.pdf")

    # Draw determination data
    figure, axes = postprocessing.plot_determination_histogram(
        opsData[opsData["Determination"] == 0].index, opsData.index.max())
    figure.savefig("determination_serial.pdf")

    # Draw hardware node loading data
    loadingData = pd.read_csv(os.path.join(inputDir,
                                           filePaths["h_node_loading"]))
    figure, axes = postprocessing.plot_loading_histogram(
        loadingData["Number of contained application nodes"])
    figure.savefig("loading_serial.pdf")


if __name__ == "__main__":
    rc = postprocessing.check_expected_files(sys.argv[1], filePaths.values())
    if (not rc):
        sys.exit(1)
    doit(sys.argv[1])
