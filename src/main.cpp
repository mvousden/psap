#include "problem.hpp"

int main(int argc, char** argv)
{
    NodeH mine("mine", 0);
    NodeA yours("yours");
    Problem problem;
    problem.nodeAs[0] = std::make_shared<NodeA>("yours");
    return 1;
}
