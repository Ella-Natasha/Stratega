#pragma once
#include <random>
#include <unordered_map>
#include <vector>
#include <Stratega/Representation/Grid2D.h>
#include <Stratega/Representation/Tile.h>
#include <Stratega/Representation/Entity.h>
#include <Stratega/Representation/Player.h>
#include <Stratega/Representation/TileType.h>
#include <Stratega/Representation/GameInfo.h>
#include <Stratega/Configuration/GameConfig.h>
#include <Stratega/MapGUI/MiniTileMap.h>

namespace SGA
{

    /// <summary>
    /// ADD SUMMARY !
    /// </summary>
    struct MapState final
    {
        explicit MapState(Grid2D<Tile>&& board);
        MapState();

        std::shared_ptr<GameInfo> gameInfo;
        std::string mapName;
        Grid2D<Tile> board;
        std::vector<Entity> entities;
        std::unordered_map<int, std::vector<Entity>> playerOwnedEntities;
        MiniTileMap miniMap;
        std::unordered_map<std::string, int > basicStats;
        double asymmetricFitness;
        double fitness;

        bool feasible;

        // Map Metrics
        std::map<std::string, int> basicMapStats = {
//                {"Player Owned Entities", 0},
                {"Available Resources", 0},
                {"Used Space", 0},
//                {"Max Base Distance", 0},
                {"Average Base Distance", 0},
                {"Min Base Distance", 0},
        };
        std::map<std::string, int> progressbarMapStats = {
                {"Resource Safety", 52},
                {"Resource Safety Fairness", 49},
                {"Safe Area", 86},
                {"Safe Area Fairness", 93},
                {"Exploration", 63},
                {"Exploration Fairness", 17},
        };

        void calcMetrics();

        //Entities
        const Entity* getEntityAt(const Vector2f& pos) const;

        Entity* getEntity(int entityID);

        const Entity* getEntityConst(int entityID) const;

        Entity* getEntity(Vector2f pos, float maxDistance);

        int addEntity(const EntityType& type, int playerID, const Vector2f& position);

        Entity* getEntity(Vector2f pos);

        void removeEntityAt(const Vector2f& pos);

        void clearAllEntities();

        //Grid
        /// <summary>
        /// Checks if tile is occupied or the tile is walkable
        /// </summary>
        /// <param name="position">The position of the tile map</param>
        /// <returns>A boolean indicating if the tile in the given position is walkable</returns>
        bool isWalkable(const Vector2i& position);

        /// <summary>
        /// Checks if position is inside of the tile map
        /// </summary>
        /// <param name="pos">The position to be checked</param>
        /// <returns>A boolean indicating if tile map contains the position</returns>
        bool isInBounds(Vector2i pos) const;

        /// <summary>
        /// Checks if position is inside of the tile map
        /// </summary>
        /// <param name="pos">The position to be checked</param>
        /// <returns>A boolean indicating if tile map contains the position</returns>
        bool isInBounds(Vector2f pos) const;

        bool validateMap();


        std::string getBoardString(const GameConfig* config) const;

    private:
        int nextEntityID;

        // basicMapStats Metrics
        void calcPlayerOwnedEntities();
        void calcAvailableResources();
        void calcUsedSpace();
        void calcMaxBaseDistance();
        void calcAverageBaseDistance();
        void calcMinBaseDistance();

        // progressbarMapStats Metrics
        void calcResourceSafety();
        void calcResourceSafetyFairness();
        void calcSafeArea();
        void calcSafeAreaFairness();
        void calcExploration();
        void calcExplorationFairness();

        //fitness
        void calcSymmetry();
        void calcEntityAllocationFairness();
        void calcEntityPositionFairness();
        void getOwnedEntities();

        std::unordered_map<std::string,std::vector<SGA::Vector2i>> floodFillMetricHelper(double startX, double startY);
        void floodFillMetricHelperUtil(double x, double y, std::vector<SGA::Vector2i>& visited , std::vector<SGA::Vector2i>& visitedEntities);
        void aStarMetricHelper();
    };
}
