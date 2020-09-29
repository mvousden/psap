#include "problem.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>

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
