#include "problem_definition_wrapper.hpp"
#include "parallel_annealer.hpp"
#include "serial_annealer.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    /* Mouse mode - useful for runtime measurements. */
    bool mouseMode = {{MOUSE_MODE}};

    /* Whether or not to anneal in serial, or parallel. */
    bool serial = {{SERIAL_MODE}};

    /* Synchronisity (serial=false only) - do we synchronise to ensure no
     * computation with stale data (true), or do we only synchronise only to
     * ensure data structure integrity (false)? */
    bool fullySynchronous = {{FULLY_SYNCHRONOUS}};

    /* Construct problem */
    Problem problem;
    problem_definition::define(problem);

    /* Directory to write to - clear it. */
    std::filesystem::path outDir = "";
    if (!mouseMode)
    {
        decltype(outDir) outRootDir = "output";
        outDir = outRootDir / problem.name;
        problem.define_output_path(outDir);
        problem.initialise_logging();
    }

    /* Prepare problem for annealing */
    problem.initialise_edge_cache(
        static_cast<unsigned>(problem.nodeHs.size()));
    problem.populate_edge_cache();
    problem.initial_condition_random();

    Iteration maxIteration = static_cast<Iteration>({{ITERATIONS}});
    if (!mouseMode)
    {
        /* Check/write integrity */
        problem.write_integrity_check_errs(
            (outDir / "integrity_before.err").u8string());

        /* Compute starting fitness for logging */
        {
            std::stringstream message;
            message << "Initial fitness: "
                    << problem.compute_total_fitness() << ".";
            problem.log(message.str());
        }

        /* Write initial condition data */
        problem.write_a_degrees((outDir / "a_degrees.csv").u8string());
        problem.write_a_h_graph(
            (outDir / "initial_a_h_graph.csv").u8string());
        problem.write_a_to_h_map(
            (outDir / "initial_a_to_h_map.csv").u8string());

        /* Begin to solve the problem. */
        {
            std::stringstream message;
            message << "Annealing problem for " << maxIteration
                    << " iterations.";
            problem.log(message.str());
        }
    }

    /* Create the annealer and do the dirty. */
    if (!mouseMode)
    {
        /* Run noisily with much logging and outputting of files. */
        if (serial)
        {
            SerialAnnealer<ExpDecayDisorder>(maxIteration, outDir)(problem);
        }
        else
        {
            /* Take intermediate fitness measurements. */
            ParallelAnnealer<ExpDecayDisorder>
                ({{NUM_THREADS}}, maxIteration, outDir)
                (problem, maxIteration / 20, fullySynchronous);
        }
    }

    else
    {
        /* Run as quietly as possible, printing timing information only. */
        if (serial)
        {
            auto annealer = SerialAnnealer<ExpDecayDisorder>(maxIteration);
            auto timeAtStart = std::chrono::steady_clock::now();
            annealer(problem, fullySynchronous);
            std::cout << std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - timeAtStart).count()
                      << std::endl;
        }
        else
        {
            auto annealer = ParallelAnnealer<ExpDecayDisorder>({{NUM_THREADS}},
                                                               maxIteration);
            auto timeAtStart = std::chrono::steady_clock::now();
            annealer(problem, fullySynchronous);
            std::cout << std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - timeAtStart).count()
                      << std::endl;
        }

    }

    /* Write solved stuff */
    if (!mouseMode)
    {
        problem.log("Annealing complete.");

        /* Write solved stuff */
        {
            std::stringstream message;
            message << "Final fitness: "
                    << problem.compute_total_fitness() << ".";
            problem.log(message.str());
        }

        problem.write_a_h_graph((outDir / "final_a_h_graph.csv").u8string());
        problem.write_a_to_h_map((outDir / "final_a_to_h_map.csv").u8string());
        problem.write_h_graph((outDir / "h_graph.csv").u8string());
        problem.write_h_nodes((outDir / "h_nodes.csv").u8string());
        problem.write_h_node_loading(
            (outDir / "h_node_loading.csv").u8string());
        problem.write_integrity_check_errs(
            (outDir / "integrity_after.err").u8string());
    }

    return 0;
}