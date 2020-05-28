#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "nodes.hpp"

#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <string>
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

    /* Methods that interact with edgeCacheH. */
    void initialise_edge_cache(unsigned diameter);
    void populate_edge_cache();

    /* Initial conditions for the annealer. */
    void initial_condition_bucket();
    void initial_condition_random();

    /* Initialisation for atomic selection */
    void initialise_atomic_selector();

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

    /* State dumps */
    void write_a_h_graph(std::string path);
    void write_a_to_h_map(std::string path);
    void write_h_graph(std::string path);
    void write_h_nodes(std::string path);
    void write_h_node_loading(std::string path);

private:
    std::vector<std::vector<float>> edgeCacheH;
    std::mt19937 rng;

    /* Granular selection */
    void select_sela(decltype(nodeAs)::iterator& selA);
    void select_sela_atomic(decltype(nodeAs)::iterator& selA);
    void select_get_oldh(decltype(nodeAs)::iterator& selA,
                         decltype(nodeHs)::iterator& oldH);
    void select_selh(decltype(nodeHs)::iterator& selH,
                     decltype(nodeHs)::iterator& avoid);
};

#endif
