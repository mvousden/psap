#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "nodes.hpp"

#include <limits>
#include <vector>

class Problem
{
public:
    void initialise_edge_cache(unsigned diameter);
    std::vector<std::shared_ptr<NodeA>> nodeAs;
    std::vector<std::shared_ptr<NodeH>> nodeHs;
    std::vector<std::vector<float>> edgeCacheH;
    unsigned pMax = std::numeric_limits<unsigned>::max();
};

#endif
