Implementation
==============

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
   in the :ref:`Data Structure` section.

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

.. _Data Structure:

PSAP Data Structure
-------------------

The primary driver for data structure types in PSAP is the iteration loop in
the simulated annealing process, as this loop consumes the majority of
execution time for large problems. Consideration must also be paid to the
structure used to define the problem. PSAP uses the Standard Template Library
(STL) available in C++17, so container types available in this standard will be
considered.

Simulated Annealing Loop
++++++++++++++++++++++++

The following table shows how the loop of simulated annealing needs to interact
with a putative data structure.

.. csv-table:: Data Structure Operations in the Inner Simulated Annealing Loop
   :widths: 5 35 60
   :file: data_structure_table.csv
   :header-rows: 1

One approach to efficiently implementing a data structure with the above
requirements would be an array-based approach, where nodes are defined by
natural-number indeces, and states are defined as entries in the arrays. This
approach is effective because the number of hardware and application nodes in
the problem does not change. The primary benefit of array-based approaches is
that they exploit spatial locality when fetching blocks from memory into the
caching system on the CPU. However, this is of negligible benefit here; spatial
locality plays little part when data is being selected at random. For improved
readability, an object-oriented approach is used in PSAP. The following UML
graph illustrates the data structure used by the annealer.

.. graphviz::

   digraph G {
   fontname="Inconsolata"
   fontsize=11;

   /* Class definitions (as graph nodes) */
   node[color="#005500",
        fillcolor="#DBFFDE:#A8FF8F",
        fontname="Inconsolata",
        fontsize=11,
        gradientangle=270,
        margin=0,
        shape="rect",
        style="filled"];

   NodeH[label=<<TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0">
   <TR><TD>NodeH</TD></TR>
   <TR><TD ALIGN="LEFT">
   + contents: set&lt;weak_ptr&lt;NodeA&gt;&gt;<BR ALIGN="LEFT"/>
   + name: string<BR ALIGN="LEFT"/>
   + index: unsigned<BR ALIGN="LEFT"/>
   </TD></TR>
   <TR><TD ALIGN="TEXT">
   None<BR ALIGN="TEXT"/>
   </TD></TR>
   <TR><TD ALIGN="TEXT">
   Node in the Hardware graph.<BR ALIGN="TEXT"/>
   </TD></TR></TABLE>>];

   NodeA[label=<<TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0">
   <TR><TD>NodeA</TD></TR>
   <TR><TD ALIGN="LEFT">
   + location: weak_ptr&lt;NodeH&gt;<BR ALIGN="LEFT"/>
   + name: string<BR ALIGN="LEFT"/>
   + neighbours: vector&lt;weak_ptr&lt;NodeA&gt;&gt;<BR ALIGN="LEFT"/>
   </TD></TR>
   <TR><TD ALIGN="TEXT">
   None<BR ALIGN="TEXT"/>
   </TD></TR>
   <TR><TD ALIGN="TEXT">
   Node in the Application graph.<BR ALIGN="TEXT"/>
   </TD></TR></TABLE>>];

   Problem[label=<<TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0">
   <TR><TD>Problem</TD></TR>
   <TR><TD ALIGN="LEFT">
   + nodeAs: array&lt;shared_ptr&lt;NodeA&gt;,X&gt;<BR ALIGN="LEFT"/>
   + nodeHs: array&lt;shared_ptr&lt;NodeH&gt;,Y&gt;<BR ALIGN="LEFT"/>
   + edgeCacheH: array&lt;array&lt;float,X&gt;,X&gt;<BR ALIGN="LEFT"/>
   + pMax: unsigned<BR ALIGN="LEFT"/>
   </TD></TR>
   <TR><TD ALIGN="TEXT">
   None<BR ALIGN="TEXT"/>
   </TD></TR>
   <TR><TD ALIGN="TEXT">
   Problem definition. X and Y define<BR ALIGN="TEXT"/>
   the problem size, and are known at<BR ALIGN="TEXT"/>
   compile time.<BR ALIGN="TEXT"/>
   </TD></TR></TABLE>>];

   /* Relationships */
   edge[color="#000000",
        fontname="Inconsolata",
        fontsize=11];

   /* One-to-many containment definitions. */
   {
       edge[arrowhead="diamond"];
       Problem -> NodeH;
       Problem -> NodeA;
   }

   /* Weak references (one-to-many) */
   {
       edge[arrowhead="odiamond"];
       NodeH -> NodeA[constraint=false];
   }

   /* Weak references (one-to-one) */
   {
       edge[arrowhead="vee"];
       NodeA -> NodeH[constraint=false];
   }

   }

Items in the data structure above map to the mathematical formulation in the
following manner:

.. csv-table:: Mapping between symbols in the mathematical model to data
               structures
   :widths: 10 25 65
   :file: math_data_mapping.csv
   :header-rows: 1

Notes:

 - Nodes for both the application and hardware graph are stored on the heap in
   shared pointers held in the problem structure. These pointers are stored in
   arrays, exploiting the fact that the problem size is known at compile time,
   supporting random access and indexing.

 - The weight cache, computed by the Floyd-Warshall algorithm, is stored as an
   array of arrays, again exploiting the fact that the problem size is known at
   compile time, while also providing fast lookup given the indices.

 - Each hardware node is aware of its index from the perspective of the
   problem, which makes looking up entries in the edge cache more efficient in
   time, while slightly increasing memory usage.

 - Each hardware node has a set of application nodes, as removal of an entry is
   fast (O(1), aside from binary tree rebalancing), and random access is
   supported. While an ``unordered_set`` would be superior for removal, it is
   not possible to define a safe hashing function on a weak pointer
   template. I'm not sure about that last point, but would be delighted to be
   proven wrong.

 - Each application node holds a reference to the hardware node that contains
   it, to facilitate operation 6 in the data structure operations table.

 - Each application node holds a vector of references to its neighbours in the
   application graph. A vector is chosen here because, while the size of this
   container is known for each node at compile time, there is no (reasonable)
   common size. Furthermore, resizing will not happen inside the simulated
   annealing loop because the neighbours are defined during problem
   definition. Consequently, since items are never removed from this container
   (it is only iterated over during operation 7 in the data structure
   operations table to obtain :math:`N_H` elements to iterate over), a vector
   incurs no time complexity penalty over a list (or similar structure).

 - The mapping component of the solution :math:`m_N(n_A):N_A\to N_H`, which
   identifies the hardware node that holds an application node, can be
   constructed by from the ``location`` member of each element of
   ``Problem::nodeAs``. The name component of each hardware node and
   application node is used to exfiltrate the data in a human-readable format.

Populating the Data Structure from a Problem Definition
+++++++++++++++++++++++++++++++++++++++++++++++++++++++

The problem definition file ``includes/problem_definition.hpp`` defines the
placement problem, and is written by the user. This file is read at
compile-time by the C preprocessor, and is used to initialise the problem
structure for PSAP. The reasoning behind this unusual way of reading in a
configuration file is that I want to save time writing a file reader for an
arbitrary case. Doing it properly would take longer than the total amount of
time I have to spend on this project. The context of the file has access to a
single variable: ``Problem problem``, whose elements can be freely
populated. The rest of PSAP expects the problem definition file to populate:

 - ``problem.nodeAs`` with shared pointers to application nodes created on the
   heap, with appropriate definitions for the ``index`` and ``name``
   fields. The ``contents`` field is expected to remain empty; this field is
   populated by the simulated annealing initialiser.

 - ``problem.nodeHs`` with shared pointers to hardware nodes created on the
   heap, with appropriate definitions for the ``neighbours`` and ``name``
   fields. The ``location`` field is expected to remain empty; this field is
   populated by the simulated annealing initialiser.

 - ``problem.pMax`` with a value limiting the number of application nodes that
   can be placed on hardware nodes.

 - ``problem.edgeCacheH`` elements with weights of nodes that are adjacent in
   the hardware graph. Prior to the problem definition, PSAP initialises all
   matrix elements with value ``std::numeric_limits<float>::max``, aside from
   the diagonal elements which are initialised to zero.

The integrity of the data structure (i.e. whether the indeces in arrays line up
with the nodes they refer to, whether lengths in the edge cache are
non-negative, or whether the names of nodes are unique) is not checked. The
onus is on the author of the problem definition file to do this, again, because
I wish to save time.

Example problem definition files exist in ``problem_definition_examples/``.
