#pragma once
#include <vector>
#include <unordered_map>
#include <Stratega/MapGUI/MapState.h>
#include <Stratega/MapGUI/Utils/SGA/GenticAlgorithmParams.h>

namespace SGA
{
    struct TileType;
    struct EntityType;

    class SimpleGeneticAlgorithm
    {
    public:
        explicit SimpleGeneticAlgorithm(GAParams& mapGenerationParams);
        SimpleGeneticAlgorithm();

        std::vector<MapState> run(GAParams& mapGenerationParams);
        std::vector<MapState> runForMapElites(GAParams& mapGenerationParams, bool forceX, bool increase);
        bool compareFitness(MapState &a, MapState &b);

    private:
        const MapState* initMap;
        bool generateEntities = true;
        std::unordered_map<int, TileType> tileGenes;
        std::unordered_map<int, TileType> WalkableTileGenes;
        std::unordered_map<int, TileType> NonWalkableTileGenes;
        std::unordered_map<int, EntityType> entityGenes;

        static int randomNum(int start, int end);
        TileType mutateTileGene();
        TileType mutateTileGeneToWalkable();
        TileType mutateTileGeneToNonWalkable();

        MapState mate(MapState parent1, MapState parent2);
        std::vector<Tile> createGenome(int maxInitialMutation);
        MapState createIndividual(std::vector<Tile> genome);
    };
}
