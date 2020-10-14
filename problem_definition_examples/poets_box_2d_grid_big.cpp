/* This file defines a problem with an application on two-dimensional grid
 * geometry (not toroidal, simple graph), and a hardware configuration similar
 * to that of one POETS box. Each hardware node represents a POETS core. */

problem.name = "poets_box_2d_grid";

/* Problem sizes:
 * - Application: floor((1000*1000*2)^0.5) element square grid.
 * - Hardware: Six boards per box, sixteen mailboxes per board, four cores per
 *       mailbox. Two boxes.
 * Be wary of changing coreRange - if you do, you will also need to change
 * the definitions used to define hardware node positions (posHoriz and
 * posVerti, later). */
constexpr decltype(problem.nodeAs)::size_type gridDiameter = 1414;
constexpr decltype(problem.nodeHs)::size_type boardOuterRange = 3;
constexpr decltype(boardOuterRange) boardInnerRange = 4;
constexpr decltype(boardOuterRange) mboxOuterRange = 4;
constexpr decltype(boardOuterRange) mboxInnerRange = 4;
constexpr decltype(boardOuterRange) coreRange = 4;
constexpr decltype(boardOuterRange) totCores =
    boardOuterRange * boardInnerRange *
    mboxOuterRange * mboxInnerRange * coreRange;  // 3 * 4 * 4 * 4 * 4 = 768

/* Define maximum number of application nodes permitted on a hardware node,
 * given sixteen threads per core, and 256 application nodes per thread. */
problem.pMax = 16 * 256;  // 16 * 256 = 4096

/* Vector sizing */
problem.nodeAs.reserve(gridDiameter * gridDiameter);
problem.nodeHs.reserve(totCores);

/* Define directions for connectivity of application and hardware nodes. Beware
 * of code duplication! (P=positive, N=negative). */
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
 *  4. Core co-ordinate
 * These "co-ordinates" (for lack of a better word) are the values in array
 * key. */
std::map<std::array<decltype(boardOuterRange), 5>,
         decltype(problem.nodeHs)::size_type> hIndexGivenPos;

/* Create hardware nodes */
for (decltype(problem.nodeHs)::size_type boardOuterIdx = 0;
     boardOuterIdx < boardOuterRange; boardOuterIdx++){
for (decltype(problem.nodeHs)::size_type boardInnerIdx = 0;
     boardInnerIdx < boardInnerRange; boardInnerIdx++){
for (decltype(problem.nodeHs)::size_type mboxOuterIdx = 0;
     mboxOuterIdx < mboxOuterRange; mboxOuterIdx++){
for (decltype(problem.nodeHs)::size_type mboxInnerIdx = 0;
     mboxInnerIdx < mboxInnerRange; mboxInnerIdx++){
for (decltype(problem.nodeHs)::size_type coreIdx = 0;
     coreIdx < coreRange; coreIdx++)
{
    /* Get index and determine position of the mailbox (outer = horizontal,
     * inner = vertical). Cores are arranged in a grid (best efforts), will
     * break if more than four cores are used. */
    unsigned hIndex = static_cast<unsigned>(problem.nodeHs.size());
    float posHoriz = static_cast<float>(boardOuterIdx * mboxOuterRange * 2 +
                                        mboxOuterIdx * 2 +
                                        (coreIdx & 1));
    float posVerti = static_cast<float>(boardInnerIdx * mboxInnerRange * 2 +
                                        mboxInnerIdx * 2 +
                                        (coreIdx >> 1 & 1 ));
    std::stringstream name;
    name << "H_" << boardOuterIdx
         << "_" << boardInnerIdx
         << "_" << mboxOuterIdx
         << "_" << mboxInnerIdx
         << "_" << coreIdx;
    problem.nodeHs.push_back(std::make_shared<NodeH>(name.str(), hIndex,
                                                     posHoriz, posVerti));
    hIndexGivenPos[{boardOuterIdx, boardInnerIdx,
                    mboxOuterIdx, mboxInnerIdx, coreIdx}] = hIndex;
}}}}}

/* Inter-mailbox connections and inter-board connections. */
float interMboxWeight = 100;
float interCoreWeight = 0.1;
decltype(problem.nodeHs)::size_type boardOuterIdx, boardInnerIdx,
    mboxOuterIdx, mboxInnerIdx, coreIdx;
for (boardOuterIdx = 0; boardOuterIdx < boardOuterRange; boardOuterIdx++){
for (boardInnerIdx = 0; boardInnerIdx < boardInnerRange; boardInnerIdx++){
for (mboxOuterIdx = 0; mboxOuterIdx < mboxOuterRange; mboxOuterIdx++){
for (mboxInnerIdx = 0; mboxInnerIdx < mboxInnerRange; mboxInnerIdx++){
for (coreIdx = 0; coreIdx < coreRange; coreIdx++)
{
    /* Lookup index of this core, and get a weak pointer. */
    auto hIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                                     mboxOuterIdx, mboxInnerIdx, coreIdx});
    auto hPtr = std::weak_ptr(problem.nodeHs.at(hIndex));

    /* Iterate over fellow cores in this mailbox, connecting this core to each
     * other neighbouring core. */
    for (decltype(coreIdx) nCoreIdx = 0; nCoreIdx < coreRange; nCoreIdx++)
    {
        if (coreIdx == nCoreIdx) continue;

        auto nIndex = hIndexGivenPos.at({boardOuterIdx, boardInnerIdx,
                                         mboxOuterIdx, mboxInnerIdx,
                                         nCoreIdx});
        /* Connect */
        problem.edgeHs.push_back(std::tuple(
            static_cast<unsigned>(hIndex),
            static_cast<unsigned>(nIndex), interCoreWeight));
    }

    /* Iterate over each direction in the mailbox topology. */
    for (const auto& direction : directions)
    {
        /* Bounds checking. */
        if ((direction == Dir::outerP and mboxOuterIdx == mboxOuterRange - 1) or
            (direction == Dir::outerN and mboxOuterIdx == 0) or
            (direction == Dir::innerP and mboxInnerIdx == mboxInnerRange - 1) or
            (direction == Dir::innerN and mboxInnerIdx == 0)) continue;

        /* Get neighbour positions. */
        decltype(boardOuterIdx) nBoardOuterIdx =
            std::numeric_limits<decltype(boardOuterIdx)>::max();
        decltype(boardInnerIdx) nBoardInnerIdx = nBoardOuterIdx;
        decltype(mboxOuterIdx) nMboxOuterIdx = nBoardOuterIdx;
        decltype(mboxInnerIdx) nMboxInnerIdx = nBoardOuterIdx;

        /* Compute/lookup neighbour mailbox index. */
        switch (direction)
        {
            case Dir::outerP:
                nBoardOuterIdx = boardOuterIdx;
                nBoardInnerIdx = boardInnerIdx;
                nMboxOuterIdx = mboxOuterIdx + 1;
                nMboxInnerIdx = mboxInnerIdx;
                break;

            case Dir::outerN:
                nBoardOuterIdx = boardOuterIdx;
                nBoardInnerIdx = boardInnerIdx;
                nMboxOuterIdx = mboxOuterIdx - 1;
                nMboxInnerIdx = mboxInnerIdx;
                break;

            case Dir::innerP:
                nBoardOuterIdx = boardOuterIdx;
                nBoardInnerIdx = boardInnerIdx;
                nMboxOuterIdx = mboxOuterIdx;
                nMboxInnerIdx = mboxInnerIdx + 1;
                break;

            case Dir::innerN:
                nBoardOuterIdx = boardOuterIdx;
                nBoardInnerIdx = boardInnerIdx;
                nMboxOuterIdx = mboxOuterIdx;
                nMboxInnerIdx = mboxInnerIdx - 1;
                break;
        }

        /* Connect this core to each core in the neighbouring mailbox. The cost
         * is a little wonky. */
        for (decltype(coreIdx) nCoreIdx = 0; nCoreIdx < coreRange; nCoreIdx++)
        {
            auto nIndex = hIndexGivenPos.at({nBoardOuterIdx, nBoardInnerIdx,
                                             nMboxOuterIdx, nMboxInnerIdx,
                                             nCoreIdx});
            /* Connect */
            problem.edgeHs.push_back(std::tuple(
                static_cast<unsigned>(hIndex),
                static_cast<unsigned>(nIndex), interMboxWeight));
        }
    }
}}}}}

/* Connect cores in mailboxes that cross boards. */
float interBoardWeight = 800;
for (boardOuterIdx = 0; boardOuterIdx < boardOuterRange; boardOuterIdx++){
for (boardInnerIdx = 0; boardInnerIdx < boardInnerRange; boardInnerIdx++){

    /* Attempt to make a connection for each direction in the board
     * topology. Unlike with the previous connection setup, this one is
     * unrolled because each iteration is considerably different. Sorry. */
    decltype(boardOuterIdx) nBoardOuterIdx, nBoardInnerIdx,
        nMboxOuterIdx, nMboxInnerIdx, mboxInnerIdx, mboxOuterIdx, nCoreIdx;

    /* Firstly, consider the outer+ direction. Bounds check: */
    if (boardOuterIdx != boardOuterRange - 1)
    {
        /* Compute neighbour index. */
        nBoardOuterIdx = boardOuterIdx + 1;
        nBoardInnerIdx = boardInnerIdx;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxInnerIdx = 0; mboxInnerIdx < mboxInnerRange; mboxInnerIdx++)
        {
            mboxOuterIdx = mboxOuterRange - 1;
            nMboxOuterIdx = 0;
            nMboxInnerIdx = mboxInnerIdx;
            for (coreIdx = 0; coreIdx < coreRange; coreIdx++){
            for (nCoreIdx = 0; nCoreIdx < coreRange; nCoreIdx++)
            {
                auto hIndex = hIndexGivenPos.at(
                    {boardOuterIdx, boardInnerIdx, mboxOuterIdx,
                     mboxInnerIdx, coreIdx});
                auto nIndex = hIndexGivenPos.at(
                    {nBoardOuterIdx, nBoardInnerIdx, nMboxOuterIdx,
                     nMboxInnerIdx, nCoreIdx});
                problem.edgeHs.push_back(std::tuple(
                    static_cast<unsigned>(hIndex),
                    static_cast<unsigned>(nIndex), interBoardWeight));
            }}
        }
    }

    /* Outer- direction. Bounds check: */
    if (boardOuterIdx != 0)
    {
        /* Compute neighbour index. */
        nBoardOuterIdx = boardOuterIdx - 1;
        nBoardInnerIdx = boardInnerIdx;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxInnerIdx = 0; mboxInnerIdx < mboxInnerRange; mboxInnerIdx++)
        {
            mboxOuterIdx = 0;
            nMboxOuterIdx = mboxOuterRange - 1;
            nMboxInnerIdx = mboxInnerIdx;
            for (coreIdx = 0; coreIdx < coreRange; coreIdx++){
            for (nCoreIdx = 0; nCoreIdx < coreRange; nCoreIdx++)
            {
                auto hIndex = hIndexGivenPos.at(
                    {boardOuterIdx, boardInnerIdx, mboxOuterIdx,
                     mboxInnerIdx, coreIdx});
                auto nIndex = hIndexGivenPos.at(
                    {nBoardOuterIdx, nBoardInnerIdx, nMboxOuterIdx,
                     nMboxInnerIdx, nCoreIdx});
                problem.edgeHs.push_back(std::tuple(
                    static_cast<unsigned>(hIndex),
                    static_cast<unsigned>(nIndex), interBoardWeight));
            }}
        }
    }

    /* Inner+ direction. Bounds check: */
    if (boardInnerIdx != boardInnerRange - 1)
    {
        /* Compute neighbour index. */
        nBoardOuterIdx = boardOuterIdx;
        nBoardInnerIdx = boardInnerIdx + 1;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxOuterIdx = 0; mboxOuterIdx < mboxOuterRange; mboxOuterIdx++)
        {
            mboxInnerIdx = mboxInnerRange - 1;
            nMboxInnerIdx = 0;
            nMboxOuterIdx = mboxOuterIdx;
            for (coreIdx = 0; coreIdx < coreRange; coreIdx++){
            for (nCoreIdx = 0; nCoreIdx < coreRange; nCoreIdx++)
            {
                auto hIndex = hIndexGivenPos.at(
                    {boardOuterIdx, boardInnerIdx, mboxOuterIdx,
                     mboxInnerIdx, coreIdx});
                auto nIndex = hIndexGivenPos.at(
                    {nBoardOuterIdx, nBoardInnerIdx, nMboxOuterIdx,
                     nMboxInnerIdx, nCoreIdx});
                problem.edgeHs.push_back(std::tuple(
                    static_cast<unsigned>(hIndex),
                    static_cast<unsigned>(nIndex), interBoardWeight));
            }}
        }
    }

    /* Inner- direction. Bounds check: */
    if (boardInnerIdx != 0)
    {
        /* Compute neighbour index. */
        nBoardOuterIdx = boardOuterIdx;
        nBoardInnerIdx = boardInnerIdx - 1;

        /* Connect together the mailboxes on the interface between these two
         * neighbouring boards. */
        for (mboxOuterIdx = 0; mboxOuterIdx < mboxOuterRange; mboxOuterIdx++)
        {
            mboxInnerIdx = 0;
            nMboxInnerIdx = mboxInnerRange - 1;
            nMboxOuterIdx = mboxOuterIdx;
            for (coreIdx = 0; coreIdx < coreRange; coreIdx++){
            for (nCoreIdx = 0; nCoreIdx < coreRange; nCoreIdx++)
            {
                auto hIndex = hIndexGivenPos.at(
                    {boardOuterIdx, boardInnerIdx, mboxOuterIdx,
                     mboxInnerIdx, coreIdx});
                auto nIndex = hIndexGivenPos.at(
                    {nBoardOuterIdx, nBoardInnerIdx, nMboxOuterIdx,
                     nMboxInnerIdx, nCoreIdx});
                problem.edgeHs.push_back(std::tuple(
                    static_cast<unsigned>(hIndex),
                    static_cast<unsigned>(nIndex), interBoardWeight));
            }}
        }
    }
}}
