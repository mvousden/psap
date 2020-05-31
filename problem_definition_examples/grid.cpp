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
constexpr decltype(problem.nodeHs)::size_type boardOuterRange = 3;
constexpr decltype(boardOuterRange) boardInnerRange = 2;
constexpr decltype(boardOuterRange) mboxOuterRange = 4;
constexpr decltype(boardOuterRange) mboxInnerRange = 4;
constexpr decltype(boardOuterRange) totMailboxes =
    boardOuterRange * boardInnerRange *
    mboxOuterRange * mboxInnerRange;  // 3 * 2 * 4 * 4 = 96

/* Define maximum number of application nodes permitted on a hardware node,
 * given four cores per mailbox, sixteen threads per core, and 128 application
 * nodes per thread. */
problem.pMax = 4 * 16 * 128;  // 4 * 16 * 128 = 8192

/* Vector sizing */
problem.nodeAs.reserve(gridDiameter * gridDiameter);
problem.nodeHs.reserve(totMailboxes);

/* Define directions for connectivity of application and hardware nodes. Beware
 * of code duplication! */
enum class Dir {outerP, outerN, innerP, innerN};
const std::array<Dir, 4> directions = {Dir::outerP, Dir::outerN,
                                       Dir::innerP, Dir::innerN};

/* Create a nested array that identifies the index (in nodeAs) of an
 * application node given its (context-sensitive) position in the grid. */
std::array<std::array<decltype(problem.nodeAs)::size_type, gridDiameter>,
           gridDiameter> aIndexGivenPos;

/* Create application nodes */
decltype(problem.nodeAs)::size_type aInnerIndex, aOuterIndex;
auto locWidth = std::to_string(gridDiameter).size();
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
            if ((direction == Dir::outerP and
                 aOuterIndex == gridDiameter - 1) or
                (direction == Dir::outerN and aOuterIndex == 0) or
                (direction == Dir::innerP and
                 aInnerIndex == gridDiameter - 1) or
                (direction == Dir::innerN and aInnerIndex == 0)) continue;

            /* Compute/lookup neighbour index. */
            decltype(aInnerIndex) nIndex;
            switch (direction)
            {
                case Dir::outerP: nIndex = aIndexGivenPos.at(aOuterIndex + 1)\
                                     .at(aInnerIndex);
                                  break;

                case Dir::outerN: nIndex = aIndexGivenPos.at(aOuterIndex - 1)\
                                     .at(aInnerIndex);
                                  break;

                case Dir::innerP: nIndex = aIndexGivenPos.at(aOuterIndex)\
                                     .at(aInnerIndex + 1);
                                  break;

                case Dir::innerN: nIndex = aIndexGivenPos.at(aOuterIndex)\
                                     .at(aInnerIndex - 1);
                                  break;

                default: nIndex = std::numeric_limits<decltype(nIndex)>::max();
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
 *  0. Board horizontal (outer) co-ordinate
 *  1. Board vertical (inner) co-ordinate
 *  2. Mailbox horizontal (outer) co-ordinate
 *  3. Mailbox vertical (inner) co-ordinate
 * These "co-ordinates" (for lack of a better word) are the values in array
 * key. */
std::map<std::array<decltype(boardOuterRange), 4>,
         decltype(problem.nodeHs)::size_type> hIndexGivenPos;

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
    float posHoriz = static_cast<float>(boardOuterIdx * mboxOuterRange +
                                        mboxOuterIdx);
    float posVerti = static_cast<float>(boardInnerIdx * mboxInnerRange +
                                        mboxInnerIdx);
    std::stringstream name;
    name << "H_" << boardOuterIdx
         << "_" << boardInnerIdx
         << "_" << mboxOuterIdx
         << "_" << mboxInnerIdx;
    problem.nodeHs.push_back(std::make_shared<NodeH>(name.str(), hIndex,
                                                     posHoriz, posVerti));
    hIndexGivenPos[{boardOuterIdx, boardInnerIdx,
                    mboxOuterIdx, mboxInnerIdx}] = hIndex;
}}}}

/* Connect mailboxes within each board up appropriately. */
float interMboxWeight = 100;
decltype(problem.nodeHs)::size_type boardOuterIdx, boardInnerIdx;
decltype(problem.nodeHs)::size_type mboxOuterIdx, mboxInnerIdx;
for (boardOuterIdx = 0; boardOuterIdx < boardOuterRange; boardOuterIdx++){
for (boardInnerIdx = 0; boardInnerIdx < boardInnerRange; boardInnerIdx++){
for (mboxOuterIdx = 0; mboxOuterIdx < mboxOuterRange; mboxOuterIdx++){
for (mboxInnerIdx = 0; mboxInnerIdx < mboxInnerRange; mboxInnerIdx++)
{
    /* Lookup index of this mailbox, and get a weak pointer. */
    auto hIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                                     mboxOuterIdx, mboxInnerIdx});
    auto hPtr = std::weak_ptr(problem.nodeHs.at(hIndex));

    /* Iterate over each direction in the topology. */
    for (const auto& direction : directions)
    {
        /* Bounds checking. */
        if ((direction == Dir::outerP and mboxOuterIdx == mboxOuterRange - 1) or
            (direction == Dir::outerN and mboxOuterIdx == 0) or
            (direction == Dir::innerP and mboxInnerIdx == mboxInnerRange - 1) or
            (direction == Dir::innerN and mboxInnerIdx == 0)) continue;

        /* Compute/lookup neighbour index. */
        decltype(hIndex) nIndex;
        switch (direction)
        {
            case Dir::outerP: nIndex = hIndexGivenPos.at(
                                           {boardOuterIdx, boardInnerIdx,
                                            mboxOuterIdx + 1, mboxInnerIdx});
                                       break;

            case Dir::outerN: nIndex = hIndexGivenPos.at(
                                           {boardOuterIdx, boardInnerIdx,
                                            mboxOuterIdx - 1, mboxInnerIdx});
                                       break;

            case Dir::innerP: nIndex = hIndexGivenPos.at(
                                           {boardOuterIdx, boardInnerIdx,
                                            mboxOuterIdx, mboxInnerIdx + 1});
                                       break;

            case Dir::innerN: nIndex = hIndexGivenPos.at(
                                           {boardOuterIdx, boardInnerIdx,
                                            mboxOuterIdx, mboxInnerIdx - 1});
                                       break;

            default: nIndex = std::numeric_limits<decltype(nIndex)>::max();
        }

        /* Connect */
        problem.edgeHs.push_back(std::tuple(
            static_cast<unsigned>(hIndex),
            static_cast<unsigned>(nIndex), interMboxWeight));
    }
}}}}

/* Connect mailboxes that cross boards appropriately. */
float interBoardWeight = 800;
for (boardOuterIdx = 0; boardOuterIdx < boardOuterRange; boardOuterIdx++){
for (boardInnerIdx = 0; boardInnerIdx < boardInnerRange; boardInnerIdx++){

    /* Attempt to make a connection for each direction in the topology. Unlike
     * the previous loops, this one is unrolled because each iteration is
     * considerably different. */
    decltype(boardOuterIdx) neighbourOuterIdx, neighbourInnerIdx;

    /* Firstly, consider the outer+ direction. Bounds check: */
    if (boardOuterIdx != boardOuterRange - 1)
    {
        /* Compute neighbour index. */
        neighbourOuterIdx = boardOuterIdx + 1;
        neighbourInnerIdx = boardInnerIdx;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxInnerIdx = 0; mboxInnerIdx < mboxInnerRange; mboxInnerIdx++)
        {
            auto hIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                mboxOuterRange - 1, mboxInnerIdx});
            auto nIndex = hIndexGivenPos.at({neighbourOuterIdx,
                neighbourInnerIdx, 0, mboxInnerIdx});
            problem.edgeHs.push_back(std::tuple(
                static_cast<unsigned>(hIndex),
                static_cast<unsigned>(nIndex), interBoardWeight));
        }
    }

    /* Outer- direction. Bounds check: */
    if (boardOuterIdx != 0)
    {
        /* Compute neighbour index. */
        neighbourOuterIdx = boardOuterIdx + 1;
        neighbourInnerIdx = boardInnerIdx;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxInnerIdx = 0; mboxInnerIdx < mboxInnerRange; mboxInnerIdx++)
        {
            auto hIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                0, mboxInnerIdx});
            auto nIndex = hIndexGivenPos.at({neighbourOuterIdx,
                neighbourInnerIdx, mboxOuterRange - 1, mboxInnerIdx});
            problem.edgeHs.push_back(std::tuple(
                static_cast<unsigned>(hIndex),
                static_cast<unsigned>(nIndex), interBoardWeight));
        }
    }

    /* Inner+ direction. Bounds check: */
    if (boardInnerIdx != boardInnerRange - 1)
    {
        /* Compute neighbour index. */
        neighbourOuterIdx = boardOuterIdx;
        neighbourInnerIdx = boardInnerIdx + 1;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxOuterIdx = 0; mboxOuterIdx < mboxOuterRange; mboxOuterIdx++)
        {
            auto hIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                mboxOuterIdx, mboxInnerRange - 1});
            auto nIndex = hIndexGivenPos.at({neighbourOuterIdx,
                neighbourInnerIdx, mboxOuterIdx, 0});
            problem.edgeHs.push_back(std::tuple(
                static_cast<unsigned>(hIndex),
                static_cast<unsigned>(nIndex), interBoardWeight));
        }
    }

    /* Inner+ direction. Bounds check: */
    if (boardInnerIdx != 0)
    {
        /* Compute neighbour index. */
        neighbourOuterIdx = boardOuterIdx;
        neighbourInnerIdx = boardInnerIdx - 1;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxOuterIdx = 0; mboxOuterIdx < mboxOuterRange; mboxOuterIdx++)
        {
            auto hIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                mboxOuterIdx, 0});
            auto nIndex = hIndexGivenPos.at({neighbourOuterIdx,
                neighbourInnerIdx, mboxOuterIdx, mboxInnerRange - 1});
            problem.edgeHs.push_back(std::tuple(
                static_cast<unsigned>(hIndex),
                static_cast<unsigned>(nIndex), interBoardWeight));
        }
    }
}}
