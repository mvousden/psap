#ifndef PROBLEM_DEFINITION_HPP
#define PROBLEM_DEFINITION_HPP

/* This file defines a simple problem, with four hardware nodes connected in a
 * ring with equal edge weights (each permitted to have at most two application
 * nodes associated with them), and with eight application nodes connected in a
 * ring. */

/* Define the number of nodes in the application graph, and the number of nodes
 * in the hardware graph, using the C preprocessor. */
#define APPLICATION_SIZE 8
#define HARDWARE_SIZE 4

#include "problem.hpp"

/* Define a method that populates the problem object passed to it. */
void populate_problem(Problem& problem)
{
    /* Define maximum number of application nodes permitted on a hardware
     * node. */
    problem.pMax = 2;

    /* Application nodes */
    problem.nodeAs[0] = std::make_shared<NodeA>("appNode0");
    problem.nodeAs[1] = std::make_shared<NodeA>("appNode1");
    problem.nodeAs[2] = std::make_shared<NodeA>("appNode2");
    problem.nodeAs[3] = std::make_shared<NodeA>("appNode3");
    problem.nodeAs[4] = std::make_shared<NodeA>("appNode4");
    problem.nodeAs[5] = std::make_shared<NodeA>("appNode5");
    problem.nodeAs[6] = std::make_shared<NodeA>("appNode6");
    problem.nodeAs[7] = std::make_shared<NodeA>("appNode7");

    /* Application neighbours - forward */
    problem.nodeAs[0]->neighours.append(std::weak_ptr(problem.nodeAs[1]));
    problem.nodeAs[1]->neighours.append(std::weak_ptr(problem.nodeAs[2]));
    problem.nodeAs[2]->neighours.append(std::weak_ptr(problem.nodeAs[3]));
    problem.nodeAs[3]->neighours.append(std::weak_ptr(problem.nodeAs[4]));
    problem.nodeAs[4]->neighours.append(std::weak_ptr(problem.nodeAs[5]));
    problem.nodeAs[5]->neighours.append(std::weak_ptr(problem.nodeAs[6]));
    problem.nodeAs[6]->neighours.append(std::weak_ptr(problem.nodeAs[7]));
    problem.nodeAs[7]->neighours.append(std::weak_ptr(problem.nodeAs[0]));

    /* Application neighbours - reverse */
    problem.nodeAs[0]->neighours.append(std::weak_ptr(problem.nodeAs[7]));
    problem.nodeAs[1]->neighours.append(std::weak_ptr(problem.nodeAs[0]));
    problem.nodeAs[2]->neighours.append(std::weak_ptr(problem.nodeAs[1]));
    problem.nodeAs[3]->neighours.append(std::weak_ptr(problem.nodeAs[2]));
    problem.nodeAs[4]->neighours.append(std::weak_ptr(problem.nodeAs[3]));
    problem.nodeAs[5]->neighours.append(std::weak_ptr(problem.nodeAs[4]));
    problem.nodeAs[6]->neighours.append(std::weak_ptr(problem.nodeAs[5]));
    problem.nodeAs[7]->neighours.append(std::weak_ptr(problem.nodeAs[6]));

    /* Hardware nodes */
    problem.nodeHs[0] = std::make_shared<NodeH>("hwNode0");
    problem.nodeHs[1] = std::make_shared<NodeH>("hwNode1");
    problem.nodeHs[2] = std::make_shared<NodeH>("hwNode2");
    problem.nodeHs[3] = std::make_shared<NodeH>("hwNode3");

    /* Hardware neighbours - forward */
    float weight = 2;
    problem.edgeCacheH[0][1] = weight;
    problem.edgeCacheH[1][2] = weight;
    problem.edgeCacheH[2][3] = weight;
    problem.edgeCacheH[3][0] = weight;

    /* Hardware neighbours - reverse */
    problem.edgeCacheH[0][3] = weight;
    problem.edgeCacheH[1][0] = weight;
    problem.edgeCacheH[2][1] = weight;
    problem.edgeCacheH[3][2] = weight;
}

#endif
