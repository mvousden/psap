#include "serial_annealer.hpp"

#include <chrono>
#include <sstream>
#include <string>
#include <utility>

template<class DisorderT>
SerialAnnealer<DisorderT>::SerialAnnealer(
    Iteration maxIterationArg,
    const std::filesystem::path& outDirArg,
    Seed disorderSeed):
    Annealer<DisorderT>(maxIterationArg, outDirArg, "SerialAnnealer",
                        disorderSeed){}

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
    std::ofstream csvOut;
    std::ofstream clockOut;
    if (this->log)
    {
        csvOut.open(this->outDir / csvPath, std::ofstream::trunc);
        csvOut << "Selected application node index,"
               << "Selected hardware node index,"
               << "Transformed Fitness,"
               << "Transformed Clustering Fitness,"
               << "Transformed Locality Fitness,"
               << "Determination\n";

        clockOut.open(this->outDir / clockPath, std::ofstream::trunc);
        this->write_metadata();
    }

    auto selA = problem.nodeAs.begin();
    auto selH = problem.nodeHs.begin();
    auto oldH = problem.nodeHs.begin();

    /* Base fitness "used" from the start of each iteration. */
    auto oldClusteringFitness = problem.compute_total_clustering_fitness();
    auto oldLocalityFitness = problem.compute_total_locality_fitness();
    auto oldFitness = oldLocalityFitness + oldClusteringFitness;

    /* Write data for iteration zero to deploy initial fitness. */
    if (this->log) csvOut << "-1,-1,"
                          << oldFitness << ","
                          << oldClusteringFitness << ","
                          << oldLocalityFitness << ",1\n";

    /* Start the timer. */
    auto timeAtStart = std::chrono::steady_clock::now();
    do
    {
        iteration++;

        /* Selection */
        problem.select_serial(selA, selH, oldH);
        if (this->log) csvOut << selA - problem.nodeAs.begin() << ","
                              << selH - problem.nodeHs.begin() << ",";

        /* Fitness of components before transformation. */
        auto oldClusteringFitnessComponents =
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        auto oldLocalityFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2;

        /* Transformation */
        problem.transform(selA, selH, oldH);

        /* Fitness of components after transformation. */
        auto newClusteringFitnessComponents =
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        auto newLocalityFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2;

        /* New fitness computation, and writing to CSV. */
        auto newClusteringFitness = oldClusteringFitness -
            oldClusteringFitnessComponents + newClusteringFitnessComponents;

        auto newLocalityFitness = oldLocalityFitness -
            oldLocalityFitnessComponents + newLocalityFitnessComponents;

        auto newFitness = newLocalityFitness + newClusteringFitness;

        if (this->log) csvOut << newFitness << ","
                              << newClusteringFitness << ","
                              << newLocalityFitness << ",";

        /* Determination */
        bool sufficientlyDetermined =
            this->disorder.determine(oldFitness, newFitness, iteration);

        /* If the solution was sufficiently determined to be chosen, update the
         * base fitness to support computation for the next
         * iteration. Otherwise, revert the solution. */
        if (sufficientlyDetermined)
        {
            if (this->log) csvOut << 1 << '\n';
            oldFitness = newFitness;
            oldClusteringFitness = newClusteringFitness;
            oldLocalityFitness = newLocalityFitness;
        }

        else
        {
            if (this->log) csvOut << 0 << '\n';
            problem.transform(selA, oldH, selH);
        }
    }
    while (iteration != this->maxIteration);  /* Termination */

    if (this->log)
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
