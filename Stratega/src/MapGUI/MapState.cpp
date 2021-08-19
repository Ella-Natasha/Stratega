#include <Stratega/MapGUI/MapState.h>
#include <Stratega/Representation/Vector2.h>
#include <Stratega/MapGUI/Utils/Algorithms/AStar.h>

namespace SGA
{
    MapState::MapState(Grid2D<Tile>&& board) :
            board(std::move(board)),
            nextEntityID(0)
    {
    }

    MapState::MapState() :
            board(0, 0, Tile(2, 0, 0)),
            nextEntityID(0)
    {
    }

    Entity* MapState::getEntity(int entityID)
    {
        auto iter = std::find_if(std::begin(entities), std::end(entities),
                                 [&](Entity const& p) { return p.id == entityID; });
        return iter == entities.end() ? nullptr : &*iter;
    }

    const Entity* MapState::getEntityConst(int entityID) const
    {
        auto iter = std::find_if(std::begin(entities), std::end(entities),
                                 [&](Entity const& p) { return p.id == entityID; });
        return iter == entities.end() ? nullptr : &*iter;

    }

    Entity* MapState::getEntity(Vector2f pos, float maxDistance)
    {
        for (auto& unit : entities)
        {
            if (unit.position.distance(pos) <= maxDistance)
            {
                return &unit;
            }
        }
        return nullptr;
    }

    int MapState::addEntity(const EntityType& type, int playerID, const Vector2f& position)
    {
        auto instance = type.instantiateEntity(nextEntityID);
        instance.ownerID = playerID;
        instance.position = position;
        entities.emplace_back(std::move(instance));

        nextEntityID++;
        return instance.id;
    }

    void MapState::addTileLock(const Vector2f& position)
    {
        board[{(int)position.x, (int)position.y}].locked = true;
        lockedTiles.emplace_back(position);
    }

    Entity* MapState::getEntity(Vector2f pos)
    {
        for (auto& entity : entities)
        {
            if (entity.position == pos)
                return &entity;
        }

        return nullptr;
    }

    bool MapState::isWalkable(const Vector2i& position)
    {
        Tile& targetTile = board.get(position.x, position.y);
        Entity* targetUnit = getEntity(Vector2f(position));

        return targetUnit == nullptr && targetTile.isWalkable;
    }

    bool MapState::isInBounds(Vector2i pos) const
    {
        return pos.x >= 0 && pos.x < static_cast<int>(board.getWidth()) && pos.y >= 0 && pos.y < static_cast<int>(board.getHeight());
    }

    bool MapState::isInBounds(Vector2f pos) const
    {
        return pos.x >= 0 && pos.x < board.getWidth() && pos.y >= 0 && pos.y < board.getHeight();
    }

    const Entity* MapState::getEntityAt(const Vector2f& pos) const
    {
        for(const auto& entity : entities)
        {
            if(static_cast<int>(pos.x) == static_cast<int>(entity.position.x) && static_cast<int>(pos.y) == static_cast<int>(entity.position.y))
            {
                return &entity;
            }
        }

        return nullptr;
    }

    bool MapState::tileLockAt(const Vector2f& pos) const
    {
        for(const auto& lock : lockedTiles)
        {
            if(static_cast<int>(pos.x) == static_cast<int>(lock.x) && static_cast<int>(pos.y) == static_cast<int>(lock.y))
            {
                return true;
            }
        }

        return false;
    }

    void MapState::removeEntityAt(const Vector2f& pos)
    {
        for (int i = 0; i < static_cast<int>(entities.size()); ++i)
        {
            if(static_cast<int>(pos.x) == static_cast<int>(entities.at(i).position.x) && static_cast<int>(pos.y) == static_cast<int>(entities.at(i).position.y))
            {
                entities.erase(entities.begin() + i);
            }
        }
        nextEntityID--;
        getOwnedEntities();
    }

    void MapState::removeTileLockAt(Vector2f pos)
    {
        board[{(int)pos.x, (int)pos.y}].locked = false;
        for (int i = 0; i < static_cast<int>(lockedTiles.size()); ++i)
        {
            if(static_cast<int>(pos.x) == static_cast<int>(lockedTiles.at(i).x) && static_cast<int>(pos.y) == static_cast<int>(lockedTiles.at(i).y))
            {
                lockedTiles.erase(lockedTiles.begin() + i);
            }
        }
    }

    void MapState::clearAllEntities()
    {
        entities.clear();
        nextEntityID = 0;
        getOwnedEntities();
    }

    std::string MapState::getBoardString(const GameConfig* config) const
    {
        std::string map;

        //Add tiles
        for (int y = 0; y < static_cast<int>(board.getHeight()); ++y)
        {
            for (int x = 0; x < static_cast<int>(board.getWidth()); ++x)
            {
                //Get tile type
                auto &tile = board[{x, y}];
                map += config->tileTypes.at(tile.tileTypeID).symbol;
                map += "  ";
            }
            map += "\n";
        }

        //Add entities
        for (auto& entity : entities)
        {
            auto& pos = entity.position;
            const char symbol = config->entityTypes.at(entity.typeID).symbol;
            const char ownerID = std::to_string(entity.ownerID)[0];
            const int entityMapIndex = (pos.y * (int)board.getWidth() + pos.x) * 3 + pos.y;

            map[entityMapIndex] = symbol;

            if (!entity.isNeutral())
                map[entityMapIndex + 1] = ownerID;
        }

        return map;
    }

    //TODO all metrics and associated calculations are in the
    // process of being moved to a new class.
    void MapState::calcMetrics()
    {
        getOwnedEntities();
        calcSymmetry();

        if(validateMap())
        {
            calcEntityAllocationFairness();
            calcExploration();
            calcSafeArea();
//            calcResourceSafety();
            calcEntityTotalFairness();

        }
        else
        {
            for(auto& stat : fitnessMapStats)
                stat.second = 0;
            for(auto& stat : entityMapStats)
                stat.second = 0;
        }
    }

    void MapState::calcResourceSafety()
    {

    }

    void MapState::calcSafeArea()
    {
        playerSafeAreas.clear();
        std::vector<TileSafetyInfo> playerSafeTiles;
        for (auto& reachableTile : reachableWalkable)
        {
            auto tileSafety = TileSafetyInfo(reachableTile.x, reachableTile.y, -1);
            int ownerID;
            bool containsEntity = false;
            int minDistToTile = 100;
            std::map<int, std::vector<DistanceToTile>> distToPlayerEntities;
            for (auto& player : playerOwnedEntities)
            {
                auto& playerID = player.first;
                std::vector<DistanceToTile> distToEnts;
                for (auto& ownedEntity : player.second)
                {
                    auto astar = AStar(board);
                    astar.aStarSearch(Vector2i(ownedEntity.position.x, ownedEntity.position.y), reachableTile);
//                    double distToTile = ownedEntity.position.chebyshevDistance(Vector2f(reachableTile.x, reachableTile.y));
                    double distToTile = astar.pathLength;
                    auto distanceToTile = DistanceToTile(ownedEntity.position.x, ownedEntity.position.y, distToTile);
                    distToEnts.push_back(distanceToTile);
                    if (distToTile < minDistToTile)
                    {
                        ownerID = player.first;
                        minDistToTile = distToTile;
                    }
                    else if (distToTile == minDistToTile && ownerID != playerID)
                        ownerID = -1;

                    if (distToTile == 0 || std::count(neutralEntityPositions.begin(), neutralEntityPositions.end(), Vector2f(reachableTile.x, reachableTile.y)))
                        containsEntity = true;
                }
                distToPlayerEntities.insert({playerID, distToEnts});
            }

            tileSafety.safeForPlayerID = ownerID;
            tileSafety.containsEntity = containsEntity;
            tileSafety.distToPlayerEntities = distToPlayerEntities;

            if (ownerID != -1)
            {
                double distFromOwnerEnts = 0;
                for(auto& distToEnt : distToPlayerEntities.at(ownerID))
                {
                    distFromOwnerEnts += distToEnt.distance;
                }

                double maxSafety = 0;
                double distFromNonOwnerEnts = 0;
                double min = 0;
                for (auto& player : distToPlayerEntities)
                {
                    if (ownerID != player.first)
                    {
                        for(auto& distToEnt : player.second)
                        {
                            distFromNonOwnerEnts += distToEnt.distance;
                        }
                    }
                }
                double safety = (distFromNonOwnerEnts - distFromOwnerEnts) / (distFromNonOwnerEnts + distFromOwnerEnts);
                if (safety > maxSafety)
                    maxSafety = safety;

                tileSafety.safteyValue = maxSafety;
            }

            playerSafeTiles.push_back(tileSafety);
            if (ownerID != -1)
                playerSafeAreas.insert({reachableTile, ownerID});
        }

        std::unordered_map<int, double> playerSafetyScores;
        std::unordered_map<int, double> resourceSafetyScores;
        for (auto& tile : playerSafeTiles)
        {
            if (tile.safeForPlayerID != -1)
            {
                if(playerSafetyScores.contains(tile.safeForPlayerID))
                    playerSafetyScores.at(tile.safeForPlayerID) += tile.safteyValue;
                else
                    playerSafetyScores.insert({tile.safeForPlayerID, tile.safteyValue});

                if (tile.containsEntity)
                {
                    if(resourceSafetyScores.contains(tile.safeForPlayerID))
                        resourceSafetyScores.at(tile.safeForPlayerID) += tile.safteyValue;
                    else
                        resourceSafetyScores.insert({tile.safeForPlayerID, tile.safteyValue});
                }
            }
        }

        double maxSafteyScore = 0;
        double minSafteyScore = 100;
        double totalSafetyScore = 0;
        for (auto& player : playerSafetyScores)
        {
            double playerSafetyScore = player.second;
            if(playerSafetyScore > maxSafteyScore)
                maxSafteyScore = playerSafetyScore;
            if(playerSafetyScore < minSafteyScore)
                minSafteyScore = playerSafetyScore;
            totalSafetyScore += playerSafetyScore;
        }

        double maxResourceSafteyScore = 0;
        double minResourceSafteyScore = 100;
        double totalResourceSafetyScore = 0;
        for (auto& player : resourceSafetyScores)
        {
            double resourceSafetyScore = player.second;
            if(resourceSafetyScore > maxResourceSafteyScore)
                maxResourceSafteyScore = resourceSafetyScore;
            if(resourceSafetyScore < minResourceSafteyScore)
                minResourceSafteyScore = resourceSafetyScore;
            totalResourceSafetyScore += resourceSafetyScore;
        }
        entityMapStats.at("Safe Areas") = (totalSafetyScore/reachableWalkable.size()) * 100;
        entityMapStats.at("Safe Area Fairness") = ((minSafteyScore * playerSafetyScores.size())/(maxSafteyScore * playerSafetyScores.size())) * 100;
        entityMapStats.at("Resource Safety") = (totalResourceSafetyScore/entities.size()) * 100;
        entityMapStats.at("Resource Safety Fairness") = ((minResourceSafteyScore * resourceSafetyScores.size())/(maxResourceSafteyScore * resourceSafetyScores.size())) * 100;
    }

    void MapState::calcEntityTotalFairness()
    {
        double total = 0;
        total += entityMapStats.at("Safe Area Fairness");
        total += entityMapStats.at("Exploration Fairness");
        total += entityMapStats.at("Resource Safety Fairness");
        fitnessMapStats.at("Overall Player Fairness") = total / 3;
    }

    void MapState::calcExploration()
    {
        int numPlayers = playerOwnedEntities.size();
        std::unordered_map<int, double> playerExplorationScores;
        playerExploration.clear();

        // get exploration from each owned entity to each other player owned entity
        for (auto& player : playerOwnedEntities)
        {
            std::set<Vector2i> allReachableTiles;
            std::set<Vector2i> allReachableEntities;
            int totalTilesCrossed = 0;
            auto& playerID = player.first;
            auto& ownedEntities = player.second;
            for(auto& ownedEntity : ownedEntities)
            {
                // Loop through all entities
                for (auto& entity : entities)
                {
                    // if not neutral or owned by the current player
                    if(entity.ownerID != -1 && entity.ownerID != playerID)
                    {
                        auto floodFillEnd = entity.position;
                        auto floodFillStart = ownedEntity.position;
                        auto reachable = floodFillMetricHelper(floodFillStart, floodFillEnd);
                        auto& tilesCrossed = reachable.at("reachableTiles");
                        auto& entitiesMet = reachable.at("reachableEntities");
                        totalTilesCrossed += tilesCrossed.size();

                        for(auto& tile : tilesCrossed)
                        {
                            allReachableTiles.insert(tile);
                        }
                        for(auto& ent : entitiesMet)
                        {
                            allReachableEntities.insert(ent);
                        }
                    }
                }
            }
            double avgPlayerExp = (double)totalTilesCrossed / playerOwnedEntities.at(playerID).size();
            playerExplorationScores.insert({playerID, avgPlayerExp});
            playerExploration.insert({playerID, {
                {"tilesCrossed", allReachableTiles},
                {"entitiesMet", allReachableEntities}
            }});
        }

        int totExploration = 0;
        int maxExploration = 0;
        for (auto& score : playerExplorationScores)
        {
            totExploration += score.second;
            if(score.second > maxExploration)
                maxExploration = score.second;
        }

        int totWalkable = 0;
        for (auto& tile : board.grid)
        {
            if(tile.isWalkable)
                totWalkable++;
        }

        double avgExploration = (double)totExploration/numPlayers;
        int percentExploration = ((double)avgExploration/totWalkable) * 100;
        int explorationFairness = (double)totExploration/(maxExploration * numPlayers) * 100;
        entityMapStats.at("Exploration") = percentExploration;
        entityMapStats.at("Exploration Fairness") = explorationFairness;
    }

    bool MapState::validateMap()
    {
        if (entities.empty() || playerOwnedEntities.size() < 2)
        {
            this->feasible = false;
            return false;
        }

        auto& floodFillStart = entities[0].position;
        Vector2f floodFillEnd = Vector2f(-1, -1);
        auto reachable = floodFillMetricHelper(floodFillStart, floodFillEnd);
        auto reachableEntities = reachable.at("reachableEntities");
        reachableWalkable = reachable.at("reachableTiles");

        if( reachableEntities.size() == entities.size())
        {
            this->feasible = true;
            return true;
        }

        else
        {
            this->feasible = false;
            return false;
        }

    }

    void MapState::calcSymmetry()
    {
        int boardWidth = board.getWidth();
        int boardHeight = board.getHeight();

        // get horizontal symmetry value
        // (in terms of walkable or impassable tiles specific tile type does not matter)
        int midPointHorizontal = boardHeight / 2;
        int identicalOnHorizontal = 0;
        // add center row tiles if height is odd
        if (boardHeight%2 != 0)
        {
            identicalOnHorizontal += boardWidth;
        }

        for (int x = 0; x < boardWidth; x++)
        {
            for (int y = 0; y < midPointHorizontal; y++)
            {
                int newY = boardHeight - y - 1;
                const auto& tile = board[{x, y}];
                const auto oppositeTile = board[{x, newY}];

                if (tile.isWalkable == oppositeTile.isWalkable)
                    identicalOnHorizontal += 2;
            }
        }

        // get vertical symmetry value
        // (in terms of walkable or impassable tiles specific tile type does not matter)
        int midPointVertical = boardWidth / 2;
        int identicalOnVertical = 0;
        // add center row tiles if height is odd
        if (boardWidth%2 != 0)
        {
            identicalOnVertical += boardHeight;
        }

        for (int x = 0; x < midPointVertical; ++x)
        {
            for (int y = 0; y < boardHeight; ++y)
            {
                const auto& tile = board[{x, y}];
                const auto& oppositeTile = board[{boardWidth - 1 - x, y}];

                if (tile.isWalkable == oppositeTile.isWalkable)
                    identicalOnVertical += 2;
            }
        }

        //leading diagonal symmetry
        int identicalOnLeadDiagonal = 0;
        for (int x = 0; x < boardHeight; x++)
        {
            for (int y = x; y >= 0; y--)
            {
                if (y == x)
                    identicalOnLeadDiagonal++;
                else
                {
                    const auto& tile = board[{x, y}];
                    const auto& oppositeTile = board[{y, x}];
                    if (tile.isWalkable == oppositeTile.isWalkable)
                        identicalOnLeadDiagonal += 2;
                }
            }
        }

        //antidiagonal symmetry
        int identicalOnAntiDiagonal = 0;
        int j = 0;
        for (int x = boardWidth -1; x >= 0; x--)
        {
            int i = 1;
            for (int y = j; y < boardHeight; y++)
            {
                if (y + x == boardWidth-1)
                {
                    identicalOnAntiDiagonal++;
                }
                else
                {
                    const auto& tile = board[{x, y}];
                    const auto& oppositeTile = board[{x-i, y-i}];
                    if (tile.isWalkable == oppositeTile.isWalkable)
                        identicalOnAntiDiagonal += 2;
                    i++;
                }
            }
            j++;
        }

        // get highest symmetry value
        double avgSymmetry = (double)(identicalOnVertical + identicalOnHorizontal + identicalOnLeadDiagonal + identicalOnAntiDiagonal) / 4;
        double totTiles = board.grid.size();
        int percentSymmetry = (avgSymmetry / totTiles) * 100;
        fitnessMapStats.at("Overall Symmetry") = percentSymmetry;
        symmetryMapStats.at("Horizontal Symmetry") = (identicalOnHorizontal / totTiles) * 100;
        symmetryMapStats.at("Vertical Symmetry") = (identicalOnVertical / totTiles) * 100;
        symmetryMapStats.at("Leading Diagonal Symmetry") = (identicalOnLeadDiagonal / totTiles) * 100;
        symmetryMapStats.at("Antidiagonal Symmetry") = (identicalOnAntiDiagonal / totTiles) * 100;
//        asymmetricFitness = 100 - percentSymmetry;
    }

    void MapState::getOwnedEntities()
    {
        std::unordered_map<int, std::vector<Entity>> ownedEntities;
        auto& availableEntities = basicMapStats.at("Available Resources");
        availableEntities = 0;
        neutralEntityPositions.clear();
        for (auto& entity : entities)
        {
            if (!entity.isNeutral() && !ownedEntities.contains(entity.ownerID))
            {
                std::vector<Entity> playerEntity;
                playerEntity.push_back(entity);
                ownedEntities.insert({entity.ownerID, playerEntity});
            }
            else if (!entity.isNeutral())
            {
                ownedEntities.at(entity.ownerID).push_back(entity);
            }
            else
            {
                neutralEntityPositions.push_back(entity.position);
                availableEntities++;
            }
        }
        this->playerOwnedEntities = ownedEntities;
//        basicMapStats.at("Available Resources") = neutralEntities.size();
    }

    void MapState::calcEntityAllocationFairness()
    {
        // get no. entities of each type for each player
        std::map<int, std::map<int, int>> playerEntityScores;
        std::set<int> entityIDs;
        for (auto& owner : playerOwnedEntities)
        {
            // get number of entities each player owns for basic map stats
            std::string statName = "Agent " + std::to_string(owner.first) + " Owned Entities";
            if (basicMapStats.contains(statName))
                basicMapStats.at(statName) = owner.second.size();
            else
                basicMapStats.insert({statName, owner.second.size()});

            // start Entity Allocation Fairness
            std::map<int, int> entityIdCount;
            for (auto& owned : owner.second)
            {
                entityIDs.insert(owned.typeID);
                if (!entityIdCount.contains(owned.typeID))
                {
                    entityIdCount.insert({owned.typeID, 1});
                }
                else
                {
                    entityIdCount.at(owned.typeID) += 1;
                }
            }
            playerEntityScores.insert({owner.first, entityIdCount});
        }

        // calc fairness of entities between players
        int maxNum = 0;
        int totalNum = 0;
        for (auto& id : entityIDs)
        {
            int localMax = 0;
            for (auto& entity : playerEntityScores)
            {
                if (entity.second.contains(id))
                {
                    localMax = (localMax < entity.second.at(id)) ? entity.second.at(id) : localMax;
                    totalNum += entity.second.at(id);
                }

            }
            maxNum += localMax * playerEntityScores.size();
        }

        float entityFairness = maxNum / totalNum;
    }

    std::unordered_map<std::string,std::vector<SGA::Vector2i>> MapState::floodFillMetricHelper(Vector2f& startPosition, Vector2f& endPosition)
    {
        std::unordered_map<std::string,std::vector<SGA::Vector2i>> fillResults;
        std::vector<SGA::Vector2i> reachableTiles;
        std::vector<SGA::Vector2i> reachableEntities;
        int fillSize;

        if(endPosition.x == -1)
            fillSize = std::max(board.getWidth(), board.getHeight());
        else
        {
            auto aStar = AStar(board);
            aStar.aStarSearch(Vector2i(startPosition.x, startPosition.y), Vector2i(endPosition.x, endPosition.y));
            fillSize = aStar.pathLength;
        }

        endMaxX = startPosition.x + fillSize + 1;
        endMaxY = startPosition.y + fillSize + 1;
        endMinX = startPosition.x - fillSize - 1;
        endMinY = startPosition.y - fillSize - 1;
        floodFillMetricHelperUtil(startPosition, endPosition, fillSize, reachableTiles, reachableEntities);
        fillResults.insert({{"reachableTiles", reachableTiles}, {"reachableEntities", reachableEntities}});
        return fillResults;
    }

    void MapState::floodFillMetricHelperUtil(Vector2f startPosition, Vector2f& endPosition, int& fillSize, std::vector<SGA::Vector2i>& visited, std::vector<SGA::Vector2i>& visitedEntities)
    {
        // base cases
        auto& x = startPosition.x;
        auto& y = startPosition.y;
        auto pos = Vector2i(x, y);

        if (!board.isInBounds(x, y))
            return;
        if (!board.get(x, y).isWalkable)
            return;
        if (startPosition == endPosition || x == endMaxX || x == endMinX || y == endMaxY || y == endMinY)
        {
            return;
        }
        for (auto& vec : visited)
        {
            if(vec == pos)
                return;
        }

        // entity check
        if (getEntityAt(Vector2f(x, y)))
        {
            visitedEntities.push_back(pos);
        }
        visited.push_back(pos);

        // 4 directions
        floodFillMetricHelperUtil(Vector2f(x+1, y), endPosition, fillSize, visited, visitedEntities);
        floodFillMetricHelperUtil(Vector2f(x-1, y), endPosition, fillSize, visited, visitedEntities);
        floodFillMetricHelperUtil(Vector2f(x, y+1), endPosition, fillSize, visited, visitedEntities);
        floodFillMetricHelperUtil(Vector2f(x, y-1), endPosition, fillSize, visited, visitedEntities);

        // 8 directions
        floodFillMetricHelperUtil(Vector2f(x+1, y-1), endPosition, fillSize, visited, visitedEntities);
        floodFillMetricHelperUtil(Vector2f(x-1, y-1), endPosition, fillSize, visited, visitedEntities);
        floodFillMetricHelperUtil(Vector2f(x+1, y+1), endPosition, fillSize, visited, visitedEntities);
        floodFillMetricHelperUtil(Vector2f(x-1, y+1), endPosition, fillSize, visited, visitedEntities);
    }

}
