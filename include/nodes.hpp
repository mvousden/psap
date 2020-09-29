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
 * Note that the 'lock' mutex members are synchronisation objects for the
 * parallel execution case (and are optimised-out in the serial
 * case). Application nodes are locked on selection (responsibility of the
 * problem object), whereas hardware nodes are locked on transformation
 * (responsibility of the annealer).
 *
 * The 'transformCount' atomic members are used to identify whether a node has
 * been transformed compared to a known starting point. It does not matter if
 * they wrap, as only a difference in counter value is checked for (and
 * wrapping all the way around in one iteration is nigh-on impossible with few
 * compute workers). These members are only used by parallel annealers, and are
 * ignored by serial annealers. */
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
