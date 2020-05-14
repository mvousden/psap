#include "problem.hpp"

/* Reserve space in the edge cache as a function of the diameter, and define
 * default values as per the specification - zeroes on the diagonals, and a
 * huge number everywhere else. */
void Problem::initialise_edge_cache(unsigned diameter)
{
    std::vector<std::vector<float>>::size_type eOuterIndex;
    std::vector<float>::size_type eInnerIndex;
    edgeCacheH.reserve(diameter);
    for (eOuterIndex = 0; eOuterIndex < diameter; eOuterIndex++)
    {
        auto inner = edgeCacheH[eOuterIndex];
        inner.reserve(diameter);
        for (eInnerIndex = 0; eInnerIndex < diameter; eInnerIndex++)
        {
            if (eOuterIndex == eInnerIndex) inner[eInnerIndex] = 0;
            else inner[eInnerIndex] = std::numeric_limits<float>::max();
        }
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
