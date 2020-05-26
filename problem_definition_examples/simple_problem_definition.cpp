/* This file defines a simple problem, with eight hardware nodes connected in a
 * ring with equal edge weights (each permitted to have at most three
 * application nodes associated with them), and with sixteen application nodes,
 * also connected in a ring in both directions.
 *
 * The optimal solution to this problem has a clustering fitness of -32, and a
 * locality fitness of -32, giving a total fitness of -64. */

problem.name = "simple_ring_problem";

/* Reserve space in the vectors for nodes. */
decltype(problem.nodeAs)::size_type nodeASize = 16;
decltype(problem.nodeHs)::size_type nodeHSize = 8;
problem.nodeAs.reserve(nodeASize);
problem.nodeHs.reserve(nodeHSize);

/* Define maximum number of application nodes permitted on a hardware node. */
problem.pMax = 3;

/* Application nodes */
decltype(problem.nodeAs)::size_type aIndex;
for (aIndex = 0; aIndex < nodeASize; aIndex++)
{
    std::string name = "appNode" + std::to_string(aIndex);
    problem.nodeAs.push_back(std::make_shared<NodeA>(name));
}

/* Application neighbours. */
for (aIndex = 0; aIndex < nodeASize; aIndex++)
{
    /* Identify neighbours */
    decltype(NodeA::neighbours)::value_type fwNeighbour, bwNeighbour;
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

/* Hardware nodes, positioned in a nice little hardcoded ring. */
decltype(problem.nodeHs)::size_type hIndex;
for (hIndex = 0; hIndex < nodeHSize; hIndex++)
{
    std::string name = "hwNode" + std::to_string(hIndex);
    if (nodeASize == 8)
    {
        float posHoriz, posVerti = -1;
        if (hIndex == 0){posHoriz = 0; posVerti = 0;}
        else if (hIndex == 1){posHoriz = 0; posVerti = 1;}
        else if (hIndex == 2){posHoriz = 0; posVerti = 2;}
        else if (hIndex == 3){posHoriz = 1; posVerti = 2;}
        else if (hIndex == 4){posHoriz = 2; posVerti = 2;}
        else if (hIndex == 5){posHoriz = 2; posVerti = 1;}
        else if (hIndex == 6){posHoriz = 2; posVerti = 0;}
        else if (hIndex == 7){posHoriz = 1; posVerti = 0;}
        problem.nodeHs.push_back(std::make_shared<NodeH>(
            name, static_cast<unsigned>(hIndex), posHoriz, posVerti));
    }
    else problem.nodeHs.push_back(std::make_shared<NodeH>(
        name, static_cast<unsigned>(hIndex)));
}

/* Hardware neighbours. It's an undirected graph, so only neighbours in one
 * direction should be tracked. */
float weight = 2;
for (hIndex = 0; hIndex < nodeHSize; hIndex++)
{
    /* Identify forward neighbour and track. */
    decltype(hIndex) fwNeighbour;
    if (hIndex == nodeHSize - 1) fwNeighbour = 0;
    else fwNeighbour = hIndex + 1;
    problem.edgeHs.push_back(std::tuple(static_cast<unsigned>(hIndex),
                                        static_cast<unsigned>(fwNeighbour),
                                        weight));
}
