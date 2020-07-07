#ifndef ANNEALER_HPP
#define ANNEALER_HPP

#include "disorder_schedules.hpp"
#include "problem.hpp"

template <class DisorderT=ExpDecayDisorder>
class Annealer
{
public:
    Annealer(Iteration maxIterationArg=100,
             std::filesystem::path outDirArg="");
    void operator()(Problem& problem){anneal(problem);}

protected:
    Iteration maxIteration;
    DisorderT disorder;
    virtual void anneal(Problem& problem) = 0;

    /* Output stuff - logging has to go somewhere. */
    std::filesystem::path outDir;
    bool log = false;
};

#endif
