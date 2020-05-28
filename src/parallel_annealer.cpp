#include "parallel_annealer.hpp"

#include <thread>

template<class DisorderT>
ParallelAnnealer<DisorderT>::ParallelAnnealer(unsigned numThreadsArg,
                                              Iteration maxIterationArg,
                                              std::string csvPathRootArg):
    numThreads(numThreadsArg),
    maxIteration(maxIterationArg),
    disorder(maxIterationArg),
    csvPathRoot(csvPathRootArg)
{
    log = !csvPathRoot.empty();
}

/* Hits the solution repeatedly with many hammers at the same time while
 * cooling it. Hopefully improves it (the jury's out). */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::anneal(Problem& problem)
{
    /* If no output path has been defined for logging operations, then don't
     * log them. Otherwise, do so. Logging clobbers previous anneals. Note the
     * use of '\n' over std::endl to reduce flushing (possibly at the cost of
     * portability). Main will flush everything at the end.
     *
     * There will be one CSV file for each thread. Each row in each CSV
     * corresponds to a new iteration. */
    std::vector<std::ofstream> csvOuts (numThreads);
    decltype(csvOuts)::size_type csvOutIndex;
    if (log)
    {
        for (csvOutIndex = 0; csvOutIndex < csvOuts.size(); csvOutIndex++)
        {
            std::stringstream csvPath;
            csvPath << csvPathRoot << "-" << csvOutIndex;
            csvOuts.at(csvOutIndex).open(csvPath.str().c_str(),
                                         std::ofstream::trunc);
            csvOuts.at(csvOutIndex) << "Iteration,"
                                    << "Selected application node index,"
                                    << "Selected hardware node index,"
                                    << "Transformed Fitness,"
                                    << "Determination\n";
        }
    }

    /* Initialise problem locking infrastructure */
    problem.initialise_atomic_locks();

    /* Spawn slave threads to do the annealing. */
    std::vector<std::thread> threads;
    for (unsigned threadId = 0; threadId < numThreads; threadId++)
        threads.emplace_back(&ParallelAnnealer<DisorderT>::co_anneal, this,
                             std::ref(problem),
                             std::ref(csvOuts.at(threadId)));

    /* Join with slave threads. */
    for (auto& thread : threads) thread.join();

    /* Close log files. */
    if (log) for (auto& csvOut : csvOuts) csvOut.close();
}

/* An individual hammer, to be wielded by a single thread. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::co_anneal(Problem& problem,
                                            std::ofstream& csvOut)
{
    auto selA = problem.nodeAs.begin();
    auto selH = problem.nodeHs.begin();
    auto oldH = problem.nodeHs.begin();

    /* Base fitness "used" from the start of each iteration. Note that the
     * currently-storef fitness will drift from the total fitness. This is fine
     * because determination only care about the fitness difference from an
     * operation - not its absolute value or ratio. */
    auto oldFitness = problem.compute_total_fitness();

    /* Write data for iteration zero to deploy initial fitness. May differ
     * between threads. */
    if (log) csvOut << "-1,-1," << oldFitness << ",1\n";

    Iteration localIteration;
    while (true)
    {
        /* Get the iteration number. The variable "iteration" is shared between
         * threads. */
        localIteration = iteration++;
        if (log) csvOut << localIteration << ",";

        /* "Atomic" selection */
        problem.select(selA, selH, oldH, true);
        if (log) csvOut << selA - problem.nodeAs.begin() << ","
                        << selH - problem.nodeHs.begin() << ",";

        /* RAII locking */
        std::lock_guard<decltype(Problem::lockAs)::value_type> appLock
            (problem.lockAs[selA - problem.nodeAs.begin()], std::adopt_lock);

        /* Fitness of components before transformation. */
        auto oldFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2 +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        /* Transformation */
        {
            std::lock_guard<decltype(tformMx)> lockTform(tformMx);
            problem.transform(selA, selH, oldH);
        }

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
            disorder.determine(oldFitness, newFitness, localIteration);

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
            {
                std::lock_guard<decltype(tformMx)> lockTform(tformMx);
                problem.transform(selA, oldH, selH);
            }
        }

        /* Termination. */
        if (iteration >= maxIteration) break;
    }
}

#include "parallel_annealer-impl.hpp"
