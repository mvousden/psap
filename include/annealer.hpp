#ifndef ANNEALER_HPP
#define ANNEALER_HPP

#include "disorder_schedules.hpp"
#include "problem.hpp"

#include <fstream>
#include <string>

template <class DisorderT=ExpDecayDisorder>
class Annealer
{
public:
    Annealer(Iteration maxIterationArg=100,
             std::filesystem::path outDirArg="",
             const char* handleArg="",
             Seed disorderSeed=kSeedSkip);
    void operator()(Problem& problem){anneal(problem);}

protected:
    Iteration maxIteration;
    DisorderT disorder;
    std::string handle = "Annealer (undefined)";
    virtual void anneal(Problem& problem) = 0;

    /* Output stuff - logging has to go somewhere. */
    std::filesystem::path outDir;
    bool log = false;

    /* Metadata information */
    void write_metadata();
    constexpr static auto metadataName = "metadata.txt";
};

#endif
