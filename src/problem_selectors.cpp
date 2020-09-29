#include "problem.hpp"

/* Selects:
 *
 *  - One application node at random, and places it in selA.
 *
 *  - One hardware node at random, and places it in selH.
 *
 * Also defines oldH, which is the hardware node that currently contains selA
 * (for convenience).
 *
 * If atomic is true, selection is performed in an atomic way such
 * that no two threads can select the same application node at the same
 * time.
 *
 * Does not modify the state in any way. Swap selection not supported yet.
 *
 * Returns zero if atomic is false, and returns the number of collisions when
 * selecting the application node otherwise. */
unsigned Problem::select(decltype(nodeAs)::iterator& selA,
                         decltype(nodeHs)::iterator& selH,
                         decltype(nodeHs)::iterator& oldH, bool atomic)
{
    unsigned output = 0;
    if (atomic) output = select_sela_atomic(selA); else select_sela(selA);
    select_get_oldh(selA, oldH);
    select_selh(selH, oldH);
    return output;
}

/* Selection of an application node. */
void Problem::select_sela(decltype(nodeAs)::iterator& selA)
{
    selA = nodeAs.begin();
    std::uniform_int_distribution<decltype(nodeAs)::size_type>
        distributionSelA(0, nodeAs.size() - 1);
    std::advance(selA, distributionSelA(rng));
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
unsigned Problem::select_sela_atomic(decltype(nodeAs)::iterator& selA)
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

/* Getting hardware node associated with application node. */
void Problem::select_get_oldh(decltype(nodeAs)::iterator& selA,
                              decltype(nodeHs)::iterator& oldH)
{
    oldH = nodeHs.begin() + (*selA)->location.lock()->index;
}

/* Selection of a hardware node, avoiding selection of a certain node (to avoid
 * duplication) */
void Problem::select_selh(decltype(nodeHs)::iterator& selH,
                          decltype(nodeHs)::iterator& avoid)
{
    /* Reselect if the hardware node selected is full, or if it already
     * contains the application node. This extra functionality isn't needed if
     * a swap operation is defined, and is pretty inefficient if the
     * application graph "just fits" in the hardware graph. If this is the
     * case, consider increasing pMax instead. */
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
