Review on Parallel Simulated Annealing
======================================

Simulated annealing is a stochastic global optimisation metaheuristic [1]_
[2]_. Simulated annealing is a search method that allows, in the general case,
exploration of a solution space and selection of a guaranteed local optimum,
with some concession for global search. Fundamentally, the simulated annealing
algorithm, which we refer to as Canonical Simulated Annealing (CSA) is:

 1. **Initialisation**: Define an initial state.

 2. **Selection**: Select a neighbouring state to the current state.

 3. **Fitness Evaluation**: Evaluate the fitness of the selected state, given
    the fitness of the previous state.

 4. **Determination**: If the new state is fitter, accept it without further
    question. If not, accept it depending on how "exploratory" the algorithm
    currently is.

 5. **Termination**: If a termination condition has been met, return the most
    recently accepted solution. Otherwise, go back to 2 (and select another
    state).

Or, more formally:

 1. **Initialisation**: Set state :math:`s=s_0` and :math:`n=0`.

 2. **Selection**: Select neighbouring state
    :math:`s_{\mathrm{new}}=\delta(s)`.

 3. **Fitness Evaluation**: Compute :math:`E(s_{\mathrm{new}})`, given
    :math:`E(s)`.

 4. **Determination**: If :math:`E(s_{\mathrm{new}}) < E(s)`, then

    - Set :math:`s=s_{\mathrm{new}}`.

    Otherwise, if :math:`E(s_{\mathrm{new}})\geq E(s)`, then

    - If :math:`P(E(s),E(s_{\mathrm{new}}),T(n))`, then

      - set :math:`s=s_{\mathrm{new}}`.

 6. **Termination**: If not :math:`F(s,n)`, then increment :math:`n` and go
    to 2.

 7. The output is :math:`s`.

where:

 - :math:`n\in\mathbb{N}_0`, :math:`n\in[0,n_\mathrm{max}]` is a step
   (iteration) counter.

 - :math:`S` is the set of possible mappings (i.e. the state space). In the
   context of placement, :math:`S` represents the set of possible mappings of
   the application graph onto the hardware graph. Mappings :math:`s` and
   :math:`s_n` both :math:`\in S`, and are used interchangeably.

 - :math:`s_0\in S` is an intelligently-chosen initial state. In the context of
   placement, :math:`s_0` is a mapping of the application graph onto the
   hardware graph.

 - :math:`\delta(s)\in S` is an atomic transformation on state :math:`s`, used
   to transition between "neighbouring" states in :math:`S`.

 - :math:`E(s)\in\mathbb{R}` is the fitness of state :math:`s`.

 - :math:`T(n)\in\mathbb{R}` is some disorder parameter analogous to
   temperature in traditional annealing. Must decrease monotonically from one
   to zero as :math:`n` increases from 0 to :math:`n_\mathrm{max}`. A high
   :math:`T` value corresponds to exploratory behaviour, and a low :math:`T`
   value corresponds to exploitary behaviour.

 - :math:`P(E(s),E(s_{\mathrm{new}}),T(n))\in[0,1]` is an acceptance
   probability function, which determines whether or not a worse solution is
   accepted as a function of the disorder parameter.

 - :math:`F(s,n)\in\{\text{true},\text{false}\}` is a termination condition
   (could be bound by a maximum step, a derivative of the state, or something
   else).

Simulated annealing is analogous to the annealing process applied to metal
solids or ceramic. In annealing, the material is heated to a temperature
(analogous to a high disorder parameter value in simulated annealing) that
encourage the diffusion of atoms within the material, which has the effect of
redistributing and removing dislocations in the material. Working the material
encourages this diffusion. As the material cools (and the disorder parameter
reduces), a grain structure recrystallises, where the grains nucleate and
coalesce to form larger grains in the material. Depending on the annealing
scheme, a material's ductility is increased and its hardness is reduced,
allowing for further working and shaping, followed by a hardening
process. Annealing optimises a material for further use, just as simulated
annealing optimises a solution in a solution space.

Considerations for Parallelism
------------------------------

Parallel simulated approaches in literature are typically placed within a
bilayered taxonomy [3]_. The first layer divides efficient approaches into one
of [4]_:

 - Single-trial parallel (parallel fitness delta computation).

 - Multiple-trial parallel (trials themselves are resolved independently in
   parallel).

The former of these is less relevant for the placement problem, as for graphs
of a moderate degree; the fitness delta computation itself is simply the
aggregation of the differences of the weights between nodes in the application
graph as they are mapped onto the hardware graph.

Many steps in the simulated annealing algorithm detailed above can be
parallelised to find an optimal solution more quickly. Common approaches
include [4]_, [5]_:

 - **Division**: Run a number of simulated annealing instances independently,
   each maintaining their own working state and fitness. Once the termination
   condition has been met by all workers, the best solution across all workers
   is selected as the final solution. One modification to this algorithm is to
   introduce synchronisation granularity, where the best solution between
   workers is broadcast-synchronised after certain number of "iteration steps".

 - **Clustering**: Workers share a working state and fitness, but instead
   evaluate different atomic operations independently on the same state until
   one worker identifies an operation that improves the fitness. This worker
   broadcasts this operation, or state (depending on implementation) to the
   other workers, who in turn abandon their present computation to adopt the
   new state.

 - **Error**: A Multiple-Instruction, Single-Data (MISD) implementation, where
   workers operate on the same state, and investigate atomic operations in
   parallel. So named because errors in fitness are introduced when one worker
   changes the fitness (because it has tested and accepted an operation on the
   state) while another worker is evaluating the fitness to determine whether
   or not to accept an operation it has selected. It is possible to rectify
   this by "locking" the state between evaluations in worker-local memory, but
   at considerable performance cost.

The clustering algorithm is poor when the rate of operation acceptance is high
(i.e. high temperature, at the start of the optimisation process), because many
transitions are accepted, which results in great communication and wasted work
from interruption. One strategy typically adopted is to begin with the division
algorithm with some synchronisation granularity, and to cluster workers
together when it is "efficient" to do so - this can be predicted at runtime
given the temperature schedule and current relaxation data. This approach is
best applied to problems with a compact search space and a large number of
different atomic operations, where each operation has a significant impact on
the solution structure (e.g. travelling salesman [5]_).

Error algorithms are commonly applied to traditional placement problems [6]_,
[7]_, though each application imposes slight modifications into the formulation
to exploit the problem under study. One such modification is to actively lock
colliding operations across parallel compute units [8]_. This can be achieved by
using shared-memory locks in a MISD system, or by dividing the state space into
equal regions for each process, where only operands within it’s region can be
selected by a process.

Another common modification is to introduce the concept of stages to the
algorithm [7]_, [8]_. By way of example, [7]_ divides the optimisation
problem into:

 - Global Placement: Logic elements are grouped, and these groups of logic
   elements are placed in the domain using simulated annealing (with move and
   swap operations).

 - Detailed Placement: Logic elements are moved within their groups, again
   using simulated annealing.  Elements do not leave their groups.

This approach fits well with the architecture considered by Sergey, as they
have both inter- and intra-logic block routing, better justifying the
discretisation of placement into independent subproblems. A similar approach
could be used for the placement problem, for example, given application graph
:math:`A(D, E)`, (where :math:`D` denotes the set of devices in the
application, and :math:`E` denotes the set of edges):

 - Partitioning: Partition the problem :math:`A` into smaller subproblems
   :math:`A_i` , such that :math:`A(D,E)=\bigcup\limits_{i=1}^{N} A_i(D_i,E_i)`
   possibly using Fiduccia-Mattheyses, Kernighan-Lin, or one of its derivatives
   [9]_, [10]_, [11]_

 - Inter-partition placement: Placement of the devices :math:`D_i` within each
   of the subproblems :math:`A_i` (in parallel).

 - Intra-partition placement: Position the individiual partitions :math:`A_i`
   across the compute fabric, accounting for the "spatial" requirements of each
   subproblem, and of the hardware graph.

 - Refinement: Annealing placement of :math:`D` in :math:`A` across the entire
   hardware graph. Effectively relaxes the system akin to our current approach.

.. rubric:: References
.. [1] Scott Kirkpatrick, Daniel C. Gelatt, and Mario P. Vecchi. "Optimization
   by Simulated Annealing". In: Science 220.4598 (1983), pp. 671–680. DOI:
   10.1126/science.220.4598.671. URL:
   https://science.sciencemag.org/content/220/4598/671.
.. [2] Vladimı́r Černỳ. "Thermodynamical Approach to the Traveling Salesman
   Problem: An Efficient Simulation Algorithm". In: Journal of Optimization
   Theory and Applications 45.1 (1985), pp. 41–51. DOI:
   10.1007/BF00940812. URL:
   https://link.springer.com/article/10.1007/BF00940812.
.. [3] D. Janaki Ram, T.H. Sreenivas, and K. Ganapathy Subramaniam. "Parallel
   Simulated Annealing Algorithms". In: Journal of Parallel and Distributed
   Computing 37.2 (1996), pp. 207–212. DOI: 10.1006/jpdc.1996.0121. URL:
   https://www.sciencedirect.com/science/article/pii/S0743731596901215.
.. [4] Richard W Eglese. "Simulated Annealing: a Tool for Operational
   Research". In: European Journal of Operational Research 46.3 (1990),
   pp. 271–281. DOI: 10.1016/0377-2217(90)90001-R. URL:
   https://www.sciencedirect.com/science/article/pii/037722179090001R.
.. [5] Emile Aarts and Jan Korst. Simulated annealing and Boltzmann
   machines. New York, NY; John Wiley and Sons Inc., 1988.
.. [6] Andrea Casotto, Fabio Romeo, and Alberto Sangiovanni-Vincentelli. "A
   Parallel Simulated Annealing Algorithm for the Placement of
   Macro-Cells". In: IEEE Transactions on Computer-Aided Design of Integrated
   Circuits and Systems 6.5 (1987), pp. 838–847. DOI:
   10.1109/TCAD.1987.1270327. URL:
   https://ieeexplore.ieee.org/abstract/document/1270327.
.. [7] Gavrilov Sergey, Zheleznikov Daniil, and Chochaev Rustam. "Simulated
   Annealing Based Placement Optimization for Reconfigurable
   Systems-on-Chip". In: 2019 IEEE Conference of Russian Young Re- searchers in
   Electrical and Electronic Engineering (EIConRus). IEEE. 2019,
   pp. 1597–1600. DOI: 10.1109/EIConRus.2019.8657251. URL:
   https://ieeexplore.ieee.org/abstract/document/8657251.
.. [8] Pathirikkat Gopakumar, M Jaya Bharata Reddy, and Dusmata Kumar
   Mohanta. "Pragmatic Multi- Stage Simulated Annealing for Optimal Placement
   of Synchrophasor Measurement Units in Smart Power Grids". In: Frontiers in
   Energy 9.2 (2015), pp. 148–161. DOI: 10.1007/s11708-015-0344-z. URL:
   https://link.springer.com/article/10.1007/s11708-015-0344-z.
.. [9] Charles M. Fiduccia and Robert M. Mattheyses. "A Linear-Time Heuristic
   for Improving Network Partitions". In: 19th Design Automation
   Conference. IEEE. 1982, pp. 175–181. DOI: 10.1109/DAC.1982.1585498. URL:
   https://ieeexplore.ieee.org/abstract/document/1585498.
.. [10] Brian W. Kernighan and Shen Lin. "An Efficient Heuristic Procedure for
   Partitioning Graphs". In: Bell System Technical Journal 49.2 (1970),
   pp. 291–307. DOI: 10.1002/j.1538-7305.1970.tb01770.x. URL:
   https://onlinelibrary.wiley.com/doi/abs/10.1002/j.1538-7305.1970.tb01770.x.
.. [11] George Karypis and Vipin Kumar. "A Fast and High Quality Multilevel
   Scheme for Partitioning Irregular Graphs". In: SIAM Journal on Scientific
   Computing 20.1 (1998), pp. 359–392. doi: 10.1137/ S1064827595287997. url:
   https://epubs.siam.org/doi/abs/10.1137/S1064827595287997.
