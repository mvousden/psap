#ifndef DISORDER_SCHEDULES_HPP
#define DISORDER_SCHEDULES_HPP

#include <random>

/* Overflow is a real possibility, believe me. Better to incur a small
 * memory than to check for overflow every iteration. */
typedef unsigned long long Iteration;

class Disorder
{
public:
    Disorder(Iteration maxIteration);
    virtual bool determine(float, float, Iteration) = 0;

protected:
    Iteration maxIteration;

    /* Randomness source and distribution for disorder. */
    std::mt19937 rng;
    std::uniform_real_distribution<> distribution{0, 1};
};

/* Disorder decays exponentially. */
class ExpDecayDisorder: public Disorder
{
public:
    ExpDecayDisorder(Iteration maxIteration);
    bool determine(float, float, Iteration);

private:
    float disorderDecay;
};

/* Disorder decays linearly. */
class LinearDecayDisorder: public Disorder
{
public:
    LinearDecayDisorder(Iteration maxIteration);
    bool determine(float, float, Iteration);
private:
    float gradient;
    float intercept;
};

/* There is no disorder (only superior solutions are ever accepted) */
class NoDisorder: public Disorder
{
public:
    NoDisorder(Iteration maxIteration);
    bool determine(float, float, Iteration);
};

#endif
