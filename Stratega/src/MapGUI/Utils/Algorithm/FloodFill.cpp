//#include <Stratega/MapGUI/Utils/Algorithms/FloodFill.h>
//
//namespace SGA {
//    std::unordered_map <std::string, std::vector<SGA::Vector2i>>
//    FloodFill::floodFillMetricHelper(double startX, double startY) {
//        std::unordered_map <std::string, std::vector<SGA::Vector2i>> fillResults;
//        std::vector <SGA::Vector2i> reachableTiles;
//        std::vector <SGA::Vector2i> reachableEntities;
//        floodFillMetricHelperUtil(startX, startY, reachableTiles, reachableEntities);
//
//        fillResults.insert({{"reachableTiles",    reachableTiles},
//                            {"reachableEntities", reachableEntities}});
//        return fillResults;
//    }
//
//    void FloodFill::floodFillMetricHelperUtil(double x, double y, std::vector <SGA::Vector2i> &visited,
//                                              std::vector <SGA::Vector2i> &visitedEntities) {
//        // base cases
//        auto pos = Vector2i((int) x, (int) y);
//        if (!board.isInBounds((int) x, (int) y))
//            return;
//        if (!board.get((int) x, (int) y).isWalkable)
//            return;
//        for (auto &vec : visited) {
//            if (vec == pos)
//                return;
//        }
//
//        // entity check
//        if (getEntityAt(Vector2f((int) x, (int) y))) {
//            visitedEntities.push_back(pos);
//        }
//        visited.push_back(pos);
//
//        floodFillMetricHelperUtil(x + 1, y, visited, visitedEntities);
//        floodFillMetricHelperUtil(x - 1, y, visited, visitedEntities);
//        floodFillMetricHelperUtil(x, y + 1, visited, visitedEntities);
//        floodFillMetricHelperUtil(x, y - 1, visited, visitedEntities);
//    }
//}