/* A quick and dirty config file for a bunch of properties used by main to
 * organise annealing. */

/* Mouse mode - useful for runtime measurements. */
bool mouseMode = false;

/* Whether or not to anneal in serial, or parallel. */
bool serial = false;

/* If parallel, number of workers to use. */
unsigned numWorkers = 1;

/* Synchronisity (serial=false only) - do we synchronise to ensure no
 * computation with stale data (true), or do we only synchronise only to
 * ensure data structure integrity (false)? */
bool fullySynchronous = false;

/* Seed, if any. */
bool useSeed = false;
Seed seed = 1;

/* How many iterations? */
Iteration maxIteration = static_cast<Iteration>(1e6);
