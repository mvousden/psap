#include "parallel_annealer.hpp"

#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

template<class DisorderT>
ParallelAnnealer<DisorderT>::ParallelAnnealer(
    unsigned numThreadsArg,
    Iteration maxIterationArg,
    const std::filesystem::path& outDirArg,
    Seed disorderSeed):
    Annealer<DisorderT>(maxIterationArg, outDirArg, "ParallelAnnealer",
                        disorderSeed),
    numThreads(numThreadsArg){}

/* Hits the solution repeatedly with many hammers at the same time while
 * cooling it. Hopefully improves it (the jury's out).
 *
 * The recordEvery argument, if non-zero, causes the annealing to stop after
 * every "recordEvery" iterations to compute and record the fitness at that
 * time if logging is enabled (i.e. if outDirArg is nonempty in the
 * constructor).
 *
 * The parallel annealer has two synchronising modes: "synchronous" and
 * "semi-asynchronous". The former anneals, ensuring correct fitness
 * computation. The latter anneals with stale fitness data (and is hopefully
 * faster). The "synchronous" mode is used if `fullySynchronous` is true, and
 * the "semi-asynchronous" mode is used otherwise. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::anneal(Problem& problem,
                                         Iteration recordEvery,
                                         bool fullySynchronous)
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
            csvOuts.at(csvOutIndex).open(this->outDir / csvPath.str(),
                std::ofstream::trunc);
            csvOuts.at(csvOutIndex) << "Iteration,"
                                    << "Selected application node index,"
                                    << "Selected hardware node index,"
                                    << "Number of selection collisions,"
                                    << "Transformed Fitness,"
                                    << "Transformed Clustering Fitness,"
                                    << "Transformed Locality Fitness,"
                                    << "Fitness computation is reliable,"
                                    << "Determination\n";
        }

        /* Frequent serial fitness computation. */
        if (recordEvery != 0)
        {
            csvOutMaster.open(this->outDir / fitnessPath,
                              std::ofstream::trunc);
            csvOutMaster << "Iteration,"
                         << "Fitness,"
                         << "Clustering Fitness,"
                         << "Locality Fitness\n";
        }

        /* Wallclock measurement. */
        clockOut.open(this->outDir / clockPath, std::ofstream::trunc);

        /* Metadata */
        write_metadata();
    }

    /* If we're doing periodic fitness updates, throw one in before starting to
     * anneal. An expansive scope matters here. */
    float clusteringFitness;
    float localityFitness;
    if (this->log and recordEvery != 0)
    {
        clusteringFitness = problem.compute_total_clustering_fitness();
        localityFitness = problem.compute_total_locality_fitness();
        csvOutMaster << iteration << ","
                     << clusteringFitness + localityFitness << ","
                     << clusteringFitness << ","
                     << localityFitness << std::endl;
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
        /* Compute next stopping point. Don't stop if we're not logging
         * (because there would be no point). */
        Iteration nextStop;
        if (recordEvery == 0 or (this->log == false))
        {
            nextStop = this->maxIteration;
        }
        else nextStop = std::min(this->maxIteration, iteration + recordEvery);

        /* Measure wallclock time between now and when the threads are joined
         * with. */
        auto timeAtStart = std::chrono::steady_clock::now();

        /* Spawn slave threads to do the annealing. */
        std::vector<std::thread> threads;
        for (unsigned threadId = 0; threadId < numThreads; threadId++)
        {
            if (fullySynchronous)
            {
                threads.emplace_back(
                    &ParallelAnnealer<DisorderT>::co_anneal_synchronous,
                    this, std::ref(problem), std::ref(csvOuts.at(threadId)),
                    nextStop, clusteringFitness, localityFitness);
            }
            else
            {
                threads.emplace_back
                    (&ParallelAnnealer<DisorderT>::co_anneal_sasynchronous,
                     this, std::ref(problem), std::ref(csvOuts.at(threadId)),
                     nextStop, clusteringFitness, localityFitness);
            }
        }

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
            clusteringFitness = problem.compute_total_clustering_fitness();
            localityFitness = problem.compute_total_locality_fitness();
            csvOutMaster << iteration << ","
                         << clusteringFitness + localityFitness << ","
                         << clusteringFitness << ","
                         << localityFitness << std::endl;

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

/* An individual hammer, to be wielded by a single thread. Communicates with
 * other threads semi-asynchronously. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::co_anneal_sasynchronous(
    Problem& problem, std::ofstream& csvOut, Iteration maxIteration,
    float oldClusteringFitness, float oldLocalityFitness)
{
    auto selA = problem.nodeAs.begin();
    auto selH = problem.nodeHs.begin();
    auto oldH = problem.nodeHs.begin();

    /* Base fitness "used" from the start of each iteration. Note that the
     * currently-stored fitness will drift from the total fitness. This is fine
     * because determination only care about the fitness difference from an
     * operation - not its absolute value or ratio. */
    auto oldFitness = oldClusteringFitness + oldLocalityFitness;

    /* Really, nobody cares about the initial fitness value, but it's
     * interesting to watch it change. */
    if (this->log) csvOut << "-1,-1,-1,0,"
                          << oldFitness << ","
                          << oldClusteringFitness << ","
                          << oldLocalityFitness << ",1,1\n";

    Iteration localIteration;
    do
    {
        /* Get the iteration number. The variable "iteration" is shared between
         * threads. */
        localIteration = iteration++;
        if (this->log) csvOut << localIteration << ",";
        unsigned selectionCollisions = 0;
        TransformCount oldTformFootprint;
        float oldClusteringFitnessComponents;
        float oldLocalityFitnessComponents;
        bool goAroundAgain;

        do
        {
            /* "Atomic" selection */
            selectionCollisions +=
                problem.select_parallel_sasynchronous(selA, selH, oldH);

            /* Compute the transformation footprint, so that we can identify
             * whether or not the fitness computation is reliable (it is
             * unreliable if another thread changes a relevant bit of the
             * datastructure. We don't do anything different if it is
             * unreliable outside of logging the occurence in the output).
             *
             * Since the only use of transform footprints are logging-related
             * (behaviour doesn't change if we detect an unreliable fitness
             * computation), we only do this if we're logging. */
            if (this->log) oldTformFootprint = compute_transform_footprint(
                selA, selH, oldH);

            /* Fitness of components before transformation. */
            oldClusteringFitnessComponents =
                problem.compute_hw_node_clustering_fitness(**selH) +
                problem.compute_hw_node_clustering_fitness(**oldH);

            oldLocalityFitnessComponents =
                problem.compute_app_node_locality_fitness(**selA) * 2;

            /* Transformation. If it's now invalid, go to the next one. */
            if(!locking_transform(problem, selA, selH, oldH))
            {
                (*selA)->lock.unlock();
                selectionCollisions++;
                goAroundAgain = true;
            }
            else goAroundAgain = false;
        }
        while(goAroundAgain);

        /* RAII locking */
        std::lock_guard<decltype((*selA)->lock)> appLock((*selA)->lock,
                                                         std::adopt_lock);

        if (this->log) csvOut << selA - problem.nodeAs.begin() << ","
                              << selH - problem.nodeHs.begin() << ","
                              << selectionCollisions << ",";

        /* Fitness of components after transformation. */
        auto newClusteringFitnessComponents =
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        auto newLocalityFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2;

        /* Footprint after transformation. Note the minus three - this is
         * because our move transformation causes three changes to the data
         * structure, and we don't want to count those. */
        TransformCount newTformFootprint = 0;
        if (this->log) newTformFootprint = compute_transform_footprint(
            selA, selH, oldH) - 3;

        /* New fitness computation. */
        auto newClusteringFitness = oldClusteringFitness -
            oldClusteringFitnessComponents + newClusteringFitnessComponents;

        auto newLocalityFitness = oldLocalityFitness -
            oldLocalityFitnessComponents + newLocalityFitnessComponents;

        auto newFitness = newLocalityFitness + newClusteringFitness;

        /* Writing new fitness value to CSV, and whether or not the fitness
         * computation this iteration is unreliable. */
        if (this->log) csvOut << newFitness << ","
                              << newClusteringFitness << ","
                              << newLocalityFitness << ","
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
            oldClusteringFitness = newClusteringFitness;
            oldLocalityFitness = newLocalityFitness;
        }

        else
        {
            if (this->log) csvOut << 0 << '\n';
            locking_transform(problem, selA, oldH, selH);
        }
    }
    while (iteration < maxIteration);  /* Termination condition */
}

/* An individual hammer, to be wielded by a single thread. Communicates with
 * other threads synchronously.
 *
 * Shameless code duplication with the semi-asynchronous co-anneal method, but
 * life is short. */
template<class DisorderT>
void ParallelAnnealer<DisorderT>::co_anneal_synchronous(
    Problem& problem, std::ofstream& csvOut, Iteration maxIteration,
    float oldClusteringFitness, float oldLocalityFitness)
{
    auto selA = problem.nodeAs.begin();
    auto selH = problem.nodeHs.begin();
    auto oldH = problem.nodeHs.begin();

    /* Base fitness "used" from the start of each iteration. Note that the
     * currently-stored fitness will drift from the total fitness. This is fine
     * because determination only care about the fitness difference from an
     * operation - not its absolute value or ratio. */
    auto oldFitness = oldClusteringFitness + oldLocalityFitness;

    /* Really, nobody cares about the initial fitness value, but it's
     * interesting to watch it change. */
    if (this->log) csvOut << "-1,-1,-1,0,"
                          << oldFitness << ","
                          << oldClusteringFitness << ","
                          << oldLocalityFitness << ",1,1\n";

    Iteration localIteration;
    do
    {
        /* Get the iteration number. The variable "iteration" is shared between
         * threads. */
        localIteration = iteration++;
        if (this->log) csvOut << localIteration << ",";

        /* "Atomic" selection */
        auto selectionCollisions = \
            problem.select_parallel_synchronous(selA, selH, oldH);
        if (this->log) csvOut << selA - problem.nodeAs.begin() << ","
                              << selH - problem.nodeHs.begin() << ","
                              << selectionCollisions << ",";

        /* RAII locking - selected application node, selected hardware node,
         * old hardware node, and neighbouring application nodes. We're just
         * adopting the previously-locked nodes here. */
        std::vector<std::unique_lock<decltype((*selA)->lock)>> appLocks;
        appLocks.emplace_back((*selA)->lock, std::adopt_lock);
        appLocks.emplace_back((*selH)->lock, std::adopt_lock);
        appLocks.emplace_back((*oldH)->lock, std::adopt_lock);
        for (const auto& neighbour : (*selA)->neighbours)
            appLocks.emplace_back(neighbour.lock()->lock, std::adopt_lock);

        /* Compute the transformation footprint. Only done when logging, and
         * only done to demonstrate that there are no collisions when annealing
         * synchonously. */
        TransformCount oldTformFootprint = 0;
        if (this->log) oldTformFootprint = compute_transform_footprint(
            selA, selH, oldH);

        /* Fitness of components before transformation. */
        auto oldClusteringFitnessComponents =
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        auto oldLocalityFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2;

        /* Transformation. Note it's not a locking transform, because we've
         * already claimed our locks for this iteration. */
        problem.transform(selA, selH, oldH);

        /* Increment transformation counters, to be sporting. */
        (*selA)->transformCount++;
        (*selH)->transformCount++;
        (*oldH)->transformCount++;

        /* Fitness of components after transformation. */
        auto newClusteringFitnessComponents =
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(**oldH);

        auto newLocalityFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) * 2;

        /* Footprint after transformation. Note the minus three - this is
         * because our move transformation causes three changes to the data
         * structure, and we don't want to count those. */
        TransformCount newTformFootprint = 0;
        if (this->log) newTformFootprint = compute_transform_footprint(
            selA, selH, oldH) - 3;

        /* New fitness computation. */
        auto newClusteringFitness = oldClusteringFitness -
            oldClusteringFitnessComponents + newClusteringFitnessComponents;

        auto newLocalityFitness = oldLocalityFitness -
            oldLocalityFitnessComponents + newLocalityFitnessComponents;

        auto newFitness = newLocalityFitness + newClusteringFitness;

        /* Writing new fitness value to CSV, and whether or not the fitness
         * computation this iteration is unreliable. */
        if (this->log) csvOut << newFitness << ","
                              << newClusteringFitness << ","
                              << newLocalityFitness << ","
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
            oldClusteringFitness = newClusteringFitness;
            oldLocalityFitness = newLocalityFitness;
        }

        else
        {
            if (this->log) csvOut << 0 << '\n';
            problem.transform(selA, oldH, selH);
        }
    }
    while (iteration < maxIteration);  /* Termination condition */
}

/* Computes the transformation footprint from the set of nodes that are used
 * for a transformation.
 *
 * This footprint will be incremented by the number of nodes affected by the
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
 * problem.transform. Rejects the transformation outright if the target
 * hardware node is now full - if the transformation is rejected in this way,
 * false is returned (as opposed to true). */
template<class DisorderT>
bool ParallelAnnealer<DisorderT>::locking_transform(Problem& problem,
    decltype(Problem::nodeAs)::iterator& selA,
    decltype(Problem::nodeHs)::iterator& selH,
    decltype(Problem::nodeHs)::iterator& oldH)
{
    /* Identify where the locks are (hold them as references) */
    decltype(NodeH::lock)& selHLock = (*selH)->lock;
    decltype(NodeH::lock)& oldHLock = (*oldH)->lock;

    /* Lock them simultaneously. */
    std::lock(selHLock, oldHLock);

    /* Unlock them together (non-simultaneously) at end of scope. */
    std::lock_guard<decltype(selHLock)> selHGuard(selHLock, std::adopt_lock);
    std::lock_guard<decltype(oldHLock)> oldHGuard(oldHLock, std::adopt_lock);

    /* Check hardware node fullness. */
    if ((*selH)->contents.size() >= problem.pMax)
    {
        printf("Oh no\n");
        return false;
    }

    /* Increment transformation counters. */
    (*selA)->transformCount++;
    (*selH)->transformCount++;
    (*oldH)->transformCount++;

    /* Perform the transformation. */
    problem.transform(selA, selH, oldH);
    return true;
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
        metadata.open(this->outDir / this->metadataName, std::ofstream::app);
        metadata << "threadCount = " << numThreads;
        metadata.close();
    }
}

#include "parallel_annealer-impl.hpp"
