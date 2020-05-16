#include "problem_definition_wrapper.hpp"
#include "serial_annealer.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    /* Setup problem */
    Problem problem;
    problem_definition::define(problem);
    problem.initialise_edge_cache(problem.nodeHs.size());
    problem.populate_edge_cache();
    problem.initial_condition_random();
    auto initialFitness = problem.compute_total_fitness();

    /* Directory to write to */
    std::filesystem::path outRootDir = "output";
    std::filesystem::path outDir = outRootDir / problem.name;
    std::filesystem::create_directories(outDir);

    /* Write initial stuff */
    problem.write_a_h_graph(outDir / "initial_a_h_graph.csv");
    problem.write_a_to_h_map(outDir / "initial_a_to_h_map.csv");

    /* Solve problem */
    Iteration maxIteration = 100;
    auto annealer = SerialAnnealer(maxIteration, outDir / "anneal_ops.csv");
    annealer(problem);

    /* Write solved stuff */
    std::cout << maxIteration
              << " rounds of annealing transformed the state from having "
              << initialFitness
              << " fitness to " << problem.compute_total_fitness()
              << " fitness!" << std::endl;

    problem.write_a_h_graph(outDir / "final_a_h_graph.csv");
    problem.write_a_to_h_map(outDir / "final_a_to_h_map.csv");
    problem.write_h_graph(outDir / "h_graph.csv");

    /* This one isn't needed, but makes creating histograms easier.
    problem.write_h_node_loading(outDir / "h_node_loading.csv"); */

    return 0;
}
