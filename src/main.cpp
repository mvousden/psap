#include "problem_definition_wrapper.hpp"

int main(int argc, char** argv)
{
    Problem problem;
    problem_definition::define(problem);
    problem.populate_edge_cache();
    problem.initial_condition_random();
    return 0;
}
