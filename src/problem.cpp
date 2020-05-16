#include "problem.hpp"

#include <algorithm>
#include <fstream>
#include <list>
#include <map>

Problem::Problem()
{
    /* Define our random number generator. */
    rng = std::mt19937(std::random_device{}());
}

/* Reserve space in the edge cache as a function of the diameter, and define
 * default values as per the specification - zeroes on the diagonals, and a
 * huge number everywhere else. Also reads edgeHs to populate entries that
 * have edges. */
void Problem::initialise_edge_cache(unsigned diameter)
{
    std::vector<std::vector<float>>::size_type eOuterIndex;
    std::vector<float>::size_type eInnerIndex;

    /* Populate zeroes and infinites. */
    for (eOuterIndex = 0; eOuterIndex < diameter; eOuterIndex++)
    {
        edgeCacheH.push_back(
            std::vector<float>(diameter, std::numeric_limits<float>::max()));
        for (eInnerIndex = 0; eInnerIndex < diameter; eInnerIndex++)
            if (eOuterIndex == eInnerIndex)
                edgeCacheH[eOuterIndex][eInnerIndex] = 0;
    }

    /* Populate edges. */
    for (auto edge : edgeHs)
    {
        edgeCacheH[std::get<0>(edge)][std::get<1>(edge)] = std::get<2>(edge);
        edgeCacheH[std::get<1>(edge)][std::get<0>(edge)] = std::get<2>(edge);
    }
}

/* Populate the infinite members of the edge cache, using the Floyd-Warshall
 * algorithm. Requires the edge cache to be first initialised and, for the
 * result to be meaningful, populated with edge data. */
void Problem::populate_edge_cache()
{
    auto size = edgeCacheH.size();
    for (decltype(size) k = 0; k < size; k++)
        for (decltype(size) i = 0; i < size; i++)
            for (decltype(size) j = 0; j < size; j++)
            {
                auto trialPathWeight = edgeCacheH[i][k] + edgeCacheH[k][j];
                edgeCacheH[i][j] = std::min(edgeCacheH[i][j], trialPathWeight);
            }
}

/* Defines an initial state for the annealer, but populating the location field
 * in each application node, and the contents field in each hardware
 * node. Application nodes are assigned to hardware nodes in the order they are
 * in the problem stucture; each hardware node is "filled up" to pMax entries
 * in turn.
 *
 * Falls over violently if there are too many application nodes for the
 * hardware graph to hold.
 *
 * This initialiser assumes that the location and contents fields have not been
 * defined. */
void Problem::initial_condition_bucket()
{
    /* Start from the first hardware node. */
    auto selHIt = nodeHs.begin();

    /* Place each application node in turn. */
    for (auto selA : nodeAs)
    {
        /* If the hardware node is full, move to the next one. */
        if ((*selHIt)->contents.size() >= pMax)
            selHIt++;  /* Falls over violently if there are too many
                        * application nodes for the hardware graph to hold. */

        /* Map */
        selA->location = std::weak_ptr(*selHIt);
        (*selHIt)->contents.push_back(std::weak_ptr(selA));
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

    /* Hardware node. Reselect if the hardware node selected is full, or if it
     * already contains the application node. This extra functionality isn't
     * needed if a swap operation is defined, and is pretty inefficient if the
     * application graph "just fits" in the hardware graph. If this is the
     * case, consider increasing pMax instead. */
    do
    {
        selH = nodeHs.begin();
        std::uniform_int_distribution<
            std::vector<std::shared_ptr<NodeH>>::size_type>
            distributionSelH(0, nodeHs.size() - 1);
        std::advance(selH, distributionSelH(rng));
    } while ((*selH)->contents.size() >= pMax or
             (*selA)->location.lock() == (*selH));

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
    float size = nodeH.contents.size();
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

/* Writes, for each application edge, an entry to the CSV file at path passed
 * to by argument connecting nodes in the hardware graph. Each line is of the
 * form '<FROM_H_NODE_NAME>,<TO_H_NODE_NAME>,<COUNT>', where <COUNT> indicates
 * the number of application edges traversing this edge. Any existing file is
 * clobbered. Does no filesystem checking of any kind. */
void Problem::write_a_h_graph(std::string path)
{
    /* The primary data structure for this routine is a map of maps, which
     * represents the sparse matrix of hardware nodes to hardware nodes, where
     * the keys of the maps are the names of the nodes. Note that not all
     * hardware nodes are guaranteed to exist in this map.
     *
     * Note that this map double-counts for non-directional application
     * graphs. */
    std::map<std::string, std::map<std::string, unsigned>> edges;

    for (const auto& nodeA : nodeAs)
    {
        for (auto neighbourPtr : nodeA->neighbours)
        {
            std::string fromHName = nodeA->location.lock()->name;
            std::string toHName = neighbourPtr.lock()->location.lock()->name;

            /* Don't write anything if the two application nodes are on the
             * same hardware node. */
            if (fromHName == toHName) continue;

            /* If the entry in the submap doesn't exist, create it and
             * initialise the value to one. Otherwise, increment it. */
            if (edges[fromHName].find(toHName) == edges[fromHName].end())
                edges[fromHName][toHName] = 1;
            else edges[fromHName][toHName]++;
        }
    }

    /* Write out each entry in the map. */
    std::ofstream out(path, std::ofstream::trunc);
    for (const auto& someEdges : edges)
        for (const auto& edge : someEdges.second)
            out << someEdges.first << ","
                << edge.first << ","
                << edge.second << std::endl;
    out.close();
}

/* Writes the mapping of each application node to their hardware node into the
 * CSV file at path passed to by argument. Each line is of the form
 * '<A_NODE_NAME>,<H_NODE_NAME>'. Any existing file is clobbered. Does no
 * filesystem checking of any kind. */
void Problem::write_a_to_h_map(std::string path)
{
    std::ofstream out(path, std::ofstream::trunc);
    for (const auto& nodeA : nodeAs)
        out << nodeA->name << "," << nodeA->location.lock()->name << std::endl;
    out.close();
}

/* Writes the hardware graph to a CSV file at path passed to by argument. Each
 * line is of the form '<H_NODE_NAME>,<H_NODE_NAME>'. No weights are
 * written. Any existing file is clobbered. Does no filesystem checking of any
 * kind. */
void Problem::write_h_graph(std::string path)
{
    std::ofstream out(path, std::ofstream::trunc);
    for (const auto& edge : edgeHs)
        out << nodeHs[std::get<0>(edge)]->name << ","
            << nodeHs[std::get<1>(edge)]->name << std::endl;
    out.close();
}

/* Writes the loading of each hardware node to the CSV file at path passed to
 * by argument. Each line is of the form
 * '<H_NODE_NAME>,<NUMBER_OF_A_NODES>'. Any existing file is clobbered. Does no
 * filesystem checking of any kind. */
void Problem::write_h_node_loading(std::string path)
{
    std::ofstream out(path, std::ofstream::trunc);
    for (const auto& nodeH : nodeHs)
        out << nodeH->name << "," << nodeH->contents.size() << std::endl;
    out.close();
}
