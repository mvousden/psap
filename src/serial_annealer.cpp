#include "serial_annealer.hpp"

template<class DisorderT>
SerialAnnealer<DisorderT>::SerialAnnealer(Iteration maxIterationArg,
                                          std::string csvPathArg):
    maxIteration(maxIterationArg),
    disorder(maxIterationArg),
    csvPath(csvPathArg){}

/* Hits the solution repeatedly with a hammer and cools it. Hopefully improves
 * it (history has shown that it probably will work). */
template<class DisorderT>
void SerialAnnealer<DisorderT>::anneal(Problem& problem)
{
    /* If no output path has been defined for logging operations, then don't
     * log them. Otherwise, do so. Logging clobbers previous anneals. Note the
     * use of '\n' over std::endl to reduce flushing (possibly at the cost of
     * portability). Main will flush everything at the end.
     *
     * Each row in the CSV corresponds to a new iteration. */
    bool log = !csvPath.empty();
    std::ofstream csvOut;
    if (log)
    {
        csvOut.open(csvPath.c_str(), std::ofstream::trunc);
        csvOut << "Selected application node index,"
               << "Selected hardware node index,"
               << "Transformed Fitness,"
               << "Determination\n";
    }

    auto selA = problem.nodeAs.begin();
    auto selH = problem.nodeHs.begin();
    auto oldH = *selH;  /* Note - not an iterator */

    /* Base fitness "used" from the start of each iteration. */
    auto oldFitness = problem.compute_total_fitness();

    /* Write data for iteration zero to deploy initial fitness. */
    if (log) csvOut << "-1,-1," << oldFitness << ",1\n";

    while (true)
    {
        iteration++;

        /* Selection */
        problem.select(selA, selH);
        if (log) csvOut << selA - problem.nodeAs.begin() << ","
                        << selH - problem.nodeHs.begin() << ",";

        /* Fitness of components before transformation. */
        oldH = problem.nodeHs[(*selA)->location.lock()->index];
        auto oldFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2 +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(*oldH);

        /* Transformation */
        problem.transform(selA, selH);

        /* Fitness of components before transformation. */
        auto newFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2 +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(*oldH);

        auto newFitness = oldFitness - oldFitnessComponents +
            newFitnessComponents;
        if (log) csvOut << newFitness << ",";

        /* Determination */
        bool sufficientlyDetermined =
            disorder.determine(oldFitness, newFitness, iteration);

        /* If the solution was sufficiently determined to be chosen, update the
         * base fitness to support computation for the next
         * iteration. Otherwise, revert the solution. */
        if (sufficientlyDetermined)
        {
            if (log) csvOut << 1 << '\n';
            oldFitness = newFitness;
        }

        else
        {
            if (log) csvOut << 0 << '\n';
            auto oldHIt = problem.nodeHs.begin() + oldH->index;
            problem.transform(selA, oldHIt);
        }

        /* Termination */
        if (iteration == maxIteration) break;
    }

    if (log) csvOut.close();
}

#include "serial_annealer-impl.hpp"
