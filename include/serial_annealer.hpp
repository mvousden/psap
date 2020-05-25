#ifndef SERIAL_ANNEALER_HPP
#define SERIAL_ANNEALER_HPP

#include "disorder_schedules.hpp"
#include "problem.hpp"

#include <fstream>
#include <string>

template <class DisorderT=ExpDecayDisorder>
class SerialAnnealer
{
public:
    SerialAnnealer(Iteration maxIteration=100,
                   std::string csvPath="");
    void operator()(Problem& problem){anneal(problem);}

private:
    Iteration iteration = 0;
    Iteration maxIteration;
    DisorderT disorder;
    void anneal(Problem& problem);

    /* Output stream to a file. If none is provided, no output is written. */
    std::string csvPath;
};

#endif
