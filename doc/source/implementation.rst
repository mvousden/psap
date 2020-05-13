Implementation Considerations
=============================

For reference, the simulated annealing algorithm consists of five steps:
initialisation, selection, fitness evaluation, determination, and
termination. Note that the considerable majority of execution time will be
spent in selection and fitness evaluation, so those operations should be
optimised should be the primary consideration for optimisation. An efficient
implementation of simulated annealing:

 - performs selection operations that explore the entire solution space
   efficiently. This is discussed in the :ref:`Selection` section.

 - will precompute the distances between the nodes in the hardware graph (using
   the edge weights), so that they do not need to be computed during the
   fitness evaluation stage multiple times. This is discussed in the
   :ref:`Shortest Path Precomputation` section.

 - will use appropriate data representations for the hardware graph :math:`H`,
   application graph :math:`A`, and solution structure :math:`s`. Appropriate
   selection of these structures will optimise the lookup and modification
   operations necessary for simulated annealing to function. This is discussed
   in the :ref:`Data Structures` section.

.. _Selection:

Atomic Selection
----------------

Simulated annealing mandates that the selection operation :math:`\delta(s):S\to
S` must choose a neighbouring state. One such selection mechanism would be:

 1. Select a random application node :math:`n_A` in the application graph and
    define :math:`n_{H-}` (the :math:`\{\cdot\}_-` suffix means "old") as the
    hardware node that currently contains :math:`n_A`.

 2. Select a random hardware node :math:`n_{H+}` in the hardware graph (the
 :math:`\{\cdot\}_+` suffix means "new").

 3. Select a random number :math:`i\in\mathbb{N}_0` from the range
    :math:`[0,P_\mathrm{MAX}]`.

 4. If there are less than :math:`i` application nodes mapped to the selected
    hardware node :math:`n_{H+}`:

      4-1. perform a **move** operation :math:`\delta_M(s):S\to S`, by moving
           :math:`n_A` to :math:`n_{H+}`.

    Otherwise:

      4-2. perform a **swap** operation :math:`\delta_S(s):S\to S`, by
           simultaneously moving :math:`n_A` to :math:`n_{H+}` and the
           :math:`i^\mathrm{th}` application node in :math:`n_{H+}` (or an
           application node chosen at random) to :math:`n_{H-}`.

It can be demonstrated that this operation is sufficient to explore the entire
solution space :math:`S` by applying a combination of :math:`\delta_M` and
:math:`\delta_S` to any starting solution :math:`s`.

.. _Shortest Path Precomputation:

Shortest Path Precomputation (Floyd-Warshall)
---------------------------------------------

Motivation
++++++++++

Consider the fitness functional defined in the placement problem statement,
applied to some solution :math:`s`:

:math:`F[s]=\alpha F_L[s]+F_C[s]`

:math:`F[s(M_E,c_H,\ldots)]=-\alpha\sum_{j\in E_A}\left(\sum_{i\in M_E(j)}W(i)
\right)-\sum_{k\in N_H}\left(c_H(k)^2\right)`

The fitness difference between two solutions :math:`\Delta
F=F[s_+]-F[s_-]`. Consider this difference in the case :math:`s_+=\delta(s_-)`,
which is of concern during the fitness evaluation stage of the simulated
annealing algorithm. This difference is made simpler because :math:`\delta`
only influences at most two nodes in the hardware graph (:math:`n_{H+}` and
:math:`n_{H-}`), and the appropriate edges in the application graph. The
fitness difference evaluated in this way (obtained by subtracting the old
fitness from the new fitness) from the **move** operation is:

:math:`\Delta F=-\alpha\sum_{j\in E(n_A)}\left(\sum_{i\in M_{E+}(j)}W(i)\right)
-c_{H+}(n_{H+})^2-c_{H+}(n_{H-})^2+`
:math:`\quad\alpha\sum_{j\in E(n_A)}\left(\sum_{i\in M_{E-}(j)}W(i)\right)+
c_{H-}(n_{H+})^2+c_{H-}(n_{H-})^2`

with new solution :math:`s_+(M_{E+},c_{H+},m_{n+})`, old solution
:math:`s_-(M_{E-},c_{H-},m_{n-})`, and where :math:`E(n_A):N_A\to E_A^n`
identifies the subset of :math:`E_A` connected to :math:`n_A` (i.e. the
application edges connected to this application node). Note that the story is
similar for the **swap** operation, except that there are four double summation
terms as opposed to two (to account for the extra perturbed application node).

The double summation terms in this fitness difference are expensive to compute,
because they require iteration over all of the edges that impinge on an
application node to determine all of the hardware edges affected by the change,
and then to compute the weights of all of those edges. It is reasonable to
replace this double-sum with a precomputed sum of the weights between two
hardware nodes
:math:`W_\mathrm{AGG}(n_{H,\mathrm{FROM}},n_{H,\mathrm{TO}}):(N_H,N_H)\to
\mathbb{R}^+`, in which case the fitness difference becomes:

:math:`\Delta F=-\alpha\sum_{j\in N_+(n_a)}W_\mathrm{AGG}(n_{H,+},j)-c_{H+}
(n_{H+})^2-c_{H+}(n_{H-})^2+`
:math:`\quad\alpha\sum_{j\in N_-(n_a)}W_\mathrm{AGG}(n_{H,-},j)+c_{H-}
(n_{H+})^2+c_{H-}(n_{H-})^2`

Or, in English: the fitness difference between two atomically-**move**-changed
solutions (:math:`\Delta F`) is equal to the difference between their locality
fitnesses (:math:`\Delta F_L`), weighted (:math:`\alpha`) summed with the
difference between their clustering fitnesses (:math:`\Delta F_C`). The
difference in their locality fitnesses is equal to the difference between the
sum of the weights of each hardware edge most efficiently connecting each
hardware node with an application node on them that is adjacent to the selected
application node, with the hardware node that contains the selected application
node after the move (:math:`-\sum_{j\in N_+(n_a)}W_\mathrm{AGG}(n_{H,+},j)`),
with the same case before the move (:math:`\sum_{j\in
N_-(n_a)}W_\mathrm{AGG}(n_{H,-},j)`). The difference in their clustering
fitnesses is equal to the sum of the square of the loading of the source
hardware node before the move (:math:`c_{H-}(n_{H-})^2`) with the square of the
loading of the target node after the move (:math:`c_{H-}(n_{H+})^2`), all
differenced with the same case after the move (:math:`-c_{H+}
(n_{H+})^2-c_{H+}(n_{H-})^2`).

Changing the fitness difference computation in this way significantly reduces
the amount of computation required at the fitness evaluation stage, at the cost
of imposing an additional workload at the beginning of the task computing
values for :math:`W_\mathrm{AGG}`.

Method
++++++

Let :math:`\mathbf{E}_H\in\mathbb{R}^{+}` be a matrix of size
:math:`|N_H|\times|N_H|`, which contains precomputed shortest path data after
the Floyd-Warshall algorithm has been completed. Each element in the matrix
defines the shortest path between two nodes in the hardware graph. The
Floyd-Warshall algorithm populates this matrix in two stages:

 1. **Initialisation**:

    1-1. Set the value of each entry on the diagonal equal to zero (each node
         has an implicit zero-length connection to itself).

    1-2. For each pair of nodes in the graph, if there is an edge connecting
         them together, set each value corresponding to an edge in the graph
         equal to its weight. If there is no such edge, set the value equal to
         "infinity". Generally, if :math:`i` and :math:`j` denote the indices
         of two nodes :math:`n_H` connected by an edge, then
         :math:`\mathbf{E}_H(i, j)` is set equal to the weight of that edge.

 2. **Iteration**: Given indices :math:`i`, :math:`j`, and :math:`k`:

    .. code-block::

       for k over each index,
           for i over each index,
               for j over each index,
                   set trial path <- E_H(i, k) + E_H(k, j)
                   if E_H(i, j) > trial path,
                       set E_H(i, j) <- trial path

Once the values have been computed in this way, the function
:math:`W_\mathrm{AGG}` can return the corresponding value from
:math:`\mathbf{E}_H` without further computation. Consequently, the computation
of the fitness delta :math:`\Delta F` requires no graph traversal,
significantly improving the execution time of each iteration in the simulated
annealing process.

.. _Data Structures:

Data Structure Considerations
-----------------------------
