#ifndef NODES_HPP
#define NODES_HPP

#include <memory>
#include <set>
#include <string>
#include <vector>

class NodeH;

class NodeA
{
public:
    std::weak_ptr<NodeH> location;
    std::string name;
    std::vector<std::weak_ptr<NodeA>> neighbours;
};

class NodeH
{
public:
    std::set<std::weak_ptr<NodeA>> contents;
    std::string name;
    unsigned index;
};

#endif
