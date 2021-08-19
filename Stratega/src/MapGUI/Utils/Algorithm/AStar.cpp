#include <Stratega/MapGUI/Utils/Algorithms/AStar.h>

namespace SGA
{
    AStar::AStar(Grid2D<Tile>& boardGrid) :
        board(boardGrid)
    {
    }

    bool AStar::isValid(const Vector2i& pos)
    {
        // Returns true if row number and column number is in
        // range
        return (board.isInBounds(pos));
    }

    // A Utility Function to check whether the given cell is
    // blocked or not
    bool AStar::isUnBlocked(const Vector2i& pos)
    {
        // Returns true if the cell is not blocked else false
        return (isValid(pos) && board.get(pos.x, pos.y).isWalkable);
    }

    // A Utility Function to check whether destination cell has
    // been reached or not
    bool AStar::isDestination(const Vector2i& position, const Vector2i& dest)
    {
        return position == dest;
    }

    // A Utility Function to calculate the 'h' heuristics.
    double AStar::calculateHValue(const Vector2i& src, const Vector2i& dest)
    {
        // h is estimated with the chebyshev distance formula
        return src.chebyshevDistance(dest);
    }

    // A Utility Function to trace the path from the source to
    // destination
    void AStar::tracePathAndDist(const Grid2D<cell>& cellDetails, const Vector2i& dest)
    {
//        printf("\nThe Path is ");
        this->pathLength = 0;
        std::stack<Vector2i> Path;

        int y = dest.y;
        int x = dest.x;
        Vector2i next_node;
        do {
            next_node = cellDetails[{x, y}].parent;
            Path.push(next_node);
            y = next_node.y;
            x = next_node.x;
        } while (cellDetails[{x, y}].parent != next_node);

        while (!Path.empty()) {
            Vector2i p = Path.top();
            Path.pop();
            this->pathLength ++;
//            printf("-> (%d,%d) ", p.x, p.y);
        }
    }

    int AStar::pathDistance(const Grid2D<cell>& cellDetails, const Vector2i& dest)
    {
        std::stack<Vector2i> Path;
        pathLength = 0;

        int y = dest.y;
        int x = dest.x;
        Path.emplace(x, y);
        Vector2i next_node = cellDetails[{x, y}].parent;
        do {
            Path.push(next_node);
            next_node = cellDetails[{x, y}].parent;
            y = next_node.y;
            x = next_node.x;
        } while (cellDetails[{x, y}].parent != next_node);

        Path.emplace(x, y);
        while (!Path.empty()) {
            Path.pop();
            this->pathLength ++;
        }
        return pathLength;
    }

    // A Function to find the shortest path between a given
    // source cell to a destination cell according to A* Search
    // Algorithm
    void AStar::aStarSearch(const Vector2i& src, const Vector2i& dest)
    {
        if (src == dest) {
//            printf("Source equals destination\n");
            return;
        }
        // If the source is out of range
        if (!isValid(src)) {
            printf("Source is invalid\n");
            return;
        }

        // If the destination is out of range
        if (!isValid(dest)) {
            printf("Destination is invalid\n");
            return;
        }

        // Either the source or the destination is blocked
        if (!isUnBlocked(src) || !isUnBlocked(dest)) {
            printf("Source or the destination is blocked\n");
            return;
        }

        // If the destination cell is the same as source cell
        if (isDestination(src, dest)) {
            printf("We are already at the destination\n");
            return;
        }

        // Create a closed list and initialise it to false which
        // means that no cell has been included yet This closed
        Grid2D<bool> closedList = Grid2D(board.getWidth(), board.getHeight(), false);

        // Declare a 2D structure to hold the details of that cell
        Grid2D<cell> cellDetails = Grid2D(board.getWidth(), board.getHeight(), cell());

        // Initialising the parameters of the starting node
        int x = src.x;
        int y = src.y;

        cellDetails[{x, y}].f = 0.0;
        cellDetails[{x, y}].g = 0.0;
        cellDetails[{x, y}].h = 0.0;
        cellDetails[{x, y}].parent = Vector2i(x, y);

        /*
        Create an open list having information as-
        <f, <i, j>>
        where f = g + h,
        and i, j are the row and column index of that cell
        Note that 0 <= i <= ROW-1 & 0 <= j <= COL-1
        This open list is implenented as a set of tuple.*/
        std::priority_queue<Tuple, std::vector<Tuple>,
                std::greater<Tuple> >
                openList;

        // Put the starting cell on the open list and set its
        // 'f' as 0
        openList.emplace(0.0, x, y);

        // We set this boolean value as false as initially
        // the destination is not reached.
        while (!openList.empty()) {
            const Tuple& p = openList.top();
            // Add this vertex to the closed list
            x = get<1>(p); // second element of tupla
            y = get<2>(p); // third element of tupla

            // Remove this vertex from the open list
            openList.pop();
            closedList[{x, y}] = true;
            /*
            Generating all the 8 successor of this cell
                    N.W   N   N.E
                       \  |  /
                        \ | /
                    W----Cell----E
                        / | \
                       /  |  \
                    S.W   S   S.E

            Cell-->Popped Cell (i, j)
            N --> North     (i-1, j)
            S --> South     (i+1, j)
            E --> East     (i, j+1)
            W --> West         (i, j-1)
            N.E--> North-East (i-1, j+1)
            N.W--> North-West (i-1, j-1)
            S.E--> South-East (i+1, j+1)
            S.W--> South-West (i+1, j-1)
            */
            for (int add_x = -1; add_x <= 1; add_x++) {
                for (int add_y = -1; add_y <= 1; add_y++) {
                    Vector2i neighbour(x + add_x, y + add_y);
                    // Only process this cell if this is a valid
                    // one
                    if (isValid(neighbour)) {
                        // If the destination cell is the same
                        // as the current successor
                        if (isDestination(neighbour, dest)) { // Set the Parent of
                            // the destination cell
                            cellDetails[{neighbour.x , neighbour.y}].parent = { x, y };
//                            printf("The destination cell is "
//                                   "found\n");
                            tracePathAndDist(cellDetails, dest);
//                            pathDistance(cellDetails, dest);
                            return;
                        }
                            // If the successor is already on the
                            // closed list or if it is blocked, then
                            // ignore it.  Else do the following
                        else if (!closedList[{neighbour.x , neighbour.y}] && isUnBlocked(neighbour)) {
                            double gNew, hNew, fNew;
                            gNew = cellDetails[{x, y}].g + 1.0;
                            hNew = calculateHValue(neighbour, dest);
                            fNew = gNew + hNew;

                            // If it isn’t on the open list, add
                            // it to the open list. Make the
                            // current square the parent of this
                            // square. Record the f, g, and h
                            // costs of the square cell
                            //             OR
                            // If it is on the open list
                            // already, check to see if this
                            // path to that square is better,
                            // using 'f' cost as the measure.

                            //openList.emplace(0.0, y, x)
                            if (cellDetails[{neighbour.x , neighbour.y}].f == -1
                                || cellDetails[{neighbour.x , neighbour.y}].f > fNew) {
                                openList.emplace(fNew, neighbour.x, neighbour.y);

                                // Update the details of this
                                // cell
                                cellDetails[{neighbour.x , neighbour.y}].g = gNew;
                                cellDetails[{neighbour.x , neighbour.y}].h = hNew;
                                cellDetails[{neighbour.x , neighbour.y}].f = fNew;
                                cellDetails[{neighbour.x , neighbour.y}].parent = Vector2i(x, y);
                            }
                        }
                    }
                }
            }
        }

        // When the destination cell is not found and the open
        // list is empty, then we conclude that we failed to
        // reach the destiantion cell. This may happen when the
        // there is no way to destination cell (due to
        // blockages)
        printf("Failed to find the Destination Cell\n");
    }
}