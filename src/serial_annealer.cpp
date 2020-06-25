#include "serial_annealer.hpp"

#include <chrono>
#include <utility>

template<class DisorderT>
SerialAnnealer<DisorderT>::SerialAnnealer(Iteration maxIterationArg,
                                          std::filesystem::path outDirArg):
    maxIteration(maxIterationArg),
    disorder(maxIterationArg),
    outDir(outDirArg){}

/* Hits the solution repeatedly with a hammer and cools it. Hopefully improves
 * it (history has shown that it probably will work). */
template<class DisorderT>
void SerialAnnealer<DisorderT>::anneal(Problem& problem)
{
    /* Set up logging.
     *
     * If no output directory has been defined, then we run without
     * logging. Logging clobbers previous anneals. There are two output files
     * created in this way:
     *
     * - A CSV file describing the annealing operations, where each row
     *   corresponds to a new iteration. Note the use of '\n' over std::endl
     *   to reduce flushing (possibly at the cost of portability). Main will
     *   flush everything at the end.
     *
     * - A text file to which the wallclock runtime in seconds is dumped. */
    bool log = !outDir.empty();
    std::ofstream csvOut;
    std::ofstream clockOut;
    if (log)
    {
        csvOut.open((outDir / csvPath).u8string().c_str(),
                    std::ofstream::trunc);
        csvOut << "Selected application node index,"
               << "Selected hardware node index,"
               << "Transformed Fitness,"
               << "Determination\n";

        clockOut.open((outDir / clockPath).u8string().c_str(),
                      std::ofstream::trunc);
    }

    auto selA = problem.nodeAs.begin();
    auto selH = problem.nodeHs.begin();
    auto oldH = problem.nodeHs.begin();

    /* Base fitness "used" from the start of each iteration. */
    auto oldFitness = problem.compute_total_fitness();

    /* Write data for iteration zero to deploy initial fitness. */
    if (log) csvOut << "-1,-1," << oldFitness << ",1\n";

    /* Start the timer. */
    auto timeAtStart = std::chrono::steady_clock::now();
    do
    {
        iteration++;

        /* Selection */
        problem.select(selA, selH, oldH);
        if (log) csvOut << selA - problem.nodeAs.begin() << ","
                        << selH - problem.nodeHs.begin() << ",";

        /* Fitness of components before transformation. */
        auto oldFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2 +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        /* Transformation */
        problem.transform(selA, selH, oldH);

        /* Fitness of components before transformation. */
        auto newFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2 +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

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
            problem.transform(selA, oldH, selH);
        }
    }
    while (iteration != maxIteration);  /* Termination */

    if (log)
    {
        /* Write the elapsed time to the wallclock log file. */
        clockOut << std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - timeAtStart).count()
                 << std::endl;

        /* Close log files */
        csvOut.close();
        clockOut.close();
    }
}

#include "serial_annealer-impl.hpp"
