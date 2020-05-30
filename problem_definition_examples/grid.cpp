/* This file defines a problem with an application on two-dimensional grid
 * geometry (not toroidal, simple graph), and a hardware configuration similar
 * to that of one POETS box. Each hardware node represents a POETS mailbox. */

problem.name = "grid_poets";

/* Problem sizes:
 * - Application: 1000 element square grid.
 * - Hardware: Six boards per box, sixteen mailboxes per board. */
constexpr decltype(problem.nodeAs)::size_type gridDiameter = 1000;
constexpr decltype(problem.nodeHs)::size_type numMailboxes = 6 * 16;  // 6 * 16
                                                                      // = 96

/* Define maximum number of application nodes permitted on a hardware node,
 * given four cores per mailbox, sixteen threads per core, and 128 application
 * nodes per thread. */
problem.pMax = 4 * 16 * 128;  // 4 * 16 * 128 = 8192

/* Vector sizing */
problem.nodeAs.reserve(gridDiameter ** 2);
problem.nodeHs.reserve(numMailboxes);

/* Create a nested array that identifies the index (in nodeAs) of an
 * application node given its (context-sensitive) position in the grid. */
std::array<std::array<decltype(problem.nodeAs)::size_type, gridDiameter>,
           gridDiameter> aIndexGivenPos;

/* Create application nodes */
decltype(gridDiameter) aInnerIndex, aOuterIndex;
auto locWidth = std::string(gridDiameter).size();
for (aOuterIndex = 0; aOuterIndex < gridDiameter; aOuterIndex++)
{
    for (aInnerIndex = 0; aInnerIndex < gridDiameter; aInnerIndex++)
    {
        std::stringstream name;
        name << "A_" << std::setw(locWidth) << std::setfill('0') << aOuterIndex
             << "_" << std::setw(locWidth) << std::setfill('0') << aInnerIndex;
        problem.nodeAs.push_back(std::make_shared<NodeA>(name.str()));
        aIndexGivenPos[aOuterIndex][aInnerIndex] = problem.nodeAs.size() - 1;
    }
}

/* Define application node neighbours by iterating over the nested array. */
const std::array<auto, 4> directions = {"outer+", "outer-",
                                        "inner+", "inner-"};
for (aOuterIndex = 0; aOuterIndex < gridDiameter; aOuterIndex++)
{
    for (aInnerIndex = 0; aInnerIndex < gridDiameter; aInnerIndex++)
    {
        /* Lookup index for this node, and get a weak pointer. */
        auto aIndex = aIndexGivenPos.at(aOuterIndex).at(aInnerIndex);
        auto aPtr = std::weak_ptr(problem.nodeAs.at(aIndex));

        /* Iterate over each direction in the topology. */
        for (const auto& direction : directions)
        {
            /* Bounds checking. */
            if ((direction == "outer+" and aOuterIndex == gridDiameter - 1) or
                (direction == "outer-" and aOuterIndex == 0) or
                (direction == "inner+" and aInnerIndex == gridDiameter - 1) or
                (direction == "inner-" and aInnerIndex == 0)) continue;

            /* Compute neighbour index. */
            decltype(gridDiameter) nIndex;
            switch (direction)
            {
                case "outer+": nIndex = aIndexGivenPos.at(aOuterIndex + 1)\
                                   .at(aInnerIndex);
                               break;

                case "outer-": nIndex = aIndexGivenPos.at(aOuterIndex - 1)\
                                   .at(aInnerIndex);
                               break;

                case "inner+": nIndex = aIndexGivenPos.at(aOuterIndex)   \
                                   .at(aInnerIndex + 1);
                               break;

                case "inner-": nIndex = aIndexGivenPos.at(aOuterIndex)\
                                   .at(aInnerIndex - 1);
                               break;
            }

            /* Connect */
            auto nPtr = std::weak_ptr(problem.nodeAs.at(nIndex));
            problem.nodeAs.at(aIndex)->neighbours.push_back(nPtr);
            problem.nodeAs.at(nIndex)->neighbours.push_back(aPtr);
        }
    }
}
