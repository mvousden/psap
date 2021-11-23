/* A template configuration file, populated by run-many.sh. */

/* Mouse mode - useful for runtime measurements. */
bool mouseMode = {{MOUSE_MODE}};

/* Whether or not to anneal in serial, or parallel. */
bool serial = {{SERIAL_MODE}};

/* If parallel, number of workers to use. */
unsigned numWorkers = {{NUM_THREADS}};

/* Synchronisity (serial=false only) - do we synchronise to ensure no
 * computation with stale data (true), or do we only synchronise only to
 * ensure data structure integrity (false)? */
bool fullySynchronous = {{FULLY_SYNCHRONOUS}};

/* Seed, if any. */
bool useSeed = {{USE_SEED}};
Seed seed = {{SEED}};

/* How many iterations? */
Iteration maxIteration = static_cast<Iteration>({{ITERATIONS}});
