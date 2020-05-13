#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "nodes.hpp"

#include <array>

class Problem
{
    std::array<NodeA, 10> nodeAs;
    std::array<NodeH, 10> nodeHs;
    std::array<std::array<NodeH, 10>, 10> edgeCacheH;
    unsigned pMax = std::numeric_limits<unsigned>::max();
};

#endif
