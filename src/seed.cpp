#include "seed.hpp"

/* Function to filter a proposed seed (which may be kSeedSkip) into a seed for
 * use in a PNRG. */
Seed determine_seed(Seed seed)
{
    if (seed == kSeedSkip) return std::random_device{}();
    else return seed;
}
