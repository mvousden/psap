/* This file defines a problem with an application on two-dimensional grid
 * geometry (not toroidal, simple graph), and a hardware configuration similar
 * to that of one POETS box. Each hardware node represents a POETS mailbox. */

problem.name = "grid_poets";

/* Problem sizes:
 * - Application: 1000 element square grid.
 * - Hardware: Six boards per box, sixteen mailboxes per board.
 * Be wary of changing numMailboxes - if you do, you will also need to change
 * the definitions used by hIndexGivenPos (later). */
constexpr decltype(problem.nodeAs)::size_type gridDiameter = 1000;
constexpr decltype(problem.nodeHs)::size_type totMailboxes, boardOuterRange,
    boardInnerRange, mboxOuterRange, mboxInnerRange;
boardOuterRange = 3;
boardInnerRange = 2;
mboxOuterRange = 4;
mboxInnerRange = 4;
totMailboxes = boardOuterRange * boardInnerRange *
    mboxOuterRange * mboxInnerRange;  // 3 * 2 * 4 * 4 = 96

/* Define maximum number of application nodes permitted on a hardware node,
 * given four cores per mailbox, sixteen threads per core, and 128 application
 * nodes per thread. */
problem.pMax = 4 * 16 * 128;  // 4 * 16 * 128 = 8192

/* Vector sizing */
problem.nodeAs.reserve(gridDiameter ** 2);
problem.nodeHs.reserve(totMailboxes);

/* Define directions for connectivity of application and hardware nodes. */
const std::array<auto, 4> directions = {"outer+", "outer-",
                                        "inner+", "inner-"};

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

            /* Compute/lookup neighbour index. */
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

/* Hardware nodes - this one's a little more complicated. This map
 * identifies the index (in nodeHs) of a hardware node given its (in sequence):
 *  0. Board horizontal co-ordinate
 *  1. Board vertical co-ordinate
 *  2. Mailbox horizontal co-ordinate
 *  3. Mailbox vertical co-ordinate
 * These "co-ordinates" (for lack of a better word) are the values in array
 * key. */
std::map<std:array<decltype(problem.nodeHs)::size_type, 4>,  // Co-ordinate
         decltype(problem.nodeHs)::size_type> hIndexGivenPos // Index

/* Create hardware nodes */
for (decltype(problem.nodeHs)::size_type boardOuterIdx = 0;
     boardOuterIdx < boardOuterRange; boardOuterIdx++){
for (decltype(problem.nodeHs)::size_type boardInnerIdx = 0;
     boardInnerIdx < boardInnerRange; boardInnerIdx++){
for (decltype(problem.nodeHs)::size_type mboxOuterIdx = 0;
     mboxOuterIdx < mboxOuterRange; mboxOuterIdx++){
for (decltype(problem.nodeHs)::size_type mboxInnerIdx = 0;
     mboxInnerIdx < mboxInnerRange; mboxInnerIdx++)
{
    /* Get index and determine position (outer = horizontal, inner =
     * vertical) */
    unsigned hIndex = static_cast<unsigned>(problem.nodeHs.size());
    float posHoriz = boardOuterIdx * mboxOuterRange + mboxOuterIdx;
    float posVerti = boardInnerIdx * mboxInnerRange + mboxInnerIdx;
    std::stringstream name;
    name << "H_" << boardOuterIdx
         << "_" << boardInnerIdx
         << "_" << mboxOuterIdx
         << "_" << mboxInnerIdx;
    problem.nodeHs.push_back(std::make_shared<NodeH>(name.str(), hIndex,
                                                     posHosiz, posVerti));
    hIndexGivenPos[{boardOuterIdx, boardInnerIdx,
                    mboxOuterIdx, mboxInnerIdx}] = hIndex;
}}}}

/* Connect mailboxes within each board up appropriately. */
float interMboxWeight = 100;
for (decltype(problem.nodeHs)::size_type boardOuterIdx = 0;
     boardOuterIdx < boardOuterRange; boardOuterIdx++){
for (decltype(problem.nodeHs)::size_type boardInnerIdx = 0;
     boardInnerIdx < boardInnerRange; boardInnerIdx++){
for (decltype(problem.nodeHs)::size_type mboxOuterIdx = 0;
     mboxOuterIdx < mboxOuterRange; mboxOuterIdx++){
for (decltype(problem.nodeHs)::size_type mboxInnerIdx = 0;
     mboxInnerIdx < mboxInnerRange; mboxInnerIdx++)
{
    /* Lookup index of this mailbox, and get a weak pointer. */
    auto hIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                                     mboxOuterIdx, mboxInnerIdx});
    auto hPtr = std::weak_ptr(problem.nodeHs.at(hIndex));

    /* Iterate over each direction in the topology. */
    for (const auto& direction : directions)
    {
        /* Bounds checking. */
        if ((direction == "outer+" and mboxOuterIdx == mboxOuterRange - 1) or
            (direction == "outer-" and mboxOuterIdx == 0) or
            (direction == "inner+" and mboxInnerIdx == mboxInnerRange - 1) or
            (direction == "inner-" and mboxInnerIdx == 0)) continue;

        /* Compute/lookup neighbour index. */
        decltype(gridDiameter) nIndex;
        switch (direction)
        {
            case "outer+": nIndex = hIndexGivenPos.at(
                {boardOuterIdx, boardInnerIdx,
                 mboxOuterIdx + 1, mboxInnerIdx});
                break;

            case "outer-": nIndex = hIndexGivenPos.at(
                {boardOuterIdx, boardInnerIdx,
                 mboxOuterIdx - 1, mboxInnerIdx});
                break;

            case "inner+": nIndex = hIndexGivenPos.at(
                {boardOuterIdx, boardInnerIdx,
                 mboxOuterIdx, mboxInnerIdx + 1});
                break;

            case "inner-": nIndex = hIndexGivenPos.at(
                {boardOuterIdx, boardInnerIdx,
                 mboxOuterIdx, mboxInnerIdx - 1});
                break;
        }

        /* Connect */
        problem.edgeHs.push_back(std::tuple(hIndex, nIndex, interMboxWeight);
}}}}

/* Connect mailboxes that cross boards appropriately. */
float interBoardWeight = 800;
