#include "problem_definition_wrapper.hpp"
#include "serial_annealer.hpp"

#include <iostream>

int main(int argc, char** argv)
{
    Problem problem;
    SerialAnnealer annealer;

    /* Setup problem */
    problem_definition::define(problem);
    problem.initialise_edge_cache(problem.nodeHs.size());
    problem.populate_edge_cache();
    problem.initial_condition_random();

    std::cout << "The fitness for the initial solution is "
              << problem.compute_total_fitness()
              << "."
              << std::endl;

    /* Solve problem */
    annealer(problem);

    std::cout << "The fitness for the annealed solution is "
              << problem.compute_total_fitness()
              << "."
              << std::endl;

    return 0;
}
