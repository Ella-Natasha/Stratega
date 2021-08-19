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
#include "math.h"
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <tuple>

namespace SGA
{
    struct DistanceToTile {
        DistanceToTile(int endPosX, int endPosY, double distance):
                endPosition(endPosX, endPosY),
                distance(distance)
        {
        }

        Vector2i endPosition;
        double distance;

    };

    struct TileSafetyInfo {
        TileSafetyInfo(int x, int y, int safeForPlayerID):
                position(x, y),
                safeForPlayerID(safeForPlayerID)
        {
        }

        Vector2i position;
        int safeForPlayerID = -1;
        double safteyValue = 0;
        std::vector<int> explorableByPlayerID = {};
        bool containsEntity = false;
        std::map<int, std::vector<DistanceToTile>> distToPlayerEntities = {};
    };

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
        std::vector<Vector2f> lockedTiles;
        std::vector<Vector2f> neutralEntityPositions;
        std::unordered_map<int, std::vector<Entity>> playerOwnedEntities;
        std::unordered_map<int, std::unordered_map<std::string, std::set<SGA::Vector2i>>> playerExploration; //use for map view
        std::unordered_map<SGA::Vector2i, int> playerSafeAreas; //use for map view
        std::vector<SGA::Vector2i> reachableWalkable;
        MiniTileMap miniMap;
//        std::unordered_map<std::string, int > basicStats;
//        double asymmetricFitness;
//        double fitness;
//        bool isElite;
        bool feasible = false;

        // Map Metrics
        std::map<std::string, int> basicMapStats = {
                {"Available Resources", 0},
                {"Used Space", 0},
                {"Average Base Distance", 0},
                {"Min Base Distance", 0},
        };

        std::map<std::string, int> entityMapStats = {
                {"Safe Areas", 0},
                {"Safe Area Fairness", 0},
                {"Exploration", 0},
                {"Exploration Fairness", 0},
                {"Resource Safety", 0},
                {"Resource Safety Fairness", 0},
        };

        std::map<std::string, int> symmetryMapStats = {
                {"Horizontal Symmetry", 0},
                {"Vertical Symmetry", 0},
                {"Leading Diagonal Symmetry", 0},
                {"Antidiagonal Symmetry", 0},
        };

        std::map<std::string, int> fitnessMapStats = {
                {"Overall Symmetry", 0},
                {"Overall Player Fairness", 0},
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

        void addTileLock(const Vector2f& position);

        bool tileLockAt(const Vector2f& pos) const;

        void removeTileLockAt(Vector2f pos);

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
        std::vector<TileSafetyInfo> tileDistanceHelper();
        void calcSafeArea();
        void calcSafeAreaFairness();
        void calcExploration();
        void calcExplorationFairness();

        //fitness
        void calcSymmetry();
        void calcEntityAllocationFairness();
        void calcEntityTotalFairness();
        void getOwnedEntities();

        // floodfill
        int endMaxX = 0;
        int endMaxY = 0;
        int endMinX = 0;
        int endMinY = 0;
        std::unordered_map<std::string,std::vector<SGA::Vector2i>> floodFillMetricHelper(Vector2f& startPosition, Vector2f& endPosition);
        void floodFillMetricHelperUtil(Vector2f startPosition, Vector2f& endPosition, int& fillSize, std::vector<SGA::Vector2i>& visited, std::vector<SGA::Vector2i>& visitedEntities);
    };
}
