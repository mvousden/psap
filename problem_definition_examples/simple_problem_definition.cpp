/* This file defines a simple problem, with four hardware nodes connected in a
 * ring with equal edge weights (each permitted to have at most two application
 * nodes associated with them), and with eight application nodes connected in a
 * ring. */

/* Reserve space in the vectors for nodes. */
unsigned nodeASize = 8;
unsigned nodeHSize = 4;
problem.nodeAs.reserve(nodeASize);
problem.nodeHs.reserve(nodeHSize);

/* Define maximum number of application nodes permitted on a hardware node. */
problem.pMax = 3;

/* Application nodes */
std::vector<std::shared_ptr<NodeA>>::size_type aIndex;
for (aIndex = 0; aIndex < nodeASize; aIndex++)
{
    std::string name = "appNode" + std::to_string(aIndex);
    problem.nodeAs.push_back(std::make_shared<NodeA>(name));
}

/* Application neighbours. */
for (aIndex = 0; aIndex < nodeASize; aIndex++)
{
    /* Identify neighbours */
    std::weak_ptr<NodeA> fwNeighbour, bwNeighbour;
    if (aIndex == nodeASize - 1)
    {
        fwNeighbour = std::weak_ptr(problem.nodeAs[0]);
        bwNeighbour = std::weak_ptr(problem.nodeAs[aIndex - 1]);
    }
    else if (aIndex == 0)
    {
        fwNeighbour = std::weak_ptr(problem.nodeAs[aIndex + 1]);
        bwNeighbour = std::weak_ptr(problem.nodeAs[nodeASize - 1]);
    }
    else
    {
        fwNeighbour = std::weak_ptr(problem.nodeAs[aIndex + 1]);
        bwNeighbour = std::weak_ptr(problem.nodeAs[aIndex - 1]);
    }

    /* Track both. */
    problem.nodeAs[aIndex]->neighbours.push_back(fwNeighbour);
    problem.nodeAs[aIndex]->neighbours.push_back(bwNeighbour);
}

/* Hardware nodes */
std::vector<std::shared_ptr<NodeH>>::size_type hIndex;
for (hIndex = 0; hIndex < nodeHSize; hIndex++)
{
    std::string name = "hwNode" + std::to_string(hIndex);
    problem.nodeHs.push_back(std::make_shared<NodeH>(name, hIndex));
}

/* Hardware neighbours */
float weight = 2;
for (hIndex = 0; hIndex < nodeHSize; hIndex++)
{
    /* Identify neighbours */
    unsigned fwNeighbour, bwNeighbour;
    if (hIndex == nodeHSize - 1)
    {
        fwNeighbour = 0;
        bwNeighbour = hIndex - 1;
    }
    else if (hIndex == 0)
    {
        fwNeighbour = hIndex + 1;
        bwNeighbour = nodeHSize - 1;
    }
    else
    {
        fwNeighbour = hIndex + 1;
        bwNeighbour = hIndex - 1;
    }

    /* Track both. */
    problem.edgeHs.push_back(std::tuple(hIndex, fwNeighbour, weight));
    problem.edgeHs.push_back(std::tuple(hIndex, bwNeighbour, weight));
}
