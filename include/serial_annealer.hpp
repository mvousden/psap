#ifndef SERIAL_ANNEALER_HPP
#define SERIAL_ANNEALER_HPP

#include "annealer.hpp"

#include <filesystem>

template <class DisorderT=ExpDecayDisorder>
class SerialAnnealer: public Annealer<DisorderT>
{
public:
    SerialAnnealer(Iteration maxIterationArg=100,
                   const std::filesystem::path& outDirArg="");
    void operator()(Problem& problem){anneal(problem);}

private:
    Iteration iteration = 0;
    void anneal(Problem& problem);

    /* Output file names. If no output directory is provided, no output is
     * written. */
    constexpr static auto csvPath = "anneal_ops.csv";
    constexpr static auto clockPath = "wallclock.txt";
};

#endif
