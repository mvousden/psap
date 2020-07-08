#include "problem.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>

Problem::Problem()
{
    /* Define our random number generator. */
    rng = std::mt19937(std::random_device{}());

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

/* Define a path for dumping outputs. Pass in a directory path (will be
 * cleared). */
void Problem::define_output_path(const std::filesystem::path& outDirArg)
{
    std::filesystem::remove_all(outDirArg);
    std::filesystem::create_directories(outDirArg);
    outDir = outDirArg;
}

/* Initialise logging capability. Returns without setup if the output path has
 * not been defined. */
void Problem::initialise_logging()
{
    if (!outDir.empty())
    {
        logS = std::ofstream((outDir / logHandle).u8string(),
                             std::ofstream::app);
        log("Logging initialised.");
    }
}

/* Write a log message, and flushes. Is thread safe. */
void Problem::log(const std::string_view& message)
{
    /* Does nothing if logging is not initialised. */
    if (!logS.is_open()) return;

    /* Wait for current thread to finish logging (RAII). */
    std::lock_guard<decltype(Problem::logLock)> guard(logLock);

    /* Write datetime */
    std::stringstream combined;

    auto datetime = std::time(nullptr);
    combined << "["
             << std::put_time(std::gmtime(&datetime), "%FT%T%z") << "] ";

    /* Write message data */
    combined << message.data();

    /* Write to stdout, with newline character, and to logfile with flush. */
    std::cout << combined.str() << "\n";
    logS << combined.str() << std::endl;
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
        (*selHIt)->contents.push_back(std::weak_ptr(selA));
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
        selH->lock()->contents.push_back(selA);

        /* Remove the hardware node if it is full. */
        if (selH->lock()->contents.size() >= pMax) nonEmpty.erase(selH);
    }

    log("Initial condition applied.");
}

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
    (*oldH)->contents.remove_if([selA](const std::weak_ptr<NodeA>& cmp)
                                {return (*selA) == cmp.lock();});

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
 * Note that this method is not thread safe */
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
            if (containedA.lock() == nodeA)
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
            if (containedA.lock()->location.lock() != nodeH)
            {
                output = false;
                errors << "Hardware node '" << nodeH->name
                       << "' claims to contain application node '"
                       << containedA.lock()->name
                       << "', but that application node does not reciprocate."
                       << std::endl;
            }
        }
    }

    return output;
}

/* Writes, for each application node, an entry to the CSV file at path passed
 * to by argument with its degree. Each line is of the form
 * '<A_NODE_NAME>,<DEGREE>'. Any existing file is clobbered. Does no filesystem
 * checking of any kind. */
void Problem::write_a_degrees(const std::string_view& path)
{
    std::stringstream message;
    message << "Writing a degree list to file at '" << path.data() << "'.";
    log(message.str());

    std::ofstream out(path.data(), std::ofstream::trunc);
    out << "Application node name,Degree" << std::endl;

    for (const auto& nodeA : nodeAs)
        out << nodeA->name << "," << nodeA->neighbours.size() << std::endl;
    out.close();
}

/* Writes, for each application edge, an entry to the CSV file at path passed
 * to by argument connecting nodes in the hardware graph. Each line is of the
 * form '<FROM_H_NODE_NAME>,<TO_H_NODE_NAME>,<COUNT>', where <COUNT> indicates
 * the number of application edges traversing this edge. Any existing file is
 * clobbered. Does no filesystem checking of any kind. */
void Problem::write_a_h_graph(const std::string_view& path)
{
    std::stringstream message;
    message << "Writing a_h graph to file at '" << path.data() << "'.";
    log(message.str());

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
        for (const auto& neighbourPtr : nodeA->neighbours)
        {
            std::string fromHName = nodeA->location.lock()->name;
            std::string toHName = neighbourPtr.lock()->location.lock()->name;

            /* Don't write anything if the two application nodes are on the
             * same hardware node. */
            if (fromHName == toHName) continue;

            /* If the entry in the submap doesn't exist, create it and
             * initialise the value to one. Otherwise, increment it. */
            if (edges[fromHName].find(toHName) == edges[fromHName].end())
                edges.at(fromHName)[toHName] = 1;
            else edges.at(fromHName).at(toHName)++;
        }
    }

    /* Write out each entry in the map. */
    std::ofstream out(path.data(), std::ofstream::trunc);
    out << "Hardware node name (first),Hardware node name (second),Loading"
        << std::endl;
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
void Problem::write_a_to_h_map(const std::string_view& path)
{
    std::stringstream message;
    message << "Writing a_to_h map to file at '" << path.data() << "'.";
    log(message.str());

    std::ofstream out(path.data(), std::ofstream::trunc);
    out << "Application node name,Hardware node name" << std::endl;
    for (const auto& nodeA : nodeAs)
        out << nodeA->name << "," << nodeA->location.lock()->name << std::endl;
    out.close();
}

/* Writes the hardware graph to a CSV file at path passed to by argument. Each
 * line is of the form '<H_NODE_NAME>,<H_NODE_NAME>'. No weights are
 * written. Any existing file is clobbered. Does no filesystem checking of any
 * kind. */
void Problem::write_h_graph(const std::string_view& path)
{
    std::stringstream message;
    message << "Writing h graph to file at '" << path.data() << "'.";
    log(message.str());

    std::ofstream out(path.data(), std::ofstream::trunc);
    out << "Hardware node name (first),Hardware node name (second)"
        << std::endl;
    for (const auto& edge : edgeHs)
        out << nodeHs.at(std::get<0>(edge))->name << ","
            << nodeHs.at(std::get<1>(edge))->name << std::endl;
    out.close();
}

/* Writes the ordering of nodes to a CSV file at path passed to by
 * argument. Each line is of the form '<H_NODE_NAME>,<HORIZ_POS>,<VERTI_POS>'
 * (the index is implied, and begins at zero). Any existing file is
 * clobbered. Does no filesystem checking of any kind. */
void Problem::write_h_nodes(const std::string_view& path)
{
    std::stringstream message;
    message << "Writing h node information to file at '"
            << path.data() << "'.";
    log(message.str());

    std::ofstream out(path.data(), std::ofstream::trunc);
    out << "Hardware node name,Horizontal position,Vertical position"
        << std::endl;
    for (const auto& node : nodeHs) out << node->name << ","
                                        << node->posHoriz << ","
                                        << node->posVerti << std::endl;
    out.close();
}


/* Writes the loading of each hardware node to the CSV file at path passed to
 * by argument. Each line is of the form
 * '<H_NODE_NAME>,<NUMBER_OF_A_NODES>'. Any existing file is clobbered. Does no
 * filesystem checking of any kind. */
void Problem::write_h_node_loading(const std::string_view& path)
{
    std::stringstream message;
    message << "Writing h node loading to file at '" << path.data() << "'.";
    log(message.str());

    std::ofstream out(path.data(), std::ofstream::trunc);
    out << "Hardware node name,Number of contained application nodes"
        << std::endl;
    for (const auto& nodeH : nodeHs)
        out << nodeH->name << "," << nodeH->contents.size() << std::endl;
    out.close();
}

/* Checks data structure integrity, and writes errors to a file. Any existing
 * file is clobbered. Does not filesystem checking of any kind. If no errors
 * are found, the file is created with no content. */
void Problem::write_integrity_check_errs(const std::string_view& path)
{
    std::stringstream message;
    message << "Performing integrity check, writing to file at '"
            << path.data() << "'.";
    log(message.str());

    std::ofstream out(path.data(), std::ofstream::trunc);
    std::stringstream errorMsgs;
    if (!check_node_integrity(errorMsgs))
    {
        log("Integrity errors found.");
        out << errorMsgs.rdbuf();
    }
    else log("No integrity errors found.");
    out.close();
}
