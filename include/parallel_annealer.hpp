#ifndef PARALLEL_ANNEALER_HPP
#define PARALLEL_ANNEALER_HPP

#include "disorder_schedules.hpp"
#include "problem.hpp"

#include <fstream>
#include <mutex>
#include <sstream>

template <class DisorderT=ExpDecayDisorder>
class ParallelAnnealer
{
public:
    ParallelAnnealer(unsigned numThreads=1, Iteration maxIteration=100,
                     std::string csvPathRoot="");
    void operator()(Problem& problem){anneal(problem);}

    /* Parallel compute unit */
    void co_anneal(Problem& problem, std::ofstream& csvOut);

private:
    unsigned numThreads;
    Iteration iteration = 0;
    Iteration maxIteration;
    DisorderT disorder;
    bool log = false;
    void anneal(Problem& problem);

    /* Output stream to a set of files. If none is provided, no output is
     * written. */
    std::string csvPathRoot;

    /* Mutex to prevent multiple simultaneous transformations (inefficient) */
    std::mutex tformMx;
};

#endif
