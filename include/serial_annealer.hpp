#ifndef SERIAL_ANNEALER_HPP
#define SERIAL_ANNEALER_HPP

#include "disorder_schedules.hpp"
#include "problem.hpp"

#include <filesystem>
#include <fstream>
#include <string>

template <class DisorderT=ExpDecayDisorder>
class SerialAnnealer
{
public:
    SerialAnnealer(Iteration maxIterationArg=100,
                   std::filesystem::path outDirArg="");
    void operator()(Problem& problem){anneal(problem);}

private:
    Iteration iteration = 0;
    Iteration maxIteration;
    DisorderT disorder;
    void anneal(Problem& problem);

    /* Output streams. If no output directory is provided, no output is
     * written. */
    std::filesystem::path outDir;
    constexpr static auto csvPath = "anneal_ops.csv";
    constexpr static auto clockPath = "wallclock.txt";
};

#endif
