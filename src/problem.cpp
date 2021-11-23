#include "problem.hpp"

#include <algorithm>
#include <list>

Problem::Problem()
{
    /* Explicitly initialise contents of output path. */
    outDir.clear();
}

Problem::~Problem()
{
    /* Close log file if opened. */
    if (logS.is_open())
    {
        log("Problem destructor called. Closing log.");
        logS.close();
    }
}

/* Sets the seed for the psuedo random number generator, and resets it. */
void Problem::set_seed(Seed seed)
{
    rng = Prng(determine_seed(seed));
}

/* Reserve space in the edge cache as a function of the diameter, and define
 * default values as per the specification - zeroes on the diagonals, and a
 * huge number everywhere else. Also reads edgeHs to populate entries that
 * have edges. */
void Problem::initialise_edge_cache(unsigned diameter)
{
    decltype(edgeCacheH)::size_type eOuterIndex;
    decltype(edgeCacheH)::value_type::size_type eInnerIndex;
    std::stringstream message;
    message << "Initialising hardware edge cache with diameter "
            << diameter << ".";
    log(message.str());

    /* Populate zeroes and infinites. */
    for (eOuterIndex = 0; eOuterIndex < diameter; eOuterIndex++)
    {
        edgeCacheH.push_back(decltype(edgeCacheH)::value_type
                             (diameter, std::numeric_limits<float>::max()));
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

    log("Hardware edge cache initialised.");
}

/* Populate the infinite members of the edge cache, using the Floyd-Warshall
 * algorithm. Requires the edge cache to be first initialised and, for the
 * result to be meaningful, populated with edge data. */
void Problem::populate_edge_cache()
{
    log("Populating edge cache using the Floyd-Warshall algorithm.");
    auto size = edgeCacheH.size();
    for (decltype(size) k = 0; k < size; k++)
        for (decltype(size) i = 0; i < size; i++)
            for (decltype(size) j = 0; j < size; j++)
            {
                auto trialPathWeight = edgeCacheH[i][k] + edgeCacheH[k][j];
                edgeCacheH[i][j] = std::min(edgeCacheH[i][j], trialPathWeight);
            }
    log("Edge cache fully populated.");
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
    log("Applying bucket-filling initial condition.");

    /* Start from the first hardware node. */
    auto selHIt = nodeHs.begin();

    /* Place each application node in turn. */
    for (const auto& selA : nodeAs)
    {
        /* If the hardware node is full, move to the next one. */
        if ((*selHIt)->contents.size() >= pMax)
            selHIt++;  /* Falls over violently if there are too many
                        * application nodes for the hardware graph to hold. */

        /* Map */
        selA->location = std::weak_ptr(*selHIt);
        (*selHIt)->contents.insert(selA.get());
    }

    log("Initial condition applied.");
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
    log("Applying random initial condition.");

    /* To make random selection faster, define a container of hardware nodes
     * that can fit more application nodes in them. Elements will leave this
     * container as they become populated. */
    std::list<std::weak_ptr<NodeH>> nonEmpty;

    /* Populate the container with all nodes. */
    for (const auto& hNode : nodeHs) nonEmpty.push_back(hNode);

    /* Likewise for application nodes, though we don't select from this
     * container - we shuffle it. */
    std::vector<std::weak_ptr<NodeA>> toPlace;
    for (const auto& aNode : nodeAs) toPlace.push_back(aNode);
    std::shuffle(toPlace.begin(), toPlace.end(), rng);

    /* Place each application node in turn. */
    for (const auto& selA : toPlace)
    {
        /* Select a hardware node at random that is not yet full. */
        auto selH = nonEmpty.begin();
        std::uniform_int_distribution<decltype(nonEmpty)::size_type>
            distribution(0, nonEmpty.size() - 1);
        std::advance(selH, distribution(rng));

        /* Map */
        selA.lock()->location = *selH;
        selH->lock()->contents.insert(selA.lock().get());

        /* Remove the hardware node if it is full. */
        if (selH->lock()->contents.size() >= pMax) nonEmpty.erase(selH);
    }

    log("Initial condition applied.");
}

/* Transforms the state by moving the selected application node to the selected
 * hardware node. The iterators pass as arguments are unchanged, and are not
 * checked for validity. */
void Problem::transform(decltype(nodeAs)::iterator& selA,
                        decltype(nodeHs)::iterator& selH,
                        decltype(nodeHs)::iterator& oldH)
{
    /* Remove this application node from its current hardware node. Note that
     * the shared pointers will not be emptied - they are only emptied on
     * destruction of the Problem object. */
    (*oldH)->contents.erase(selA->get());

    /* Assign the selected hardware node to the location field of the selected
     * application node. */
    (*selA)->location = std::weak_ptr(*selH);

    /* Append the selected application node to the contents field of the
     * selected hardware node. */
    (*selH)->contents.insert(selA->get());
}

/* Computes and returns the locality fitness associated with a given
 * application node. Note that locality fitness, in terms of the problem
 * specification, is associated with an edge. Since all edges in this
 * implementation are "double-counted", this method computes half of the
 * fitness contribution. */
float Problem::compute_app_node_locality_fitness(const NodeA& nodeA)
{
    float returnValue = 0;

    /* Hardware node index associated with this application node */
    auto rootHIndex = nodeA.location.lock()->index;

    /* Edge cache row for this hardware node (to avoid getting it multiple
     * times) */
    auto edgeCacheRow = edgeCacheH.at(rootHIndex);

    /* Iterate over each application node. */
    for (const auto& neighbourPtr : nodeA.neighbours)
    {
        /* Get the hardware node index associated with that neighbour. */
        auto neighbourHIndex = neighbourPtr.lock()->location.lock()->index;

        /* Impose fitness penalty from the edgeCache. */
        returnValue -= edgeCacheRow.at(neighbourHIndex);
    }

    return returnValue;
}

/* Computes and returns the clustering fitness associated with a given hardware
 * node. */
float Problem::compute_hw_node_clustering_fitness(const NodeH& nodeH)
{
    auto size = static_cast<float>(nodeH.contents.size());
    return -size * size;
}

/* Computes and returns the total fitness of the current mapping for this
 * solution. */
float Problem::compute_total_fitness()
{
    return compute_total_clustering_fitness() +
        compute_total_locality_fitness();
}

/* Computes and returns the total clustering fitness of the current mapping for
 * this solution. */
float Problem::compute_total_clustering_fitness()
{
    float returnValue = 0;
    for (const auto& nodeH : nodeHs)
        returnValue += compute_hw_node_clustering_fitness(*nodeH);
    return returnValue;
}

/* Computes and returns the total locality fitness of the current mapping for
 * this solution. */
float Problem::compute_total_locality_fitness()
{
    float returnValue = 0;
    for (const auto& nodeA : nodeAs)
        returnValue += compute_app_node_locality_fitness(*nodeA);
    return returnValue;
}

/* Checks the integrity of locks in the data structure.
 *
 * Specifically, this method returns true if no nodes are locked, and false
 * otherwise.
 *
 * Note that this method is itself not thread safe. */
bool Problem::check_lock_integrity(std::stringstream& errors)
{
    bool output = true;  /* Innocent until proven guilty. */
    const unsigned patienceMax = 100;

    for (const auto& nodeH : nodeHs)
    {
        /* Can the hardware node be locked? We attempt a finite number of times
         * because spurious failures are permitted according to the
         * standard. We want to be sure beyond reasonable doubt. */
        unsigned patience = patienceMax;
        while (!nodeH->lock.try_lock())
        {
            patience--;
            if (patience == 0)
            {
                output = false;
                errors << "The mutex belonging to hardware node '"
                       << nodeH->name
                       << "' cannot be locked."
                       << std::endl;
                break;
            }
        }
        if (patience > 0) nodeH->lock.unlock();
    }

    /* And for application nodes. */
    for (const auto& nodeA : nodeAs)
    {
        unsigned patience = patienceMax;
        while (!nodeA->lock.try_lock())
        {
            patience--;
            if (patience == 0)
            {
                output = false;
                errors << "The mutex belonging to application node '"
                       << nodeA->name
                       << "' cannot be locked."
                       << std::endl;
                break;
            }
        }
        if (patience > 0) nodeA->lock.unlock();
    }
    return output;
}

/* Checks the integrity of the data structures holding nodes.
 *
 * Specifically, this method returns true if integrity is not compromised, and
 * false otherwise. It checks:
 *
 *  - that each application node is contained by a hardware node, and that the
 *    relationship is reciprocated (1).
 *
 *  - that each application node contained by each hardware node reciprocates
 *    that relationship (2).
 *
 * Note that this method is not thread safe. */
bool Problem::check_node_integrity(std::stringstream& errors)
{
    bool output = true;  /* Innocent until proven guilty. */

    /* Check (1). */
    for (const auto& nodeA : nodeAs)
    {
        /* Ensure that the application node is contained by something. */
        auto nodeH = nodeA->location.lock();
        if (!nodeH)
        {
            output = false;
            errors << "Application node '" << nodeA->name
                   << "' has no location information." << std::endl;
            continue;
        }

        /* Ensure the relationship is reciprocated. */
        bool found = false;
        for (const auto& containedA : nodeH->contents)
        {
            if (containedA == nodeA.get())
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            output = false;
            errors << "Application node '" << nodeA->name
                   << "' claims to be held in hardware node '" << nodeH->name
                   << "', but that hardware node does not reciprocate."
                   << std::endl;
        }
    }

    /* Check (2). */
    for (const auto& nodeH : nodeHs)
    {
        /* Iterate through each application node, and check that the
         * application node thinks it is contained by the hardware node. */
        for (const auto& containedA : nodeH->contents)
        {
            /* Note that we don't need to check application location here,
             * because it is checked by the logic in (1). */
            if (containedA->location.lock() != nodeH)
            {
                output = false;
                errors << "Hardware node '" << nodeH->name
                       << "' claims to contain application node '"
                       << containedA->name
                       << "', but that application node does not reciprocate."
                       << std::endl;
            }
        }
    }

    return output;
}
