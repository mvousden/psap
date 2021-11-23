#ifndef SEED_HPP
#define SEED_HPP

#include <limits>
#include <random>

/* Some simple common constants and types related to psuedo random number
 * generation. */
typedef std::random_device::result_type Seed;
typedef std::mt19937 Prng;

/* This value is often used as a default, to show that PNRGs are to be seeded
 * with a random device. */
const Seed kSeedSkip = std::numeric_limits<Seed>::max();

Seed determine_seed(Seed);
#endif
