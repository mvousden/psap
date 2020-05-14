#include "problem.hpp"

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
    /* Define our random number generator. */
    std::mt19937 rng(std::random_device{}());

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
