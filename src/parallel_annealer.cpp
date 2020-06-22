#include "parallel_annealer.hpp"

#include <mutex>
#include <thread>
#include <utility>

template<class DisorderT>
ParallelAnnealer<DisorderT>::ParallelAnnealer(unsigned numThreadsArg,
                                              Iteration maxIterationArg,
                                              std::string csvPathRootArg):
    numThreads(numThreadsArg),
    maxIteration(maxIterationArg),
    disorder(maxIterationArg),
    csvPathRoot(std::move(csvPathRootArg))
{
    log = !csvPathRoot.empty();
}

/* Hits the solution repeatedly with many hammers at the same time while
 * cooling it. Hopefully improves it (the jury's out).
 *
 * The recordEvery argument, if non-zero, causes the annealing to stop after
 * every "recordEvery" iterations to compute and record the fitness at that
 * time. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::anneal(Problem& problem,
                                         Iteration recordEvery)
{
    /* If no output path has been defined for logging operations, then don't
     * log them. Otherwise, do so. Logging clobbers previous anneals. Note the
     * use of '\n' over std::endl to reduce flushing (possibly at the cost of
     * portability). Main will flush everything at the end.
     *
     * There will be one CSV file for each thread. Each row in each CSV
     * corresponds to a new iteration. There will also be one "master" CSV file
     * for the serial fitness computation if recordEvery is nonzero. */
    std::vector<std::ofstream> csvOuts (numThreads);
    std::ofstream csvOutMaster;
    decltype(csvOuts)::size_type csvOutIndex;
    if (log)
    {
        /* Per-thread logging. */
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

        /* Frequent serial fitness computation. */
        if (recordEvery != 0)
        {
            csvOutMaster.open(csvPathRoot.c_str(), std::ofstream::trunc);
            csvOutMaster << "Iteration,Fitness\n";
        }
    }

    /* Initialise problem locking infrastructure */
    problem.initialise_atomic_locks();

    /* This do-while loop spawns threads to do the annealing for every
     * "recording session", then joins the threads together and records the
     * fitness at that time. The reason for this is because the problem data
     * structure is not thread-safe (thanks STL), so we need to pause execution
     * while computing fitness. Not a big deal really, as long as the recording
     * frequency is low. */
    do
    {
        /* Compute next stopping point. */
        Iteration nextStop;
        if (recordEvery == 0) nextStop = maxIteration;
        else nextStop = std::min(maxIteration, iteration + recordEvery);

        /* Spawn slave threads to do the annealing. */
        std::vector<std::thread> threads;
        for (unsigned threadId = 0; threadId < numThreads; threadId++)
            threads.emplace_back(&ParallelAnnealer<DisorderT>::co_anneal, this,
                                 std::ref(problem),
                                 std::ref(csvOuts.at(threadId)), nextStop);

        /* Join with slave threads. */
        for (auto& thread : threads) thread.join();

        /* Compute fitness value and record it. */
        if (log and recordEvery != 0)
        {
            /* Problem logging (mostly for the timestamp). */
            std::stringstream message;
            message << "Stopping annealing to record fitness at iteration "
                    << iteration << "...";
            problem.log(message.str());

            /* Fitness computation and logging. */
            csvOutMaster << iteration << ","
                         << problem.compute_total_fitness() << std::endl;

            /* End timestamp */
            problem.log("Fitness logged.");
        }
    }
    while (iteration <= maxIteration);

    /* Close log files. */
    if (log)
    {
        for (auto& csvOut : csvOuts) csvOut.close();
        csvOutMaster.close();
    }
}

/* An individual hammer, to be wielded by a single thread. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::co_anneal(Problem& problem,
                                            std::ofstream& csvOut,
                                            Iteration maxIteration)
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
        locking_transform(problem, selA, selH, oldH);

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
            locking_transform(problem, selA, oldH, selH);
        }

        /* Termination. */
        if (iteration >= maxIteration) break;
    }
}

/* Performs a transform that simultaneously locks hardware nodes during the
 * transformation to avoid data races. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::locking_transform(Problem& problem,
    decltype(Problem::nodeAs)::iterator& selA,
    decltype(Problem::nodeHs)::iterator& selH,
    decltype(Problem::nodeHs)::iterator& oldH)
{
    /* Identify where the locks are (hold them as references) */
    decltype(Problem::lockHs)::value_type& selHLock =
        problem.lockHs[selH - problem.nodeHs.begin()];
    decltype(Problem::lockHs)::value_type& oldHLock =
        problem.lockHs[oldH - problem.nodeHs.begin()];

    /* Lock them simultaneously. */
    std::lock(selHLock, oldHLock);

    /* Unlock them together (non-simultaneously) at the end of the
     * transformation. */
    std::lock_guard<decltype(Problem::lockHs)::value_type> selHGuard
        (selHLock, std::adopt_lock);
    std::lock_guard<decltype(Problem::lockHs)::value_type> oldHGuard
        (oldHLock, std::adopt_lock);

    /* Perform the transformation. */
    problem.transform(selA, selH, oldH);
}

#include "parallel_annealer-impl.hpp"
