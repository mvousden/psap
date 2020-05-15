#ifndef SERIAL_ANNEALER_HPP
#define SERIAL_ANNEALER_HPP

#include "problem.hpp"

class SerialAnnealer
{
public:
    SerialAnnealer();
    void operator()(Problem& problem){anneal(problem);}

    unsigned long long maxIteration = 100;

private:
    void anneal(Problem& problem);
    bool determination(float, float);

    /* Randomness source and distribution for disorder. */
    std::mt19937 rng;
    std::uniform_real_distribution<> distribution{0, 1};

    /* Overflow is a real possibility, believe me. Better to incur a small
     * memory than to check for overflow every iteration. */
    unsigned long long iteration = 0;
};

#endif
