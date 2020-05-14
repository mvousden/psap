#include "problem.hpp"

/* Reserve space in the edge cache as a function of the diameter, and define
 * default values as per the specification - zeroes on the diagonals, and a
 * huge number everywhere else. */
void Problem::initialise_edge_cache(unsigned diameter)
{
    std::vector<std::vector<float>>::size_type eOuterIndex;
    std::vector<float>::size_type eInnerIndex;
    edgeCacheH.reserve(diameter);
    for (eOuterIndex = 0; eOuterIndex < diameter; eOuterIndex++)
    {
        auto inner = edgeCacheH[eOuterIndex];
        inner.reserve(diameter);
        for (eInnerIndex = 0; eInnerIndex < diameter; eInnerIndex++)
        {
            if (eOuterIndex == eInnerIndex) inner[eInnerIndex] = 0;
            else inner[eInnerIndex] = std::numeric_limits<float>::max();
        }
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

    /* Likewise for application nodes. */
    std::list<std::weak_ptr<NodeA>> toPlace;
    for (auto aNode : nodeAs) toPlace.push_back(aNode);

    /* Select a random application node, until there are no more application
     * nodes to place. */
    while (!toPlace.empty())
    {
        /* Select an application node that has not yet been placed. */
        auto selA = toPlace.begin();
        std::sample(toPlace.begin(), toPlace.end(), selA, 1,
                    std::mt19937{std::random_device{}()});

        /* Select a hardware node that is not yet full. */
        auto selH = nonEmpty.begin();
        std::sample(nonEmpty.begin(), nonEmpty.end(), selH, 1,
                    std::mt19937{std::random_device{}()});

        /* Map */
        selA->lock()->location = *selH;
        selH->lock()->contents.push_back(*selA);

        /* Remove the selected application node from the selection, and remove
         * the hardware node if it is full. */
        toPlace.erase(selA);
        if (selH->lock()->contents.size() >= pMax) nonEmpty.erase(selH);
    }
}
