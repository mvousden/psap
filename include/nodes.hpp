#ifndef NODES_HPP
#define NODES_HPP

#include <list>
#include <memory>
#include <string>
#include <vector>

class NodeH;

class NodeA
{
public:
    NodeA(std::string name): name(name){}
    std::weak_ptr<NodeH> location;
    std::string name;
    std::vector<std::weak_ptr<NodeA>> neighbours;
};

class NodeH
{
public:
    NodeH(std::string name, unsigned index): name(name), index(index){}
    std::list<std::weak_ptr<NodeA>> contents;
    std::string name;
    unsigned index;
};

#endif
