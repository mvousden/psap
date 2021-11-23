#ifndef DISORDER_SCHEDULES_HPP
#define DISORDER_SCHEDULES_HPP

#include "seed.hpp"

#include <string>

/* Overflow is a real possibility, believe me. Better to incur a small memory
 * penalty than to check for overflow every iteration. */
typedef unsigned long long Iteration;

class Disorder
{
public:
    Disorder(Iteration maxIteration, Seed seed=kSeedSkip);
    virtual bool determine(float, float, Iteration) = 0;

protected:
    Iteration maxIteration;

    /* Randomness source and distribution for disorder. */
    Prng rng;
    std::uniform_real_distribution<> distribution{0, 1};
};

/* There is no disorder and no acceptance - the state never changes. */
class AbsoluteZero
{
public:
    AbsoluteZero(Iteration, Seed){};
    bool determine(float, float, Iteration){return false;}
    const char* handle = "AbsoluteZero";
};

/* Disorder decays exponentially. Better solutions are always accepted. */
class ExpDecayDisorder: public Disorder
{
public:
    ExpDecayDisorder(Iteration maxIteration, Seed seed=kSeedSkip);
    bool determine(float, float, Iteration);
    const char* handle = "ExpDecayDisorder";

private:
    double disorderDecay;
};

/* Disorder decays linearly. Better solutions are always accepted. */
class LinearDecayDisorder: public Disorder
{
public:
    LinearDecayDisorder(Iteration maxIteration, Seed seed=kSeedSkip);
    bool determine(float, float, Iteration);
    const char* handle = "LinearDecayDisorder";

private:
    double gradient;
    double intercept;
};

/* There is no disorder. Better solutions are always accepted. */
class NoDisorder: public Disorder
{
public:
    NoDisorder(Iteration maxIteration, Seed seed=kSeedSkip);
    bool determine(float, float, Iteration);
    const char* handle = "NoDisorder";
};

#endif
