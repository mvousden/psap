#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "nodes.hpp"

#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>

class Problem
{
public:
    std::vector<std::shared_ptr<NodeA>> nodeAs;
    std::vector<std::shared_ptr<NodeH>> nodeHs;
    std::vector<std::tuple<unsigned, unsigned, float>> edgeHs;
    unsigned pMax = std::numeric_limits<unsigned>::max();
    std::string name = "unnamed_problem";

    Problem();
    ~Problem();

    /* Logging and pathing */
    void define_output_path(const std::filesystem::path& outDirArg);
    void initialise_logging();
    void log(const std::string_view& message);

    /* Methods that interact with edgeCacheH. */
    void initialise_edge_cache(unsigned diameter);
    void populate_edge_cache();

    /* Initial conditions for the annealer. */
    void initial_condition_bucket();
    void initial_condition_random();

    /* Neighbouring state selection. */
    unsigned select_serial(decltype(nodeAs)::iterator& selA,
                       decltype(nodeHs)::iterator& selH,
                       decltype(nodeHs)::iterator& oldH);
    unsigned select_parallel_sasynchronous(decltype(nodeAs)::iterator& selA,
                                           decltype(nodeHs)::iterator& selH,
                                           decltype(nodeHs)::iterator& oldH);
    unsigned select_parallel_synchronous(decltype(nodeAs)::iterator& selA,
                                         decltype(nodeHs)::iterator& selH,
                                         decltype(nodeHs)::iterator& oldH);

    /* Transformation from selection data. */
    void transform(decltype(nodeAs)::iterator& selA,
                   decltype(nodeHs)::iterator& selH,
                   decltype(nodeHs)::iterator& oldH);

    /* Fitness calculators */
    float compute_app_node_locality_fitness(const NodeA& nodeA);
    float compute_hw_node_clustering_fitness(const NodeH& nodeH);
    float compute_total_fitness();
    float compute_total_clustering_fitness();
    float compute_total_locality_fitness();

    /* Integrity checking */
    bool check_lock_integrity(std::stringstream& errors);
    bool check_node_integrity(std::stringstream& errors);

    /* State dumps */
    void write_a_degrees(const std::string_view& path);
    void write_a_h_graph(const std::string_view& path);
    void write_a_to_h_map(const std::string_view& path);
    void write_h_graph(const std::string_view& path);
    void write_h_nodes(const std::string_view& path);
    void write_h_node_loading(const std::string_view& path);
    void write_lock_integrity_check_errs(const std::string_view& path);
    void write_node_integrity_check_errs(const std::string_view& path);

    /* Number of iterations a while loop goes through during selection, after
     * which a message is logged. */
    constexpr static float selectionPatience = 1e3;

private:
    std::vector<std::vector<float>> edgeCacheH;
    std::mt19937 rng;

    /* Logging and pathing */
    std::filesystem::path outDir;
    std::ofstream logS;
    std::mutex logLock;

    constexpr static auto logHandle = "log.txt";

    /* Granular selection */
    void select_serial_sela(decltype(nodeAs)::iterator& selA);
    void select_serial_oldh(decltype(nodeAs)::iterator& selA,
                            decltype(nodeHs)::iterator& oldH);
    void select_serial_selh(decltype(nodeHs)::iterator& selH,
                            decltype(nodeHs)::iterator& avoid);
    unsigned select_parallel_sasynchronous_sela(
        decltype(nodeAs)::iterator& selA);
    void select_parallel_sasynchronous_oldh(decltype(nodeAs)::iterator& selA,
                                            decltype(nodeHs)::iterator& oldH);
    void select_parallel_sasynchronous_selh(decltype(nodeHs)::iterator& selH,
                                            decltype(nodeHs)::iterator& avoid);
};

#endif
