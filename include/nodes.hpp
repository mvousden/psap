#ifndef NODES_HPP
#define NODES_HPP

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* Note that the 'lock' mutex members are synchronisation objects for the
 * parallel execution case (and are optimised-out in the serial
 * case). Application nodes are locked on selection (responsibility of the
 * problem object), whereas hardware nodes are locked on transformation
 * (responsibility of the annealer). */

class NodeH;

/* Node in the application graph. */
class NodeA
{
public:
    NodeA(std::string name): name(name){}
    std::weak_ptr<NodeH> location;
    std::string name;
    std::vector<std::weak_ptr<NodeA>> neighbours;
    std::mutex lock;
};

/* Node in the hardware graph. */
class NodeH
{
public:
    NodeH(std::string name, unsigned index): name(name), index(index){}
    NodeH(std::string name, unsigned index, float posHoriz, float posVerti):
        name(name), index(index), posHoriz(posHoriz), posVerti(posVerti){}
    std::list<std::weak_ptr<NodeA>> contents;
    std::string name;
    unsigned index;
    float posHoriz = -1;
    float posVerti = -1;
    std::mutex lock;
};

#endif
