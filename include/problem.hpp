#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "nodes.hpp"

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

    /* Logging */
    void initialise_logging(const std::string_view& path);
    void log(const std::string_view& message);

    /* Methods that interact with edgeCacheH. */
    void initialise_edge_cache(unsigned diameter);
    void populate_edge_cache();

    /* Initial conditions for the annealer. */
    void initial_condition_bucket();
    void initial_condition_random();

    /* Neighbouring state selection. */
    void select(decltype(nodeAs)::iterator& selA,
                decltype(nodeHs)::iterator& selH,
                decltype(nodeHs)::iterator& oldH, bool atomic=false);

    /* Synchronisation object for application and hardware nodes. Application
     * nodes are locked on selection (responsibility of the problem), whereas
     * hardware nodes are locked on transformation (responsibility of the
     * annealer).*/
    void initialise_atomic_locks();
    std::vector<std::mutex> lockHs;
    std::vector<std::mutex> lockAs;  /* Not "lock as", but "lock application
                                      * nodes". Obviously. */

    /* Transformation from selection data. */
    void transform(decltype(nodeAs)::iterator& selA,
                   decltype(nodeHs)::iterator& selH,
                   decltype(nodeHs)::iterator& oldH);

    /* Fitness calculators */
    float compute_app_node_locality_fitness(NodeA& nodeA);
    float compute_hw_node_clustering_fitness(NodeH& nodeH);
    float compute_total_fitness();

    /* Integrity checking */
    bool check_node_integrity(std::stringstream& errors);

    /* State dumps */
    void write_a_h_graph(const std::string_view& path);
    void write_a_to_h_map(const std::string_view& path);
    void write_h_graph(const std::string_view& path);
    void write_h_nodes(const std::string_view& path);
    void write_h_node_loading(const std::string_view& path);
    void write_integrity_check_errs(const std::string_view& path);

private:
    std::vector<std::vector<float>> edgeCacheH;
    std::mt19937 rng;

    /* Logging */
    std::ofstream logS;
    std::mutex logLock;

    /* Granular selection */
    void select_sela(decltype(nodeAs)::iterator& selA);
    void select_sela_atomic(decltype(nodeAs)::iterator& selA);
    void select_get_oldh(decltype(nodeAs)::iterator& selA,
                         decltype(nodeHs)::iterator& oldH);
    void select_selh(decltype(nodeHs)::iterator& selH,
                     decltype(nodeHs)::iterator& avoid);
};

#endif
