The Placement Problem in Distributed Computing
==============================================

The placement problem, in the context of distributed computing is the problem
of mapping a distributed application onto a distributed hardware platform. Such
an application can be described as a series of compute units that function
independently, but are also driven by communication between each other. Such
applications are commonplace: the finite-element is a compute unit in
finite-element analysis; the neuron is a compute unit in neural simulation; the
cell is a compute unit in a cellular automaton. The behaviour of these
applications emerges from the interaction of these compute units with one each
other.

A distributed hardware platform allows for concurrent processing of
instructions, coordinated by messaging communications functionality. Such
systems are common in the modern era of computing - a processor in a typical
desktop or laptop will have multiple compute cores, which can communicate to
solve a larger problem. Beyond the desktop, there exists traditional
supercomputing clusters used in HPC work, which are typically connected using
some messaging bus exploited by an interface (such as MPI). Graphics Processing
Units are also distributed compute platforms, as they contain a large number of
small cores adept at processing large vectorised data sets, and are seeing
increasing use for high-performance computing problems. Innovation in this area
has lead to the exploitation of Field-Programmable Gate Array (FPGA) boards
seeing use as distributed compute platforms [1]_.

Humanity's ambition grows with its vision; we are solving larger problems then
ever before, and this trend is not likely to abate. Applications are becoming
larger and larger, and are beginning to exploite the more distributed nature of
modern hardware platforms; this incentivises the development of faster,
smaller, and greater hardware platforms. One major challenge in operating these
platforms is to map the distributed application onto the distributed compute
machine such that the application performs the best that it can - this is the
placement problem.

Problem Definition
------------------

Let :math:`H(N_H,E_H)` represent a description of a distributed compute
hardware as a simple weighted graph, with nodes :math:`n_H\in N_H` representing
independent compute units, and with edges :math:`e_H\in E_H` representing
communication pathways between nodes in :math:`N_H`. Edges in :math:`E_H`, each
with a weight defined by the function :math:`W(e_H):E_H\to\mathbb{R}^+`. We
refer to a graph :math:`H` as a **hardware graph**.

Let :math:`A(N_A,E_A)` represent a distributed application as a simple graph,
with nodes :math:`n_A\in N_A` representing individual compute tasks that, when
combined appropriately, result in a functional representation of the objectives
of the program described by :math:`A`. The edges :math:`e_A\in E_A` represent
the communication behaviour between elements of `N_A`; note that these edges
are not weighted. We refer to a graph :math:`A` as an **application graph**.

The problem is to create a one-to-many map of nodes in :math:`N_A` to nodes in
:math:`N_H` such that no node :math:`n_H` has no more than
:math:`P_{\mathrm{MAX}}\in\mathbb{N}` application nodes assigned to it. Such a
mapping :math:`s` exists in the solution space :math:`S`; we refer to this
mapping :math:`s` as a **solution** to the placement problem. Formally,
:math:`s` is a tuple of functions :math:`s=(M_E,m_N,c_H)`, where:

 - :math:`M_E(e_A):E_A\to E_H^n` identifies the subset of :math:`E_H` that
   overlays :math:`e_A` (i.e. the hardware edges that are used when
   communication "happens" over :math:`e_A`),

 - :math:`m_N(n_A):N_A\to N_H` identifies the single element of :math:`N_H`
   that :math:`n_a` is mapped to, and

 - :math:`c_H(n_H):N_H\to\mathbb{N}_0` defines the number of elements mapped to
   a single element of :math:`N_H`.

In the general case, where :math:`|N_A|<|N_H|\times P_{\mathrm{MAX}}`
(colloquially, there are enough nodes in the hardware graph for it to be able
to hold the application graph), a solution can be obtained by mapping
application nodes randomly from :math:`N_A` into one hardware node :math:`n_H`,
repeating for all hardware nodes :math:`N_H`. However, such a solution is
unlikely to perform well in practice, making it undesirable.

Solution Fitness
----------------

The optimal solution for the placement problem:

 1. **Places neighbouring nodes in the application graph as close as possible
    to each other.** This minimises the latency between two communicating
    nodes. If an application requires two such nodes to synchronise with each
    other, being close together makes the synchronisation as fast as
    possible. If the nodes are far away, synchronisation is slow, compromising
    the execution time.

 2. **Loads the compute hardware as evenly as possible.** The mapping of
    multiple nodes in the application graph :math:`n_A` to one node in the
    hardware graph :math:`n_H` means that the compute unit represented by
    :math:`n_H` has to divide its time between the computation for each of the
    :math:`n_A`s loaded onto it. If the solution consists of two hardware nodes
    :math:`n_{H,0}` and :math:`n_{H,1}`, where :math:`n_{H,0}` is mapped with
    one application node, and :math:`n_{H,1}` is mapped with ten application
    nodes, then application nodes on :math:`n_{H,1}` will execute considerably
    more slowly than nodes on :math:`n_{H,0}`. For most applications, the
    slowest-executing node bottlenecks the runtime of the application, so an
    even loading is preferred.

Let the functional :math:`F[s]:S\to\mathbb{R}` define the fitness of solution
:math:`s`, where a high fitness corresponds to a desirable solution, and a low
fitness corresponds to an undesirable one. We decompose fitness
:math:`F[s]=\alpha F_L[s]+F_C[s]` into locality fitness :math:`F_L` (from the
first consideration above) and clustering fitness :math:`F_C` (from the second
consideration above). Coefficient :math:`\alpha\in\mathbb{R^+}` defines the
relative importance of these fitness contributions, as it can be observed that
:math:`F_L` and :math:`F_C` have conflicting requirements (the former prefers a
tightly-clustered solution, whereas the latter prefers a more "spread-out"
solution). Formally, these fitness functionals are:

:math:`F_L[s(M_E,\ldots)]=-\sum_{j\in E_A}\sum_{i\in M_E(j)}W(i)`

or colloquially, "The locality fitness is the sum, for each edge in the
application, of the weight of each hardware edge used by that application
edge". Also,

:math:`F_C[s(c_H,\ldots)]=-\sum_{k\in N_H}\left(c_H(k)^2\right)`

or colloquially, "The clustering fitness is the sum of the squares of the
loading of each hardware node." The square term specifically enforces a fitness
penalty on overloaded nodes.

One may observe that it is not possible to obtain a positive total, locality,
or clustering fitness value, because the edge weights on the hardware graph
:math:`W(i)` must be positive. For a given application with :math:`C` nodes,
the total fitness has upper bound :math:`-C`, due to the loading requirements
imposed in the problem definition and the computation of :math:`F_C`.

The optimal solution to this problem, outside of trivial cases, is
best obtained using a global search method, such as simulated annealing.

.. rubric:: References
.. [1] https://poets-project.org/
