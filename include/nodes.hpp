#ifndef NODES_HPP
#define NODES_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <set>
#include <vector>

class NodeH;  /* A circle necessary for reasonable performance. */
typedef unsigned TransformCount;

/* Nodes in general. All nodes are named.
 *
 * Locking and transformation behaviour is dependent on the properties of the
 * annealer:
 *
 * - Serial annealer: `lock` and `transformCount` are not used (and should be
 *   optimised out during compilation). Synchronisation primitives are not
 *   needed in the serial case.
 *
 * - Parallel synchronous annealer: The selected application node, the selected
 *   hardware node, the hardware node origin of the selected application node,
 *   and all neighbours of the application node are locked at selection
 *   time. The atomic `transformCount` members are not used (and should be
 *   optimised out during compilation).
 *
 * - Parallel semi-asynchronous annealer: Only the selected application node is
 *   locked at selection time. Hardware nodes are locked on
 *   transformation. This is the minimum amount of locking required to maintain
 *   the data structure, and will result in computation with stale data. The
 *   atomic `transformCount` members are used to identify whether a node has
 *   been transformed compared to a known starting point. It does not matter if
 *   they wrap, as only a difference in counter value is checked for (and
 *   wrapping all the way around in one iteration is nigh-on impossible with
 *   few compute workers). `transformCount` is only used while logging is
 *   enabled, and so should be optimised out when logging is disabled. */
class Node
{
public:
    Node(std::string name): name(name){}
    std::string name;
    std::mutex lock;
    std::atomic<TransformCount> transformCount = 0;
};

/* Node in the application graph. */
class NodeA: public Node
{
public:
    NodeA(std::string name): Node(name){}
    std::weak_ptr<NodeH> location;
    std::vector<std::weak_ptr<NodeA>> neighbours;
};

/* Node in the hardware graph. */
class NodeH: public Node
{
public:
    NodeH(std::string name, unsigned index): Node(name), index(index){}
    NodeH(std::string name, unsigned index, float posHoriz, float posVerti):
        Node(name), index(index), posHoriz(posHoriz), posVerti(posVerti){}
    std::set<NodeA*> contents;
    unsigned index;
    float posHoriz = -1;
    float posVerti = -1;
};

#endif
