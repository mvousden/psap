#include "serial_annealer.hpp"

/* Initialise disorder. */
template<class DisorderT>
SerialAnnealer<DisorderT>::SerialAnnealer(Iteration maxIterationArg):
    maxIteration(maxIterationArg),
    disorder(maxIterationArg){}

/* Hits the solution repeatedly with a hammer and cools it. Hopefully improves
 * it (history has shown that it probably will work). */
template<class DisorderT>
void SerialAnnealer<DisorderT>::anneal(Problem& problem)
{
    auto selA = problem.nodeAs.begin();
    auto selH = problem.nodeHs.begin();
    auto oldH = *selH;  /* Note - not an iterator */

    /* Base fitness "used" from the start of each iteration. */
    auto oldFitness = problem.compute_total_fitness();

    while (true)
    {
        iteration++;

        /* Selection */
        problem.select(selA, selH);

        /* Fitness of components before transformation. */
        oldH = problem.nodeHs[(*selA)->location.lock()->index];
        auto oldFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(*oldH);

        /* Transformation */
        problem.transform(selA, selH);

        /* Fitness of components before transformation. */
        auto newFitnessComponents =
            problem.compute_app_node_locality_fitness(**selA) +
            problem.compute_hw_node_clustering_fitness(**selH) +
            problem.compute_hw_node_clustering_fitness(*oldH);

        /* Determination */
        auto newFitness = oldFitness - oldFitnessComponents +
            newFitnessComponents;

        /* Always accept a better solution. */
        bool sufficientlyDetermined = true;
        if (oldFitness >= newFitness)
        {
            sufficientlyDetermined = disorder.determine(oldFitness, newFitness,
                                                        iteration);
            if (not sufficientlyDetermined)  /* Almost looks like Python. */
            {
                /* Revert the solution */
                auto oldHIt = problem.nodeHs.begin() + oldH->index;
                problem.transform(selA, oldHIt);
            }
        }

        /* If the solution was sufficiently determined to be chosen, update the
         * base fitness to support computation for the next iteration. */
        if (sufficientlyDetermined) oldFitness = newFitness;

        /* Termination */
        if (iteration == maxIteration) break;  /* How boring. */
    }
}

#include "serial_annealer-impl.hpp"
