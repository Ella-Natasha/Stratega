#include <Stratega/MapGUI/Utils/SGA/SimpleGeneticAlgorithm.h>

#include <utility>

namespace SGA
{
    SimpleGeneticAlgorithm::SimpleGeneticAlgorithm(std::shared_ptr<std::unordered_map<int, TileType>> tiles, std::shared_ptr<std::unordered_map<int, EntityType>> entities, MapState* initMap):
            initMap(initMap),
            tileGenes(std::move(tiles)),
            entityGenes(std::move(entities))
    {
        tileGenes->erase(-1); //remove fog of war tile
        for(auto& tile : *tileGenes)
        {
            if (tile.second.isWalkable)
                this->WalkableTileGenes.insert({tile.first, tile.second});
            else
                this->NonWalkableTileGenes.insert({tile.first, tile.second});
        }
    }

    int SimpleGeneticAlgorithm::randomNum(int start, int end)
    {
        int range = (end-start)+1;
        int random_int = start+(rand()%range);
        return random_int;
    }

    TileType SimpleGeneticAlgorithm::mutateTileGene()
    {
        int len = tileGenes->size();
        int r = randomNum(0, len-1);
        return tileGenes->at(r);
    }

    TileType SimpleGeneticAlgorithm::mutateTileGeneToWalkable()
    {
        int len = WalkableTileGenes.size();
        int r = randomNum(0, len-1);
        return WalkableTileGenes.at(r);
    }

    TileType SimpleGeneticAlgorithm::mutateTileGeneToNonWalkable()
    {
        int len = NonWalkableTileGenes.size();
        int r = randomNum(0, len-1);
        return NonWalkableTileGenes.at(r);
    }

    MapState SimpleGeneticAlgorithm::mate(MapState parent1, MapState parent2)
    {
        std::vector<Tile> childGenome;

        for (size_t y = 0; y < parent1.board.getHeight(); y++)
        {
            for (size_t x = 0; x < parent1.board.getWidth(); x++)
            {
                // random probability
                float p = randomNum(0, 100)/100;

                // if prob is less than 0.45, insert gene
                // from parent 1
                if(p < 0.45)
                    childGenome.emplace_back(parent1.board[{(int)x, (int)y}]);

                    // if prob is between 0.45 and 0.90, insert
                    // gene from parent 2
                else if(p < 0.90)
                    childGenome.emplace_back(parent2.board[{(int)x, (int)y}]);

                    // otherwise insert random gene(mutate),
                    // for maintaining diversity
                else
                    childGenome.emplace_back(mutateTileGene().toTile((int)x, (int)y));
            }
        }

        return createIndividual(childGenome);
    }

    std::vector<Tile> SimpleGeneticAlgorithm::createGenome()
    {
        auto& board = initMap->board;
        std::vector<Tile> tiles;

        int mutationNum = randomNum(0, board.getWidth()-1);
        std::vector<Vector2i> coordForMutation;

        for (int i = 0; i < mutationNum; ++i) {
            int randX = randomNum(0, board.getWidth()-1);
            int randY = randomNum(0, board.getHeight()-1);

            Vector2i coord = Vector2i(randX, randY);
            coordForMutation.push_back(coord);
        }

        //Instance Tiles
        for (size_t y = 0; y < board.getHeight(); y++)
        {
            for (size_t x = 0; x < board.getWidth(); x++)
            {
                bool tilePlaced = false;

                for(auto& coord : coordForMutation)
                {
                    if (coord.x == (int)x || coord.y == (int)y)
                    {
                        tiles.emplace_back(mutateTileGene().toTile(x, y));
                        tilePlaced = true;
                        break;
                    }
                }
                if (!tilePlaced)
                    tiles.emplace_back(board[{(int)x, (int)y}]);
            }
        }

        return tiles;
    }

    MapState SimpleGeneticAlgorithm::createIndividual(std::vector<Tile> genome)
    {
        auto mapState = SGA::MapState(Grid2D<Tile>(initMap->board.getWidth(), genome.begin(), genome.end()));
        mapState.mapName = "Generated Map";

        //Instance Entities
        std::vector<Entity> newEntities;
        for (auto& entity : initMap->entities)
        {
            if (mapState.board[{static_cast<int>(entity.position.x), static_cast<int>(entity.position.y)}].isWalkable)
                newEntities.push_back(entity);
            else
            {
                // TODO put entity somewhere new ... how is yet TBC ... A* to closest walkable?
            }
        }

        mapState.entities = newEntities;
        mapState.calcMetrics();

        return mapState;
    }

    bool SimpleGeneticAlgorithm::compareFitness(MapState &a, MapState &b)
    {
        return a.asymmetricFitness < b.asymmetricFitness;
    }

    std::vector<MapState> SimpleGeneticAlgorithm::run(int populationSize, int generationSize)
    {
        srand((unsigned)(time(0)));

        // current generation
        int generation = 0;

        std::vector<MapState> population = {*initMap};

        // create initial population
        for(int i = 1; i < populationSize; i++)
        {
            auto genome = createGenome();
            auto individual = createIndividual(genome);
            population.push_back(individual);
        }

        while(generation < generationSize)
        {
            // sort the population in increasing order of fitness score
            std::sort(population.begin(), population.end(), [ ]( const auto& lhs, const auto& rhs )
            {
                return lhs.asymmetricFitness > rhs.asymmetricFitness;
            });

            // Otherwise generate new offsprings for new generation
            std::vector<MapState> new_generation;

            // Perform Elitism, that mean 10% of fittest population
            // goes to the next generation
            int s = (10*populationSize)/100;
            for(int i = 0;i<s;i++)
                new_generation.push_back(population[i]);

            // From 50% of fittest population, Individuals
            // will mate to produce offspring
            s = (90*populationSize)/100;
            for(int i = 0;i<s;i++)
            {
                int len = population.size();
                int r = randomNum(0, len-1);
                auto& parent1 = population[r];
                r = randomNum(0, len-1);
                auto& parent2 = population[r];
                auto offspring = mate(parent1, parent2);
                new_generation.push_back(offspring);
            }
            population = new_generation;
            generation++;
        }

        return population;

    }
}