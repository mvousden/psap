#include "parallel_annealer.hpp"

#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

template<class DisorderT>
ParallelAnnealer<DisorderT>::ParallelAnnealer(unsigned numThreadsArg,
                                              Iteration maxIterationArg,
                                              std::filesystem::path outDirArg):
    Annealer<DisorderT>(maxIterationArg, outDirArg, "ParallelAnnealer"),
    numThreads(numThreadsArg){}

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
    if (this->log)
    {
        /* Per-thread logging. */
        for (csvOutIndex = 0; csvOutIndex < csvOuts.size(); csvOutIndex++)
        {
            std::stringstream csvPath;
            csvPath << csvBaseName << "-" << csvOutIndex << ".csv";
            csvOuts.at(csvOutIndex).open(
                (this->outDir / csvPath.str()).u8string().c_str(),
                std::ofstream::trunc);
            csvOuts.at(csvOutIndex) << "Iteration,"
                                    << "Selected application node index,"
                                    << "Selected hardware node index,"
                                    << "Number of selection collisions,"
                                    << "Transformed Fitness,"
                                    << "Fitness computation is reliable,"
                                    << "Determination\n";
        }

        /* Frequent serial fitness computation. */
        if (recordEvery != 0)
        {
            csvOutMaster.open((this->outDir / fitnessPath).u8string().c_str(),
                              std::ofstream::trunc);
            csvOutMaster << "Iteration,Fitness\n";
        }

        /* Wallclock measurement. */
        clockOut.open((this->outDir / clockPath).u8string().c_str(),
                      std::ofstream::trunc);

        /* Metadata */
        write_metadata();
    }

    /* If we're doing periodic fitness updates, throw one in before starting to
     * anneal. */
    if (this->log and recordEvery != 0)
    {
        csvOutMaster << iteration << ","
                     << problem.compute_total_fitness() << std::endl;
    }

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
        if (recordEvery == 0) nextStop = this->maxIteration;
        else nextStop = std::min(this->maxIteration, iteration + recordEvery);

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
        if (this->log and recordEvery != 0)
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
    while (iteration < this->maxIteration);

    /* Write wallclock information and close log files. */
    if (this->log)
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
    if (this->log) csvOut << "-1,-1,-1,0," << oldFitness << ",1,1\n";

    Iteration localIteration;
    do
    {
        /* Get the iteration number. The variable "iteration" is shared between
         * threads. */
        localIteration = iteration++;
        if (this->log) csvOut << localIteration << ",";

        /* "Atomic" selection */
        auto selectionCollisions = problem.select(selA, selH, oldH, true);
        if (this->log) csvOut << selA - problem.nodeAs.begin() << ","
                              << selH - problem.nodeHs.begin() << ","
                              << selectionCollisions << ",";

        /* RAII locking */
        std::lock_guard<decltype(NodeA::lock)> appLock((*selA)->lock,
                                                       std::adopt_lock);

        /* Compute the transformation footprint, so that we can identify
         * whether or not the fitness computation is reliable (it is unreliable
         * if another thread changes a relevant bit of the datastructure. We
         * don't do anything different if it is unreliable outside of
         * logging the occurence in the output). */
        TransformCount oldTformFootprint = compute_transform_footprint(
            selA, selH, oldH);

        /* Fitness of components before transformation. */
        auto oldFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2 +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        /* Transformation */
        locking_transform(problem, selA, selH, oldH);

        /* Fitness of components after transformation. */
        auto newFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2 +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        /* Footprint after transformation. Note the minus three - this is
         * because our move transformation causes three changes to the data
         * structure, and we don't want to count those. */
        TransformCount newTformFootprint = compute_transform_footprint(
            selA, selH, oldH) - 3;

        /* New fitness computation. */
        auto newFitness = oldFitness - oldFitnessComponents +
            newFitnessComponents;

        /* Writing new fitness value to csv, and whether or not the fitness
         * computation this iteration is unreliable. */
        if (this->log) csvOut << newFitness << ","
                              << (oldTformFootprint == newTformFootprint)
                              << ",";

        /* Determination */
        bool sufficientlyDetermined =
            this->disorder.determine(oldFitness, newFitness, localIteration);

        /* If the solution was sufficiently determined to be chosen, update the
         * base fitness to support computation for the next
         * iteration. Otherwise, revert the solution. */
        if (sufficientlyDetermined)
        {
            if (this->log) csvOut << 1 << '\n';
            oldFitness = newFitness;
        }

        else
        {
            if (this->log) csvOut << 0 << '\n';
            locking_transform(problem, selA, oldH, selH);
        }
    }
    while (iteration < maxIteration);  /* Termination condition */
}

/* Computes the transformation footprint from the set of notes that are used
 * for a transformation.
 *
 * This footprint will be incremented by the number of notes affected by the
 * transformation after the transformation has been taken place. Footprints are
 * used to determine whether or not nodes concerned in a transformation have
 * been affected by other threads - if the footprint is different, then the
 * state is unreliable. */
template<class DisorderT>
TransformCount ParallelAnnealer<DisorderT>::compute_transform_footprint(
    const decltype(Problem::nodeAs)::iterator& selA,
    const decltype(Problem::nodeHs)::iterator& selH,
    const decltype(Problem::nodeHs)::iterator& oldH)
{
    TransformCount output = 0;

    /* Footprint from hardware nodes. */
    output += (*selH)->transformCount;
    output += (*oldH)->transformCount;

    /* Footprint from application node, and its neighbours. */
    output += (*selA)->transformCount;
    for (const auto& neighbourPtr : (*selA)->neighbours)
    {
        output += neighbourPtr.lock()->transformCount;
    }

    return output;
}

/* Performs a transform that simultaneously locks hardware nodes during the
 * transformation to avoid data races. This is a wrapper around
 * problem.transform. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::locking_transform(Problem& problem,
    decltype(Problem::nodeAs)::iterator& selA,
    decltype(Problem::nodeHs)::iterator& selH,
    decltype(Problem::nodeHs)::iterator& oldH)
{
    /* Identify where the locks are (hold them as references) */
    decltype(NodeH::lock)& selHLock = (*selH)->lock;
    decltype(NodeH::lock)& oldHLock = (*oldH)->lock;

    /* Lock them simultaneously. */
    std::lock(selHLock, oldHLock);

    /* Unlock them together (non-simultaneously) at the end of the
     * transformation. */
    std::lock_guard<decltype(selHLock)> selHGuard(selHLock, std::adopt_lock);
    std::lock_guard<decltype(oldHLock)> oldHGuard(oldHLock, std::adopt_lock);

    /* Increment transformation counters. */
    (*selA)->transformCount++;
    (*selH)->transformCount++;
    (*oldH)->transformCount++;

    /* Perform the transformation. */
    problem.transform(selA, selH, oldH);
}

/* Uses the annealer to write metadata, then appends the number of threads to
 * the file naively.
 *
 * Don't judge, life is too short. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::write_metadata()
{
    Annealer<DisorderT>::write_metadata();
    if (this->log)
    {
        std::ofstream metadata;
        metadata.open((this->outDir / this->metadataName).u8string().c_str(),
                      std::ofstream::app);
        metadata << "threadCount = " << numThreads;
        metadata.close();
    }
}

#include "parallel_annealer-impl.hpp"
