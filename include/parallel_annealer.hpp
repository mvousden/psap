#ifndef PARALLEL_ANNEALER_HPP
#define PARALLEL_ANNEALER_HPP

#include "disorder_schedules.hpp"
#include "problem.hpp"

#include <atomic>
#include <fstream>
#include <sstream>

template <class DisorderT=ExpDecayDisorder>
class ParallelAnnealer
{
public:
    ParallelAnnealer(unsigned numThreads=1, Iteration maxIteration=100,
                     std::string csvPathRoot="");
    void operator()(Problem& problem, Iteration recordEvery=0)
        {anneal(problem, recordEvery);}

    /* Parallel compute unit */
    void co_anneal(Problem& problem, std::ofstream& csvOut,
                   Iteration maxIteration);

    static void locking_transform(Problem& problem,
                                  decltype(Problem::nodeAs)::iterator& selA,
                                  decltype(Problem::nodeHs)::iterator& selH,
                                  decltype(Problem::nodeHs)::iterator& oldH);

private:
    unsigned numThreads;
    std::atomic<Iteration> iteration = 0;
    Iteration maxIteration;
    DisorderT disorder;
    bool log = false;
    void anneal(Problem& problem, Iteration recordEvery);

    /* Output stream to a set of files. If none is provided, no output is
     * written. */
    std::string csvPathRoot;
};

#endif
