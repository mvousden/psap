/* This file denotes each permutation for the annealer class template. The
 * reason for declaring classes in this way (as opposed to the traditional tpp
 * file approach) is to reduce compile time - this approach only compiles the
 * function definitions for the annealer only for objects that include this
 * file, as opposed to all objects that include the "header file" for this
 * object.
 *
 * If you prefer the TPP approach and want to use it instead of this method,
 * that's fine too. If you're in a Git repository, look at 0dfc24f. */
template class SerialAnnealer<ExpDecayDisorder>;
template class SerialAnnealer<LinearDecayDisorder>;
