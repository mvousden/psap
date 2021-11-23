#include "problem_definition_wrapper.hpp"
#include "parallel_annealer.hpp"
#include "serial_annealer.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    /* Life's too short. */
#include "main_config.hpp"

    /* Problem? */
    Problem problem;
    if (useSeed) problem.set_seed(seed);
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

    /* Write annealer properties. */
    if (serial) problem.log("Using serial annealer.");
    else
    {
        std::stringstream message;
        message << "Using ";
        if (fullySynchronous) message << "fully-synchronous ";
        else message << "semi-asynchronous ";
        message << "parallel annealer with " << numWorkers << " workers.";
        problem.log(message.str());
    }

    /* Prepare problem for annealing */
    problem.initialise_edge_cache(
        static_cast<unsigned>(problem.nodeHs.size()));
    problem.populate_edge_cache();
    problem.initial_condition_random();

    if (!mouseMode)
    {
        /* Check/write integrity */
        if (!serial)
        {
            problem.write_lock_integrity_check_errs(
                (outDir / "integrity_locks_before.err").string());
        }
        problem.write_node_integrity_check_errs(
            (outDir / "integrity_nodes_before.err").string());

        /* Compute starting fitness for logging */
        {
            std::stringstream message;
            message << "Initial fitness: "
                    << problem.compute_total_fitness() << ".";
            problem.log(message.str());
        }

        /* Write initial condition data */
        problem.write_a_degrees((outDir / "a_degrees.csv").string());
        problem.write_a_h_graph(
            (outDir / "initial_a_h_graph.csv").string());
        problem.write_a_to_h_map(
            (outDir / "initial_a_to_h_map.csv").string());

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
            if (useSeed)
            {
                SerialAnnealer<ExpDecayDisorder>
                    (maxIteration, outDir, seed)(problem);
            }
            else
            {
                SerialAnnealer<ExpDecayDisorder>
                    (maxIteration, outDir)(problem);
            }
        }
        else
        {
            /* Take intermediate fitness measurements. */
            if (useSeed)
            {
                ParallelAnnealer<ExpDecayDisorder>(numWorkers, maxIteration,
                                                   outDir, seed)
                    (problem, maxIteration / 20, fullySynchronous);
            }
            else
            {
                ParallelAnnealer<ExpDecayDisorder>(numWorkers, maxIteration,
                                                   outDir)
                    (problem, maxIteration / 20, fullySynchronous);
            }
        }
    }

    else
    {
        /* Run as quietly as possible, printing timing information only. */
        if (serial)
        {
            if (useSeed)
            {
                auto annealer = SerialAnnealer<ExpDecayDisorder>(
                    maxIteration, "", seed);
                auto timeAtStart = std::chrono::steady_clock::now();
                annealer(problem);
                std::cout << std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - timeAtStart).count()
                          << std::endl;
            }
            else
            {
                auto annealer = SerialAnnealer<ExpDecayDisorder>(maxIteration);
                auto timeAtStart = std::chrono::steady_clock::now();
                annealer(problem);
                std::cout << std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - timeAtStart).count()
                          << std::endl;
            }
        }
        else
        {
            if (useSeed)
            {
                auto annealer = ParallelAnnealer<ExpDecayDisorder>(
                    numWorkers, maxIteration, "", seed);
                auto timeAtStart = std::chrono::steady_clock::now();
                annealer(problem, fullySynchronous);
                std::cout << std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - timeAtStart).count()
                          << std::endl;
            }
            else
            {
                auto annealer = ParallelAnnealer<ExpDecayDisorder>(
                    numWorkers, maxIteration);
                auto timeAtStart = std::chrono::steady_clock::now();
                annealer(problem, fullySynchronous);
                std::cout << std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - timeAtStart).count()
                          << std::endl;
            }
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

        problem.write_a_h_graph((outDir / "final_a_h_graph.csv").string());
        problem.write_a_to_h_map((outDir / "final_a_to_h_map.csv").string());
        problem.write_h_graph((outDir / "h_graph.csv").string());
        problem.write_h_nodes((outDir / "h_nodes.csv").string());
        problem.write_h_node_loading((outDir / "h_node_loading.csv").string());
        if (!serial)
        {
            problem.write_lock_integrity_check_errs(
                (outDir / "integrity_locks_after.err").string());
        }
        problem.write_node_integrity_check_errs(
            (outDir / "integrity_nodes_after.err").string());
    }

    return 0;
}
