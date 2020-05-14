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
 * Does not modify the state in any way. Swap selection not supported yet. */
void Problem::select(std::vector<std::shared_ptr<NodeA>>::iterator& selA,
                     std::vector<std::shared_ptr<NodeH>>::iterator& selH)
{
    /* Application node */
    selA = nodeAs.begin();
    std::uniform_int_distribution<
        std::vector<std::shared_ptr<NodeA>>::size_type>
        distributionSelA(0, nodeAs.size() - 1);
    std::advance(selA, distributionSelA(rng));

    /* Hardware node. Reselect if the hardware node selected is full. This
     * extra functionality isn't needed if a swap operation is defined, and is
     * pretty inefficient if the application graph "just fits" in the hardware
     * graph. If this is the case, consider increasing pMax instead. */
    do
    {
        selH = nodeHs.begin();
        std::uniform_int_distribution<
            std::vector<std::shared_ptr<NodeH>>::size_type>
            distributionSelH(0, nodeHs.size() - 1);
        std::advance(selH, distributionSelH(rng));
    } while ((*selH)->contents.size() != pMax);

    /* Application node in hardware node - if the number we select is greater
     * than the number of elements currently attached to the hardware node, we
     * do a move operation. Otherwise, we do a swap operation with the
     * application node with index equal to the randomly selected number.
     *
     * Except that we're not doing swap selection for now. I've left this here
     * anyway in case its useful later.
    std::uniform_int_distribution<unsigned> distributionSelHA(0, pMax);
    auto roll = distributionSelHA(rng);
    if (roll >= (*selH)->contents.size()) selHA = (*selH)->contents.end();
    else
    {
        selHA = (*selH)->contents.begin();
        std::advance(selHA, roll);
    }
    */
}

/* Transforms the state by moving the selected application node to the selected
 * hardware node. The iterators pass as arguments are unchanged, and are not
 * checked for validity. */
void Problem::transform(std::vector<std::shared_ptr<NodeA>>::iterator& selA,
                        std::vector<std::shared_ptr<NodeH>>::iterator& selH)
{
    /* Remove this application node from its current hardware node. Note that
     * the shared pointers will not be emptied - they are only emptied on
     * destruction of the Problem object. */
    (*selA)->location.lock()->contents.remove_if(
        [selA](std::weak_ptr<NodeA> cmp){return (*selA) == cmp.lock();});

    /* Assign the selected hardware node to the location field of the selected
     * application node. */
    (*selA)->location = std::weak_ptr(*selH);

    /* Append the selected application node to the contents field of the
     * selected hardware node. */
    (*selH)->contents.push_back(std::weak_ptr(*selA));
}

/* Computes and returns the locality fitness associated with a given
 * application node. Note that locality fitness, in terms of the problem
 * specification, is associated with an edge. Since all edges in this
 * implementation are "double-counted", this method divides the fitness
 * contribution by half. */
float Problem::compute_app_node_locality_fitness(NodeA& nodeA)
{
    float returnValue = 0;

    /* Hardware node index associated with this application node */
    auto rootHIndex = nodeA.location.lock()->index;

    /* Edge cache row for this hardware node (to avoid getting it multiple
     * times) */
    auto edgeCacheRow = edgeCacheH[rootHIndex];

    /* Iterate over each application node. */
    for (auto neighbourPtr : nodeA.neighbours)
    {
        /* Get the hardware node index associated with that neighbour. */
        auto neighbourHIndex = neighbourPtr.lock()->location.lock()->index;

        /* Impose fitness penalty from the edgeCache. */
        returnValue -= edgeCacheRow[neighbourHIndex];
    }

    return returnValue;
}

/* Computes and returns the clustering fitness associated with a given hardware
 * node. */
float Problem::compute_hw_node_clustering_fitness(NodeH& nodeH)
{
    auto size = nodeH.contents.size();
    return -size * size;
}

/* Computes and returns the total fitness of the current mapping for this
 * solution. */
float Problem::compute_total_fitness()
{
    float returnValue = 0;

    /* Locality fitness! */
    for (auto nodeA : nodeAs)
        returnValue += compute_app_node_locality_fitness(*nodeA);

    /* Clustering fitness! */
    for (auto nodeH : nodeHs)
        returnValue += compute_hw_node_clustering_fitness(*nodeH);

    return returnValue;
}
