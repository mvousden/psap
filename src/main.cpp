#include "problem_definition_wrapper.hpp"
#include "serial_annealer.hpp"

#include <filesystem>
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

    /* Directory to write to */
    std::filesystem::path outRootDir = "output";
    std::filesystem::path outDir = outRootDir / problem.name;
    std::filesystem::create_directories(outDir);

    /* Solve problem */
    Iteration maxIteration = 100;
    auto annealer = SerialAnnealer(maxIteration, outDir / "anneal_ops.csv");
    annealer(problem);

    std::cout << "The fitness for the solution after "
              << maxIteration
              << " rounds of annealing is "
              << problem.compute_total_fitness()
              << "."
              << std::endl;

    /* Write stuff */
    problem.write_a_h_graph(outDir / "a_h_graph.csv");
    problem.write_a_to_h_map(outDir / "a_to_h_map.csv");
    problem.write_h_graph(outDir / "h_graph.csv");
    problem.write_h_node_loading(outDir / "h_node_loading.csv");


    return 0;
}
