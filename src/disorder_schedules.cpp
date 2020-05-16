#include "disorder_schedules.hpp"

#include <cmath>

Disorder::Disorder(Iteration maxIteration):maxIteration(maxIteration)
{
    /* Define our random number generator. */
    rng = std::mt19937(std::random_device{}());
}

/* Constructors for non-virtual classes define fields that are effectively
 * constant for the lifetime of the object. */
ExpDecayDisorder::ExpDecayDisorder(Iteration maxIteration):
    Disorder::Disorder(maxIteration)
{
    disorderDecay = std::log(0.5) / (maxIteration / 3.0);
}

LinearDecayDisorder::LinearDecayDisorder(Iteration maxIteration):
    Disorder::Disorder(maxIteration)
{
    gradient = -0.5 / maxIteration;
    intercept = 0.5;
}

/* 'Determine' methods all determine whether to select a new solution, given
 * values for the old fitness and the new fitness, and the current
 * iteration. */

/* Disorder decays exponentially */
bool ExpDecayDisorder::determine(float oldFitness, float newFitness,
                                     Iteration iteration)
{
    auto fitnessRatio = oldFitness / newFitness;
    auto acceptProb = fitnessRatio * 0.5 * std::exp(disorderDecay * iteration);
    return distribution(rng) > acceptProb;
}

/* Disorder decays linearly */
bool LinearDecayDisorder::determine(float oldFitness, float newFitness,
                                        Iteration iteration)
{
    auto fitnessRatio = oldFitness / newFitness;
    auto acceptProb = fitnessRatio * (intercept + gradient * iteration);
    return distribution(rng) > acceptProb;
}
