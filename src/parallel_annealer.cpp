#include "parallel_annealer.hpp"

#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

template<class DisorderT>
ParallelAnnealer<DisorderT>::ParallelAnnealer(unsigned numThreadsArg,
                                              Iteration maxIterationArg,
                                              std::filesystem::path outDirArg):
    numThreads(numThreadsArg),
    maxIteration(maxIterationArg),
    disorder(maxIterationArg),
    outDir(outDirArg)
{
    log = !outDir.empty();
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
    /* Set up logging.
     *
     * If no output directory has been defined, then we run without
     * logging. Logging clobbers previous anneals. There are three types of
     * output files created in this way:
     *
     * - A set of CSV files (one for each thread) describing the annealing
     *   operations, where each row corresponds to a new iteration. Note the
     *   use of '\n' over std::endl to reduce flushing (possibly at the cost of
     *   portability). Main will flush everything at the end.
     *
     * - A text file to which the wallclock runtime in seconds is dumped.
     *
     * - A single "master" CSV file for the serial fitness computation, if
     *   recordEvery is nonzero. */
    std::vector<std::ofstream> csvOuts (numThreads);
    std::ofstream csvOutMaster;
    std::ofstream clockOut;
    decltype(csvOuts)::size_type csvOutIndex;
    if (log)
    {
        /* Per-thread logging. */
        for (csvOutIndex = 0; csvOutIndex < csvOuts.size(); csvOutIndex++)
        {
            std::stringstream csvPath;
            csvPath << csvBaseName << "-" << csvOutIndex << ".csv";
            csvOuts.at(csvOutIndex).open(
                (outDir / csvPath.str()).u8string().c_str(),
                std::ofstream::trunc);
            csvOuts.at(csvOutIndex) << "Iteration,"
                                    << "Selected application node index,"
                                    << "Selected hardware node index,"
                                    << "Number of selection collisions,"
                                    << "Transformed Fitness,"
                                    << "Determination\n";
        }

        /* Frequent serial fitness computation. */
        if (recordEvery != 0)
        {
            csvOutMaster.open((outDir / fitnessPath).u8string().c_str(),
                              std::ofstream::trunc);
            csvOutMaster << "Iteration,Fitness\n";
        }

        /* Wallclock measurement. */
        clockOut.open((outDir / clockPath).u8string().c_str(),
                      std::ofstream::trunc);
    }

    /* Initialise problem locking infrastructure */
    problem.initialise_atomic_locks();

    /* Initialise timer in a stupid way. */
    auto now = std::chrono::steady_clock::now();
    auto wallClock = now - now;  /* Zero */

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

        /* Measure wallclock time between now and when the threads are joined
         * with. */
        auto timeAtStart = std::chrono::steady_clock::now();

        /* Spawn slave threads to do the annealing. */
        std::vector<std::thread> threads;
        for (unsigned threadId = 0; threadId < numThreads; threadId++)
            threads.emplace_back(&ParallelAnnealer<DisorderT>::co_anneal, this,
                                 std::ref(problem),
                                 std::ref(csvOuts.at(threadId)), nextStop);

        /* Join with slave threads. */
        for (auto& thread : threads) thread.join();

        /* The other end of the wallclock measurement. */
        wallClock += (std::chrono::steady_clock::now() - timeAtStart);

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

    /* Write wallclock information and close log files. */
    if (log)
    {
        clockOut << std::chrono::duration_cast<std::chrono::seconds>(
            wallClock).count() << std::endl;
        for (auto& csvOut : csvOuts) csvOut.close();
        csvOutMaster.close();
        clockOut.close();
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
     * currently-stored fitness will drift from the total fitness. This is fine
     * because determination only care about the fitness difference from an
     * operation - not its absolute value or ratio. As such, we choose an
     * arbitrary initial value here (because nobody cares about what it is). */
    auto oldFitness = static_cast<float>(-42);

    /* Again, nobody cares about the initial fitness value, but it's
     * interesting to watch it change. */
    if (log) csvOut << "-1,-1,-1,0," << oldFitness << ",1\n";

    Iteration localIteration;
    do
    {
        /* Get the iteration number. The variable "iteration" is shared between
         * threads. */
        localIteration = iteration++;
        if (log) csvOut << localIteration << ",";

        /* "Atomic" selection */
        auto selectionCollisions = problem.select(selA, selH, oldH, true);
        if (log) csvOut << selA - problem.nodeAs.begin() << ","
                        << selH - problem.nodeHs.begin() << ","
                        << selectionCollisions << ",";

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
    }
    while (iteration < maxIteration);  /* Termination condition */
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
