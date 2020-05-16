#ifndef SERIAL_ANNEALER_HPP
#define SERIAL_ANNEALER_HPP

#include "disorder_schedules.hpp"
#include "problem.hpp"

template <class DisorderT=ExpDecayDisorder>
class SerialAnnealer
{
public:
    SerialAnnealer(Iteration maxIteration);
    void operator()(Problem& problem){anneal(problem);}
    Iteration maxIteration;

private:
    Iteration iteration = 0;
    DisorderT disorder;
    void anneal(Problem& problem);
};

#endif
