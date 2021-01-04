#!/usr/bin/env python3

# This Python script estimates the percentage of fitness calculations that are
# erroneous in a parallel simulated annealing run.
#
# The scenario considered here is a non-toroidal grid application placed on a
# series of hardware nodes (whose connectivity is irrelevant). This model
# assumes that all parallel workers start and complete their allocated
# iteration at the same time as each other worker.
#
# The percentage of erroneous calculations is written to standard output, along
# with some other useful statistics.

import random
import time

# Properties (all are integers)
problem = "small"
if problem == "small":
    nh = 384  # Number of hardware nodes in the problem definition.
    naEdge = 1000  # Number of application nodes along the edge of the square.
elif problem == "large":
    nh = 384 * 2  # Number of hardware nodes in the problem definition.
    naEdge = 1414  # Number of application nodes along the edge of the square.
else:
    raise RuntimeError("Invalid problem type '" + problem + "'.")

seed = 48  # RNG seed. Set to None if a seed is to be chosed based on the
           # clock.
nit = int(1e5)  # Number of annealing iterations to simulate. As far as your
                # inner statistician is concerned, this is a count of the
                # number of samples, and offers diminishing returns with the
                # accuracy of the output
nw = 64  # Number of parallel compute workers. Does not need to divide evenly
         # into the number of iterations, but more iterations will be added to
         # nit to accomodate the remainder.

# And here we go.
if seed is not None:
    random.seed(seed)

# Iterate until nit iterations (or more) have been processed.
appErrors = 0  # Total number of iterations that have an erroneous fitness
               # computation due to an overlap with application nodes.
hwErrors = 0  # As above, but for "local" iterations that share hardware nodes.
totErrors = 0  # Total number of iterations that have an erroroneous fitness
               # calculation of some kind. NB: Not the sum of the above two
               # quantities, because "tot = app or hw".
it = 0
startTime = time.time()
while it < nit:

    # Select hardware nodes for each worker. Note that here a worker can select
    # the same hardware node to be old and new, but we don't care because
    # workers are never compared against themselves.
    hNodesOld = [random.randint(1, nh) for _ in range(nw)]
    hNodesNew = [random.randint(1, nh) for _ in range(nw)]

    # Select application nodes for each worker.
    aNodes = [(random.randint(1, naEdge), random.randint(1, naEdge))
              for _ in range(nw)]

    # Iterate over each worker to identify whether an error has taken place in
    # that worker's iteration.
    for innerIdx in range(nw):

        # Holding state for this iteration.
        appError = False
        hwError = False

        # Check against each other worker in turn. Note, when I say "worker's
        # iteration", I'm talking about the innerIdx loop.
        for outerIdx in range(nw):
            if innerIdx == outerIdx:
                continue

            # Application error? Only check if we haven't found one already in
            # this worker's iteration.
            if appError is False:

                # An application error has occured in this worker's iteration
                # only if another worker has either selected the same
                # application node exactly, or has selected an adjacent
                # application node.
                if (abs(aNodes[outerIdx][0] - aNodes[innerIdx][0]) < 2 and
                    abs(aNodes[outerIdx][1] - aNodes[innerIdx][1]) < 2):
                    appError = True
                    appErrors += 1

            # Hardware error? Only check if we haven't found one already this
            # worker's iteration.
            if hwError is False:

                # A hardware error has occured in this worker's iteration only
                # if another worker has selected the same hardware node.
                if (hNodesNew[outerIdx] == hNodesNew[innerIdx] or
                    hNodesOld[outerIdx] == hNodesNew[innerIdx] or
                    hNodesNew[outerIdx] == hNodesOld[innerIdx] or
                    hNodesOld[outerIdx] == hNodesOld[innerIdx]):
                    hwError = True
                    hwErrors += 1

        # Track total errors (for this worker).
        if appError or hwError:
            totErrors += 1

    # Moving swiftly on (we've just done one iteration for each worker).
    it += nw

# Write out the totals.
print("Inputs:")
if seed is not None:
    print("    RNG seed: {}".format(seed))
print("    Number of hardware nodes: {}".format(nh))
print("    Number of application nodes: {0} x {0}".format(naEdge))
print("    Number of compute workers: {}".format(nw))
print("\nOutputs:")
print("    Time taken to compute these results: {:d} seconds"
      .format(int(time.time() - startTime)))
print("    Number of iterations: {}".format(it))  # NB: not nit! We may overrun
print("    Iterations with at least one application error: {} ({:2.3f}%)"
      .format(appErrors, appErrors / it * 100))
print("    Iterations with at least one hardware error: {} ({:2.3f}%)"
      .format(hwErrors, hwErrors / it * 100))
print("    Iterations with at least one error of any kind: {} ({:2.3f}%)"
      .format(totErrors, totErrors / it * 100))
