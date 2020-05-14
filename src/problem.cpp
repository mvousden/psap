#include "problem.hpp"

Problem::Problem()
{
    /* Define our random number generator. */
    rng = std::mt19937(std::random_device{}());
}

/* Reserve space in the edge cache as a function of the diameter, and define
 * default values as per the specification - zeroes on the diagonals, and a
 * huge number everywhere else. */
void Problem::initialise_edge_cache(unsigned diameter)
{
    std::vector<std::vector<float>>::size_type eOuterIndex;
    std::vector<float>::size_type eInnerIndex;
    for (eOuterIndex = 0; eOuterIndex < diameter; eOuterIndex++)
    {
        edgeCacheH.push_back(
            std::vector<float>(diameter, std::numeric_limits<float>::max()));
        auto inner = edgeCacheH[eOuterIndex];
        for (eInnerIndex = 0; eInnerIndex < diameter; eInnerIndex++)
            if (eOuterIndex == eInnerIndex) inner[eInnerIndex] = 0;
    }
}

/* Populate the infinite members of the edge cache, using the Floyd-Warshall
 * algorithm. Requires the edge cache to be first initialised and, for the
 * result to be meaningful, populated with edge data. */
void Problem::populate_edge_cache()
{
    unsigned i, j, k;
    float trialPathWeight;
    auto size = edgeCacheH.size();

    for (k = 0; k < size; k++)
        for (i = 0; i < size; i++)
            for (j = 0; j < size; j++)
            {
                trialPathWeight = edgeCacheH[i][k] + edgeCacheH[k][j];
                edgeCacheH[i][j] = std::max(edgeCacheH[i][j], trialPathWeight);
            }
}

/* Defines an initial state for the annealer, by populating the location field
 * in each application node, and the contents field in each hardware
 * node. Assignments of application nodes to hardware nodes is done at random,
 * but data structure integrity is not compromised. This method also respects
 * the pMax field defined in the problem.
 *
 * This initialiser assumes that the aforementioned fields have not been
 * defined. */
void Problem::initial_condition_random()
{
    /* To make random selection faster, define a container of hardware nodes
     * that can fit more application nodes in them. Elements will leave this
     * container as they become populated. */
    std::list<std::weak_ptr<NodeH>> nonEmpty;

    /* Populate the container with all nodes. */
    for (auto hNode : nodeHs) nonEmpty.push_back(hNode);

    /* Likewise for application nodes, though we don't select from this
     * container - we shuffle it. */
    std::vector<std::weak_ptr<NodeA>> toPlace;
    for (auto aNode : nodeAs) toPlace.push_back(aNode);
    std::shuffle(toPlace.begin(), toPlace.end(), rng);

    /* Place each application node in turn. */
    for (auto selA : toPlace)
    {
        /* Select a hardware node at random that is not yet full. */
        auto selH = nonEmpty.begin();
        std::uniform_int_distribution<
            std::list<std::weak_ptr<NodeH>>::size_type>
            distribution(0, nonEmpty.size() - 1);
        std::advance(selH, distribution(rng));

        /* Map */
        selA.lock()->location = *selH;
        selH->lock()->contents.push_back(selA);

        /* Remove the hardware node if it is full. */
        if (selH->lock()->contents.size() >= pMax) nonEmpty.erase(selH);
    }
}

/* Selects:
 *
 *  - One application node at random, and places it in selA.
 *
 *  - One hardware node at random, and places it in selH.
 *
 *  - Either one application node attached to the selected hardware node at
 *    random and places it in selHA (swap), or sets selHA to the end of the
 *    contents container for that hardware node (move).
 *
 * Does not modify the state in any way. */
void Problem::select(std::vector<std::shared_ptr<NodeA>>::iterator& selA,
                     std::vector<std::shared_ptr<NodeH>>::iterator& selH,
                     std::list<std::weak_ptr<NodeA>>::iterator& selHA)
{
    /* Application node */
    selA = nodeAs.begin();
    std::uniform_int_distribution<
        std::vector<std::shared_ptr<NodeA>>::size_type>
        distributionSelA(0, nodeAs.size() - 1);
    std::advance(selA, distributionSelA(rng));

    /* Hardware node */
    selH = nodeHs.begin();
    std::uniform_int_distribution<
        std::vector<std::shared_ptr<NodeH>>::size_type>
        distributionSelH(0, nodeHs.size() - 1);
    std::advance(selH, distributionSelH(rng));

    /* Application node in hardware node - if the number we select is greater
     * than the number of elements currently attached to the hardware node, we
     * do a move operation. Otherwise, we do a swap operation with the
     * application node with index equal to the randomly selected number. */
    std::uniform_int_distribution<unsigned> distributionSelHA(0, pMax);
    auto roll = distributionSelHA(rng);
    if (roll >= (*selH)->contents.size()) selHA = (*selH)->contents.end();
    else
    {
        selHA = (*selH)->contents.begin();
        std::advance(selHA, roll);
    }
}

/* Transforms the state in accordance with the selected iterators:
 *
 * 1. If selHA points to a non-end application node, moves the application node
 *    to the hardware node selA is attached to (swap only).
 *
 * 2. Moves selA to selH. */
void Problem::transform(std::vector<std::shared_ptr<NodeA>>::iterator& selA,
                        std::vector<std::shared_ptr<NodeH>>::iterator& selH,
                        std::list<std::weak_ptr<NodeA>>::iterator& selHA)
{
    /* Do the second-half of the swap operation first. Does nothing if we're
     * doing a move operation */
    if (selHA != (*selH)->contents.end())
    {
        (*selA)->location.lock()->contents.push_back(*selHA);
        (*selHA).lock()->location = (*selA)->location;
    }

    /* Do the move (or the move-component of the swap) */
    (*selA)->location = std::weak_ptr(*selH);
    (*selH)->contents.push_back(std::weak_ptr(*selA));
}
