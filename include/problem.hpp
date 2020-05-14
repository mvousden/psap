#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "nodes.hpp"

#include <algorithm>
#include <limits>
#include <list>
#include <random>
#include <vector>

class Problem
{
public:
    std::vector<std::shared_ptr<NodeA>> nodeAs;
    std::vector<std::shared_ptr<NodeH>> nodeHs;
    std::vector<std::vector<float>> edgeCacheH;
    unsigned pMax = std::numeric_limits<unsigned>::max();

    /* Methods that play with edgeCachceH. */
    void initialise_edge_cache(unsigned diameter);
    void populate_edge_cache();

    /* Initial conditions for the annealer. */
    void initial_condition_random();

};

#endif
