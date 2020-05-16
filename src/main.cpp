#include "problem_definition_wrapper.hpp"
#include "serial_annealer.hpp"

#include <iostream>

int main(int argc, char** argv)
{
    /* Setup problem */
    Problem problem;
    problem_definition::define(problem);
    problem.initialise_edge_cache(problem.nodeHs.size());
    problem.populate_edge_cache();
    problem.initial_condition_random();

    std::cout << "The fitness for the initial solution is "
              << problem.compute_total_fitness()
              << "."
              << std::endl;

    /* Solve problem */
    Iteration maxIteration = 100;
    auto annealer = SerialAnnealer(maxIteration);
    annealer(problem);

    std::cout << "The fitness for the solution after "
              << maxIteration
              << " rounds of annealing is "
              << problem.compute_total_fitness()
              << "."
              << std::endl;

    return 0;
}
