#include "problem_definition_wrapper.hpp"
#include "parallel_annealer.hpp"
#include "serial_annealer.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    /* Setup problem */
    Problem problem;
    problem_definition::define(problem);
    problem.initialise_edge_cache(
        static_cast<unsigned>(problem.nodeHs.size()));
    problem.populate_edge_cache();
    problem.initial_condition_random();
    auto initialFitness = problem.compute_total_fitness();

    /* Directory to write to - clear it. */
    std::filesystem::path outRootDir = "output";
    std::filesystem::path outDir = outRootDir / problem.name;
    std::filesystem::remove_all(outDir);
    std::filesystem::create_directories(outDir);

    /* Write initial stuff */
    problem.write_a_h_graph((outDir / "initial_a_h_graph.csv").u8string());
    problem.write_a_to_h_map((outDir / "initial_a_to_h_map.csv").u8string());

    /* Solve problem */
    Iteration maxIteration = static_cast<Iteration>(1e5);
    auto annealer = ParallelAnnealer<ExpDecayDisorder>(
        2, maxIteration, (outDir / "anneal_ops.csv").u8string());
    annealer(problem);

    /* Write solved stuff */
    std::cout << maxIteration
              << " rounds of annealing transformed the state from having "
              << initialFitness
              << " fitness to " << problem.compute_total_fitness()
              << " fitness!" << std::endl;

    problem.write_a_h_graph((outDir / "final_a_h_graph.csv").u8string());
    problem.write_a_to_h_map((outDir / "final_a_to_h_map.csv").u8string());
    problem.write_h_graph((outDir / "h_graph.csv").u8string());
    problem.write_h_nodes((outDir / "h_nodes.csv").u8string());
    problem.write_integrity_check_errs((outDir / "integrity.err").u8string());

    /* This one isn't needed, but makes creating histograms easier.
    problem.write_h_node_loading((outDir / "h_node_loading.csv").u8string());
    */

    return 0;
}
