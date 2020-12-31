#include "disorder_schedules.hpp"

#include <cmath>

Disorder::Disorder(Iteration maxIteration):maxIteration(maxIteration)
{
    /* Define our random number generator. */
    rng = std::mt19937(std::random_device{}());
}

/* Constructors for non-virtual classes define fields that are effectively
 * constant for the lifetime of the object. */
NoDisorder::NoDisorder(Iteration maxIteration):
    Disorder::Disorder(maxIteration){}

ExpDecayDisorder::ExpDecayDisorder(Iteration maxIteration):
    Disorder::Disorder(maxIteration)
{
    disorderDecay = std::log(0.5) / (maxIteration / 2.5);
}

LinearDecayDisorder::LinearDecayDisorder(Iteration maxIteration):
    Disorder::Disorder(maxIteration)
{
    gradient = -0.5 / maxIteration;
    intercept = 0.5;
}

/* 'Determine' methods all determine whether to select a new solution, given
 * values for the old fitness and the new fitness, and the current
 * iteration. Always accept a superior solution.
 *
 * Recall that fitnesses are negative. */
bool ExpDecayDisorder::determine(float oldFitness, float newFitness,
                                 Iteration iteration)
{
    if (oldFitness < newFitness) return true;
    auto fitnessDifference = oldFitness - newFitness;
    auto temperatureReciprocal = disorderDecay * iteration;
    auto acceptProb = std::exp(fitnessDifference * temperatureReciprocal);
    return distribution(rng) < acceptProb;
}

bool LinearDecayDisorder::determine(float oldFitness, float newFitness,
                                    Iteration iteration)
{
    if (oldFitness < newFitness) return true;
    auto fitnessDifference = oldFitness - newFitness;
    auto decay = intercept + gradient * iteration;
    auto acceptProb = std::exp(-fitnessDifference) * decay;
    return distribution(rng) < acceptProb;
}

bool NoDisorder::determine(float oldFitness, float newFitness, Iteration)
{
    return oldFitness < newFitness;
}
