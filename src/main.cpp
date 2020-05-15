#include "problem_definition_wrapper.hpp"

#include <iostream>

int main(int argc, char** argv)
{
    Problem problem;
    problem_definition::define(problem);
    problem.initialise_edge_cache(problem.nodeHs.size());
    problem.populate_edge_cache();
    problem.initial_condition_bucket();
    std::cout << "The fitness for the initial solution is "
              << problem.compute_total_fitness()
              << "."
              << std::endl;
    return 0;
}
