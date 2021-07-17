#include <Stratega/MapGUI/MapState.h>
#include <Stratega/Representation/Vector2.h>

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
            const int entityMapIndex = (pos.y * board.getWidth() + pos.x) * 3 + pos.y;

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

        if(validateMap())
        {
            calcSymmetry();
            calcEntityAllocationFairness();
//            calcResourceSafety();
//            calcResourceSafetyFairness();
//            calcSafeArea();
//            calcSafeAreaFairness();
//            calcExploration();
//            calcExplorationFairness();
        }
        else
        {
            asymmetricFitness = 0;
            fitness = 0;
        }
    }

    void MapState::calcResourceSafety()
    {

    }

    void MapState::calcResourceSafetyFairness()
    {

    }

    void MapState::calcSafeArea()
    {

    }

    void MapState::calcSafeAreaFairness()
    {

    }

    void MapState::calcExploration()
    {
        int numEntities = 0;
        float ffSum = 0;
        for (auto& ent : entities)
        {
            if (ent.ownerID == 0)
            {
                numEntities++;
                auto& floodFillStart = entities[0].position;
                auto reachable = floodFillMetricHelper(floodFillStart.x, floodFillStart.y);

                int coverage = reachable.at("reachableTiles").size();

            }
        }

    }

    void MapState::calcExplorationFairness()
    {

    }

    bool MapState::validateMap()
    {
        if (entities.empty() || playerOwnedEntities.size() < 2)
        {
            this->feasible = false;
            return false;
        }

        auto& floodFillStart = entities[0].position;
        auto reachable = floodFillMetricHelper(floodFillStart.x, floodFillStart.y);
        auto reachableEntities = reachable.at("reachableEntities");

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
        // get number of walkable and impassable tiles
        std::vector<Tile> walkable;
        std::vector<Tile> impassable;
        for (auto& tile : board.grid)
        {
            if (tile.isWalkable)
                walkable.push_back(tile);
            else
                impassable.push_back(tile);
        }

        int boardWidth = board.getWidth();
        int boardHeight = board.getHeight();

        // get horizontal symmetry value
        // (in terms of walkable or impassable tiles specific tile type does not matter)
        int midPointHorizontal = boardHeight / 2;
        int identicalOnHorizontal = 0;

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

        //lead diagonal symmetry
//        int minDist = std::min(boardHeight, boardWidth);
        double k = boardWidth / boardHeight;
        int identicalOnLeadDiagonal = 0;

        for (int x = 0; x < boardWidth; ++x)
        {
            int middle = (int)(k * x);
            for (int y = 0; y < middle; ++y) {
                const auto& tile = board[{x, y}];
                const auto& oppositeTile = board[{boardWidth - 1 - x, boardHeight - 1 - y}];
                if (tile.isWalkable == oppositeTile.isWalkable)
                    identicalOnLeadDiagonal += 2;
            }
        }

        //opposite diagonal symmetry
        int identicalOnOppositeDiagonal = 0;
//        for (int x = 0; x < boardWidth; ++x)
//        {
//            int middle = (int)(k * x);
//            for (int y = boardHeight -1; y < middle; --y) {
//                const auto& tile = board[{x, y}];
//                const auto& oppositeTile = board[{boardWidth - 1 - x, boardHeight - 1 - y}];
//                if (tile.isWalkable == oppositeTile.isWalkable)
//                    identicalOnLeadDiagonal += 2;
//            }
//        }

        // get highest symmetry value
        int totSymmetry = (identicalOnVertical + identicalOnHorizontal + identicalOnLeadDiagonal) / 3;
        asymmetricFitness = 1 - ((double)totSymmetry / (double)(board.getHeight()*board.getWidth()));
    }

    void MapState::getOwnedEntities()
    {
        std::unordered_map<int, std::vector<Entity>> ownedEntities;
        auto& neutralEntities = basicMapStats.at("Available Resources");
        neutralEntities = 0;
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
                neutralEntities++;
            }
        }
        this->playerOwnedEntities = ownedEntities;
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
        std::cout << entityFairness;
    }

    std::unordered_map<std::string,std::vector<SGA::Vector2i>> MapState::floodFillMetricHelper(double startX, double startY)
    {
        std::unordered_map<std::string,std::vector<SGA::Vector2i>> fillResults;
        std::vector<SGA::Vector2i> reachableTiles;
        std::vector<SGA::Vector2i> reachableEntities;
        floodFillMetricHelperUtil(startX, startY, reachableTiles, reachableEntities);

        fillResults.insert({{"reachableTiles", reachableTiles}, {"reachableEntities", reachableEntities}});
        return fillResults;
    }

    void MapState::floodFillMetricHelperUtil(double x, double y, std::vector<SGA::Vector2i>& visited, std::vector<SGA::Vector2i>& visitedEntities)
    {
        // base cases
        auto pos = Vector2i((int)x, (int)y);
        if (!board.isInBounds((int)x, (int)y))
            return;
        if(!board.get((int)x,(int)y).isWalkable)
            return;
        for (auto& vec : visited)
        {
            if(vec == pos)
                return;
        }

        // entity check
        if (getEntityAt(Vector2f((int)x, (int)y)))
        {
            visitedEntities.push_back(pos);
        }
        visited.push_back(pos);

        floodFillMetricHelperUtil(x+1, y, visited, visitedEntities);
        floodFillMetricHelperUtil(x-1, y, visited, visitedEntities);
        floodFillMetricHelperUtil(x, y+1, visited, visitedEntities);
        floodFillMetricHelperUtil(x, y-1, visited, visitedEntities);
    }

    void MapState::aStarMetricHelper()
    {

    }
}
