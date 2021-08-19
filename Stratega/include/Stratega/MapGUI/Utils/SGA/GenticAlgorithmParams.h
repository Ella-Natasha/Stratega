#pragma once
#include <unordered_map>
#include <Stratega/MapGUI/MapState.h>

namespace SGA
{
    enum class MapElitesDimensions
    {
        Symmetry,
        PlayerBalance,
        Exploration,
        SafeAreas,
        ResourceSafety
    };
    static const char *enum_str[] =
            { "Symmetry", "Player\nBalance", "Exploration", "Safe\nAreas", "Resource\nSafety" };

    struct GAParams
    {
        // MAP-Elites and SGA
        int aiEnabled = 1;
        int generationSize = 10;
        int populationSize = 12;
        int maxInitialMutation = 10;
        int generateEntities = 1;

        // MAP-Elites
        int useMapElites = 1;
        bool manualEliteSelection = false;
        int mapElitesMapWidth = 10;
        int mapElitesMapHeight = 10;
        bool dimensionChanged = false;
        bool chooseRandomElite = false;
        int zoomLevel = 9;
        MapElitesDimensions mapElitesYAxis = MapElitesDimensions::PlayerBalance;
        MapElitesDimensions mapElitesXAxis = MapElitesDimensions::Symmetry;

        std::map<std::string, MapState> generatedMaps;
        std::map<std::string, MapState> mapElitesGenMaps;
        std::unordered_map<int, TileType> tiles;
        std::unordered_map<int, EntityType> entities;
        MapState* renderedMap;

        [[nodiscard]] static double getDimensionValue(MapElitesDimensions& dimension, MapState& state)
        {
            switch (dimension)
            {
                case MapElitesDimensions::Symmetry:
                    return state.fitnessMapStats.at("Overall Symmetry");
                case MapElitesDimensions::PlayerBalance:
                    return state.fitnessMapStats.at("Overall Player Fairness");
                case MapElitesDimensions::Exploration:
                    return state.entityMapStats.at("Exploration");
                case MapElitesDimensions::SafeAreas:
                    return state.entityMapStats.at("Safe Areas");
                case MapElitesDimensions::ResourceSafety:
                    return state.entityMapStats.at("Resource Safety");
                default:
                    std::cout << "We shouldn't be here !";
                    break;
            }
        }
    };
}