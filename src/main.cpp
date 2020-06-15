#include "problem_definition_wrapper.hpp"
#include "parallel_annealer.hpp"
#include "serial_annealer.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    /* Construct problem */
    Problem problem;
    problem_definition::define(problem);

    /* Directory to write to - clear it. */
    std::filesystem::path outRootDir = "output";
    std::filesystem::path outDir = outRootDir / problem.name;
    std::filesystem::remove_all(outDir);
    std::filesystem::create_directories(outDir);

    /* Prepare problem for annealing */
    problem.initialise_logging((outDir / "log.txt").u8string());
    problem.initialise_edge_cache(
        static_cast<unsigned>(problem.nodeHs.size()));
    problem.populate_edge_cache();
    problem.initial_condition_random();

    /* Check/write integrity */
    problem.write_integrity_check_errs(
        (outDir / "integrity_before.err").u8string());

    /* Compute starting fitness for logging */
    auto initialFitness = problem.compute_total_fitness();

    {
        std::stringstream message;
        message << "Initial fitness: " << initialFitness << ".";
        problem.log(message.str());
    }

    /* Write initial condition data */
    problem.write_a_h_graph((outDir / "initial_a_h_graph.csv").u8string());
    problem.write_a_to_h_map((outDir / "initial_a_to_h_map.csv").u8string());

    /* Solve problem */
    Iteration maxIteration = static_cast<Iteration>(1e6);
    {
        std::stringstream message;
        message << "Annealing problem for " << maxIteration << " iterations.";
        problem.log(message.str());
    }
    auto annealer = ParallelAnnealer<ExpDecayDisorder>(
        2, maxIteration, (outDir / "anneal_ops.csv").u8string());
    annealer(problem);
    problem.log("Annealing complete.");

    /* Write solved stuff */
    {
        std::stringstream message;
        message << "Final fitness: " << problem.compute_total_fitness() << ".";
        problem.log(message.str());
    }

    problem.write_a_h_graph((outDir / "final_a_h_graph.csv").u8string());
    problem.write_a_to_h_map((outDir / "final_a_to_h_map.csv").u8string());
    problem.write_h_graph((outDir / "h_graph.csv").u8string());
    problem.write_h_nodes((outDir / "h_nodes.csv").u8string());
    problem.write_integrity_check_errs(
        (outDir / "integrity_after.err").u8string());

    /* This one isn't needed, but makes creating histograms easier.
    problem.write_h_node_loading((outDir / "h_node_loading.csv").u8string());
    */

    return 0;
}
