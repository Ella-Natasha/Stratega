#pragma once
#include <vector>
#include <unordered_map>
#include <Stratega/MapGUI/MapState.h>

namespace SGA
{
    struct TileType;
    struct EntityType;

    class SimpleGeneticAlgorithm
    {
    public:
        SimpleGeneticAlgorithm(std::shared_ptr<std::unordered_map<int, TileType>> tiles, std::shared_ptr<std::unordered_map<int, EntityType>> entities, MapState* initMap);
        std::vector<MapState> run(int populationSize, int generationSize);
        bool compareFitness(MapState &a, MapState &b);

    private:
        const MapState* initMap;
        const std::shared_ptr<std::unordered_map<int, TileType>> tileGenes;
        std::unordered_map<int, TileType> WalkableTileGenes;
        std::unordered_map<int, TileType> NonWalkableTileGenes;
        const std::shared_ptr<std::unordered_map<int, EntityType>> entityGenes;

        static int randomNum(int start, int end);
        TileType mutateTileGene();
        TileType mutateTileGeneToWalkable();
        TileType mutateTileGeneToNonWalkable();

        MapState mate(MapState parent1, MapState parent2);
        std::vector<Tile> createGenome();
        MapState createIndividual(std::vector<Tile> genome);
    };
}
