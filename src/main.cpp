#include "problem_definition_wrapper.hpp"

#include <iostream>

int main(int argc, char** argv)
{
    Problem problem;
    problem_definition::define(problem);
    problem.populate_edge_cache();
    problem.initial_condition_random();
    std::cout << "The fitness for the initial solution is "
              <<  problem.compute_total_fitness()
              << "."
              << std::endl;
    return 0;
}
