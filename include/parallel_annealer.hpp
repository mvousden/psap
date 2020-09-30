#ifndef PARALLEL_ANNEALER_HPP
#define PARALLEL_ANNEALER_HPP

#include "annealer.hpp"

#include <atomic>

template <class DisorderT=ExpDecayDisorder>
class ParallelAnnealer: public Annealer<DisorderT>
{
public:
    ParallelAnnealer(unsigned numThreads=1, Iteration maxIteration=100,
                     const std::filesystem::path& outDirArg="");
    void operator()(Problem& problem, Iteration recordEvery=0,
                    bool fullySynchronous=false)
        {anneal(problem, recordEvery, fullySynchronous);}
    void operator()(Problem& problem, bool fullySynchronous=false)
        {anneal(problem, fullySynchronous);}

    /* Parallel compute unit */
    void co_anneal_synchronous(
        Problem& problem, std::ofstream& csvOut, Iteration maxIteration,
        float oldClusteringFitness, float oldLocalityFitness);
    void co_anneal_sasynchronous(
        Problem& problem, std::ofstream& csvOut, Iteration maxIteration,
        float oldClusteringFitness, float oldLocalityFitness);

    /* Transformation utilities */
    static TransformCount compute_transform_footprint(
        const decltype(Problem::nodeAs)::iterator& selA,
        const decltype(Problem::nodeHs)::iterator& selH,
        const decltype(Problem::nodeHs)::iterator& oldH);

    static void locking_transform(Problem& problem,
                                  decltype(Problem::nodeAs)::iterator& selA,
                                  decltype(Problem::nodeHs)::iterator& selH,
                                  decltype(Problem::nodeHs)::iterator& oldH);

private:
    unsigned numThreads;
    std::atomic<Iteration> iteration = 0;

    /* Anneal methods. Note that I don't use optional arguments here because
     * Annealer has a pure virtual anneal(Problem) method, and I want to
     * encapsulate both call methods. */
    void anneal(Problem& problem, Iteration recordEvery, bool fullySynchronous);
    void anneal(Problem& problem, bool fullySynchronous)
        {anneal(problem, 0, fullySynchronous);}
    void anneal(Problem& problem){anneal(problem, 0, false);}


    /* Output file names. If no output directory is provided, no output is
     * written. */
    constexpr static auto csvBaseName = "anneal_ops";
    constexpr static auto fitnessPath = "reliable_fitness_values.csv";
    constexpr static auto clockPath = "wallclock.txt";

    /* Metadata writing */
    void write_metadata();
};

#endif
