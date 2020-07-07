#include "annealer.hpp"

template<class DisorderT>
Annealer<DisorderT>::Annealer(Iteration maxIterationArg,
                              std::filesystem::path outDirArg):
    maxIteration(maxIterationArg),
    disorder(maxIterationArg),
    outDir(outDirArg)
{
    log = !outDir.empty();
}

#include "annealer-impl.hpp"
