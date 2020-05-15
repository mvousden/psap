#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include "nodes.hpp"

#include <limits>
#include <memory>
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

    Problem();

    /* Methods that interact with edgeCacheH. */
    void initialise_edge_cache(unsigned diameter);
    void populate_edge_cache();

    /* Initial conditions for the annealer. */
    void initial_condition_bucket();
    void initial_condition_random();

    /* Neighbouring state selection and transformation */
    void select(std::vector<std::shared_ptr<NodeA>>::iterator& selA,
                std::vector<std::shared_ptr<NodeH>>::iterator& selH);
    void transform(std::vector<std::shared_ptr<NodeA>>::iterator& selA,
                   std::vector<std::shared_ptr<NodeH>>::iterator& selH);

    /* Fitness calculators */
    float compute_total_fitness();

private:
    std::vector<std::vector<float>> edgeCacheH;
    std::mt19937 rng;

    /* Fitness calculators */
    float compute_app_node_locality_fitness(NodeA& nodeA);
    float compute_hw_node_clustering_fitness(NodeH& nodeH);

    /* State dumps */
    void write_a_h_graph(std::string path);
    void write_a_to_h_map(std::string path);
    void write_h_graph(std::string path);
    void write_h_node_loading(std::string path);
};

#endif
