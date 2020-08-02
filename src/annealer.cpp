#include "annealer.hpp"

/* Macros for parsing the git revision preprocessor argument (if any) */
#define STRINGIFY(x) #x

#include <iomanip>
#include <utility>

template<class DisorderT>
Annealer<DisorderT>::Annealer(Iteration maxIterationArg,
                              std::filesystem::path outDirArg,
                              const char* handleArg):
    maxIteration(maxIterationArg),
    disorder(maxIterationArg),
    handle(handleArg),
    outDir(std::move(outDirArg))
{
    log = !outDir.empty();
}

/* Writes metadata to the (INI) metadata file, but only if the output path is
 * defined. */
template<class DisorderT>
void Annealer<DisorderT>::write_metadata()
{
    if (log)
    {
        /* Yay time. */
        auto datetime = std::time(nullptr);

        /* Git obtuseness */
        auto gitRevision = std::string(STRINGIFY(GIT_REVISION));
        if (gitRevision.empty()) {gitRevision = "(undefined)";}


        std::ofstream metadata;
        metadata.open((outDir / metadataName).u8string().c_str(),
                      std::ofstream::trunc);
        metadata << "[anneal]" << std::endl
                 << "annealerType = " << handle << std::endl
                 << "disorderType = " << disorder.handle << std::endl
                 << "gitRevision = " << gitRevision << std::endl
                 << "now = " << std::put_time(std::gmtime(&datetime),
                                              "%FT%T%z") << std::endl;
        metadata.close();
    }
}

#include "annealer-impl.hpp"
