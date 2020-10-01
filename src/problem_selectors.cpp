/* Methods defined in TU have names that follow either syntax:
 *
 *  select_<SYNC_TYPE> (or) select_<SYNC_TYPE>_<NODE>
 *
 * Where <SYNC_TYPE> can be:
 *
 * - `serial`: No locking or counting is done.
 *
 * - `parallel_sasynchronous`: Only the selected application node is locked
 *   to prevent data races. Any hardware node can be selected.
 *
 * - `parallel_synchronous`: The selected application node, its neighbours, its
 *   "old" hardware node, and the selected hardware node are all locked.
 *
 * and where <NODE> can be:
 *
 * - `sela`: Selection of an application node.
 *
 * - `oldh`: Retrival of a hardware node associated with the selected
 *   application node, before transformation.
 *
 * - `selh`: Selection of a hardware node.
 *
 * Note that, due to the (>1) number of nodes that need to be locked in the
 * synchronous approach, there is no parallel_synchronous_sela (for
 * example). With that, let's begin. */

#include "problem.hpp"

/* Serial selection. Selects:
 *
 * - One application node at random, and places it in `selA`.
 *
 * - One hardware node at random, and places it in `selH`.
 *
 * - Retrieves `oldH` given `selA` (for convenience).
 *
 * Does not modify the state of the problem in any way. Returns zero. */
unsigned Problem::select_serial(decltype(nodeAs)::iterator& selA,
                                decltype(nodeHs)::iterator& selH,
                                decltype(nodeHs)::iterator& oldH)
{
    select_serial_sela(selA);
    select_serial_oldh(selA, oldH);
    select_serial_selh(selH, oldH);
    return 0;
}

/* Serial selection of an application node at random. */
void Problem::select_serial_sela(decltype(nodeAs)::iterator& selA)
{
    selA = nodeAs.begin();
    std::uniform_int_distribution<decltype(nodeAs)::size_type>
        distributionSelA(0, nodeAs.size() - 1);
    std::advance(selA, distributionSelA(rng));
}

/* Retrieval of old hardware node given the application node. */
void Problem::select_serial_oldh(decltype(nodeAs)::iterator& selA,
                                 decltype(nodeHs)::iterator& oldH)
{
    oldH = nodeHs.begin() + (*selA)->location.lock()->index;
}

/* Selection of a hardware node, avoiding selection of a certain node (to avoid
 * selecting the old hardware node again!) */
void Problem::select_serial_selh(decltype(nodeHs)::iterator& selH,
                                 decltype(nodeHs)::iterator& avoid)
{
    /* Reselect if the hardware node selected is full, or if it already
     * contains the application node. This extra functionality becomes
     * inefficient as application graph "just fits" in the hardware graph. If
     * this is the case, consider increasing pMax instead. */
    auto attempt = Problem::selectionPatience;
    do
    {
        attempt--;
        if (attempt == 0)
        {
            log("WARNING: Hardware node selection is taking a while. Try "
                "setting a larger value for pMax.");
        }
        selH = nodeHs.begin();
        std::uniform_int_distribution<decltype(nodeHs)::size_type>
            distributionSelH(0, nodeHs.size() - 1);
        std::advance(selH, distributionSelH(rng));
    } while ((*selH)->contents.size() >= pMax or selH == avoid);
}

/* Parallel semi-asynchronous selection. Selects:
 *
 * - One application node at random, places it in `selA`, and locks it.
 *
 * - One hardware node at random, and places it in `selH`.
 *
 * - Retrieves `oldH` given `selA` (for convenience).
 *
 * Does not modify the state of the problem in any way. Returns the number of
 * collisions encountered when selecting the application node. */
unsigned Problem::select_parallel_sasynchronous(
    decltype(nodeAs)::iterator& selA,
    decltype(nodeHs)::iterator& selH,
    decltype(nodeHs)::iterator& oldH)
{
    unsigned output = select_parallel_sasynchronous_sela(selA);
    select_parallel_sasynchronous_oldh(selA, oldH);
    select_parallel_sasynchronous_selh(selH, oldH);
    return output;
}

/* Selection of an application node in an atomic manner, so that two threads
 * cannot "claim" the same application node at the same time.
 *
 * Logs if the while loop goes on for longer than is reasonable (1000
 * iterations, for now).
 *
 * Worth noting that the caller needs to unlock the mutex once they're done
 * with it.
 *
 * Returns the number of selection attempts. */
unsigned Problem::select_parallel_sasynchronous_sela(
    decltype(nodeAs)::iterator& selA)
{
    /* Select index for the application node, until we hit one that's not been
     * claimed already. */
    std::uniform_int_distribution<decltype(nodeAs)::size_type>
        distributionSelA(0, nodeAs.size() - 1);
    decltype(nodeAs)::size_type roll;
    auto attempt = Problem::selectionPatience;
    do
    {
        attempt--;
        if (attempt == 0)
        {
            log("WARNING: Atomic application node selection is taking a "
                "while. Try spawning fewer threads.");
        }
        roll = distributionSelA(rng);
    }
    while (!nodeAs.at(roll)->lock.try_lock());

    /* Put the iterator in the right place. */
    selA = nodeAs.begin();
    std::advance(selA, roll);

    return static_cast<unsigned>(Problem::selectionPatience - attempt - 1);
}

/* Wouldn't you know it. */
void Problem::select_parallel_sasynchronous_oldh(
    decltype(nodeAs)::iterator& selA,
    decltype(nodeHs)::iterator& oldH)
{select_serial_oldh(selA, oldH);}
void Problem::select_parallel_sasynchronous_selh(
    decltype(nodeHs)::iterator& selH,
    decltype(nodeHs)::iterator& avoid)
{select_serial_selh(selH, avoid);}

/* Parallel synchronous selection. Selects:
 *
 * - One application node at random, places it in `selA`, and locks it and its
 *   neighbours.
 *
 * - One hardware node at random, places it in `selH`, and locks it.
 *
 * - Retrieves `oldH` given `selA` (for convenience), and locks it.
 *
 * Does not modify the state of the problem in any way. Returns the number of
 * collisions encountered due to locks owned by other threads. */
unsigned Problem::select_parallel_synchronous(decltype(nodeAs)::iterator& selA,
                                              decltype(nodeHs)::iterator& selH,
                                              decltype(nodeHs)::iterator& oldH)
{
    /* So this one is a bit messy - we have to check the locks for several
     * nodes at the same time in order to avoid races. */

    /* Roll the dice to select an application node. */
    std::uniform_int_distribution<decltype(nodeAs)::size_type>
        distributionSelA(0, nodeAs.size() - 1);
    decltype(nodeAs)::size_type roll;
    auto appAttempt = Problem::selectionPatience;

    while (true)
    {
        appAttempt--;
        if (appAttempt == 0)
        {
            log("WARNING: Synchronous application node selection is taking a "
                "while. Try spawning fewer threads.");
        }
        roll = distributionSelA(rng);
        selA = nodeAs.begin();
        std::advance(selA, roll);

        /* Locking is a little complicated. We need to lock the application
         * node before we determine its location. Once determined, we then
         * attempt to lock the location-hardware node, and the neighbours of
         * the selected application node. If any of those locks are already
         * claimed, we "undo" all of the locks, including the first. */

        /* Give up if the selected application node is locked. */
        if (!(*selA)->lock.try_lock()) continue;

        /* Now we've selected the application node, collect the other nodes we
         * wish to lock. */
        std::vector<std::weak_ptr<Node>> nodesToLock;

        /* Firstly, each of the selected application node's neighbours. */
        for (const auto& neighbour : (*selA)->neighbours)
            nodesToLock.push_back(neighbour);

        /* Secondly, the old hardware node. */
        select_serial_oldh(selA, oldH);
        nodesToLock.push_back((*oldH));

        /* For each of these nodes, try to unlock them in turn. If any of them
         * don't work when tried, unlock all of the ones that we managed to
         * lock, and loop around again.
         *
         * Note, I would like to use the variadic std::try_lock here, but since
         * we don't know the number of neighbouring nodes at compile time,
         * we're a little stuck, yo. */
        bool failure = false;
        unsigned locksMade = 0;
        for (const auto& thisNode : nodesToLock)
        {
            if (thisNode.lock()->lock.try_lock()) locksMade++;
            else
            {
                failure = true;
                break;
            }
        }

        /* If we got what we came for, don't iterate any more. */
        if (!failure) break;

        /* Otherwise, undo all of the locks we took, and iterate again. */
        decltype(nodesToLock)::iterator nodeToUnlock = nodesToLock.begin();
        while (locksMade > 0)
        {
            nodeToUnlock->lock()->lock.unlock();
            locksMade--;
            nodeToUnlock++;
        }
        (*selA)->lock.unlock();
    }

    /* Now deal with hardware node selection. On selecting the hardware node at
     * random, we reselect if the hardware node is full, if it already contains
     * the hardware node, or has been locked (either by us [oldH], or by
     * another thread). */
    auto hwSizeAttempt = Problem::selectionPatience;
    decltype(hwSizeAttempt) hwLockTotalAttempts = 0;
    while (true)
    {
        hwSizeAttempt--;
        if (hwSizeAttempt == 0)
        {
            log("WARNING: Synchronous hardware node selection is taking a "
                "while. Try spawning fewer threads.");
        }

        auto hwLockAttempt = Problem::selectionPatience;
        do
        {
            hwLockAttempt--;
            if (hwLockAttempt == 0)
            {
                log("WARNING: Synchronous hardware node selection is taking a "
                    "while. Try spawning fewer threads.");
            }

            /* Selection */
            selH = nodeHs.begin();
            std::uniform_int_distribution<decltype(nodeHs)::size_type>
                distributionSelH(0, nodeHs.size() - 1);
            std::advance(selH, distributionSelH(rng));
        }
        /* Keep selecting until we find a node that's not in use. */
        while (!(*selH)->lock.try_lock());
        hwLockTotalAttempts += Problem::selectionPatience - hwLockAttempt;

        /* Try again if the node is invalid for another reason. */
        if ((*selH)->contents.size() >= pMax or selH == oldH)
            (*selH)->lock.unlock();
        else break;
    }

    /* Phew. */
    return static_cast<unsigned>(Problem::selectionPatience -
                                 appAttempt + hwLockTotalAttempts - 2);
}
