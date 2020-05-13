#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "problem_definition.hpp"
#include "nodes.hpp"

/* Complain loudly if we don't know how large the application or hardware sizes
 * are. This indicates a malformed problem definition file. */
#if !defined(APPLICATION_SIZE) || !defined(HARDWARE_SIZE)
#error "Application graph or hardware graph size is not defined. Have you \
provided a legal problem definition file? This is needed at compile time."
#endif

#include <array>

class Problem
{
    std::array<NodeA, APPLICATION_SIZE> nodeAs;
    std::array<NodeH, HARDWARE_SIZE> nodeHs;
    std::array<std::array<NodeH, HARDWARE_SIZE>, HARDWARE_SIZE> edgeCacheH;
    unsigned pMax = std::numeric_limits<unsigned>::max();
};

#endif
