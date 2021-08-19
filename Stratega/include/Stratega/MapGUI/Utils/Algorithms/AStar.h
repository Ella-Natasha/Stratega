// A C++ Program to implement A* Search Algorithm
#pragma once
#include "math.h"
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <tuple>
#include <Stratega/MapGUI/MapState.h>

namespace SGA
{
    // Creating a shortcut for int, int pair type
    typedef std::pair<int, int> Pair;
    // Creating a shortcut for tuple<int, int, int> type
    typedef std::tuple<double, int, int> Tuple;

    // A structure to hold the necessary parameters
    struct cell {
        // Row and Column index of its parent
        Vector2i parent;
        // f = g + h
        double f, g, h;
        cell()
                : parent(-1, -1)
                , f(-1)
                , g(-1)
                , h(-1)
        {
        }
    };

    class AStar
    {
    public:
        const Grid2D<Tile> board;
        int pathLength = 0;

    public:
        explicit AStar(Grid2D<Tile>& boardGrid);

        // A Function to find the shortest path between a given
        // source cell to a destination cell according to A* Search
        // Algorithm
        void aStarSearch(const Vector2i& src, const Vector2i& dest);

    private:
        // A Utility Function to check whether given cell (row, col)
        // is a valid cell or not.
        bool isValid(const Vector2i& pos);

        // A Utility Function to check whether the given cell is
        // blocked or not
        bool isUnBlocked(const Vector2i& pos);

        // A Utility Function to check whether destination cell has
        // been reached or not
        bool isDestination(const Vector2i& position, const Vector2i& dest);

        // A Utility Function to calculate the 'h' heuristics.
        double calculateHValue(const Vector2i& src, const Vector2i& dest);

        // A Utility Function to trace the path from the source to
        // destination
        void tracePathAndDist(const Grid2D<cell>& cellDetails, const Vector2i& dest);

        int pathDistance(const Grid2D<cell>& cellDetails, const Vector2i& dest);

    };
}
