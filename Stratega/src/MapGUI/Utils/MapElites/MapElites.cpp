#pragma once
#include <Stratega/MapGUI/Utils/MapElites/MapElites.h>
#include <Stratega/MapGUI/Utils/MapElites/Map.h>

namespace SGA
{
    MapElites::MapElites(GAParams& generationParams):
            mapGenerationParams(generationParams),
            map(generationParams.mapElitesMapWidth,
                generationParams.mapElitesMapHeight,
                MapState()),
            geneticAlgorithm(generationParams)
    {
        // generate initial pop and assign to cells if individual is a feasible map
//        int i = 0;
//        while (i < 10)
//        {
            runGeneration(mapGenerationParams.renderedMap);
//            int range = (map.cellHits.size());
//            int random_int = (rand()%range);
//
//            int x = map.occupied[random_int].x;
//            int y = map.occupied[random_int].y;
//            chosenElite = map.get(x, y);
//            mapGenerationParams.renderedMap = &chosenElite;
//            if(map.occupied.size() == map.grid.size())
//                break;
//            i++;
//        }
    }

    MapElites::MapElites() :
            mapGenerationParams(),
            map(0,0,MapState()),
            geneticAlgorithm()
    {
    }

    void MapElites::runAlgorithm()
    {
        runGeneration(mapGenerationParams.renderedMap);
    }

    void MapElites::chooseRandomElite()
    {
        int range = (map.cellHits.size());
        int random_int = (rand()%range);

        int x = map.occupied[random_int].x;
        int y = map.occupied[random_int].y;
        chosenElite = &map.get(x, y);
    }

    // returns true if change needed
    bool MapElites::compareElites(int currentStateX, int currentStateY, MapState& potentialState)
    {
        // increment map cell visted value for use in analysis of the search space covered
        Vector2i coords = Vector2i(currentStateX, currentStateY);
        if (map.cellHits.contains(coords))
            map.cellHits.at(coords) += 1;
        else
        {
            map.cellHits.insert({coords, 1});
            map.occupied.push_back(coords);
        }

        // compare mapstate fitness vals
        auto& currentState = map.get(currentStateX, currentStateY);
        int currTotFitness = 0;
        currTotFitness += GAParams::getDimensionValue(mapGenerationParams.mapElitesXAxis, currentState);
        currTotFitness += GAParams::getDimensionValue(mapGenerationParams.mapElitesYAxis, currentState);

        int newTotFitness = 0;
        newTotFitness += GAParams::getDimensionValue(mapGenerationParams.mapElitesXAxis, potentialState);
        newTotFitness += GAParams::getDimensionValue(mapGenerationParams.mapElitesYAxis, potentialState);

        return newTotFitness > currTotFitness;
    }

    void MapElites::runGeneration(MapState* mapState)
    {
        auto newGenParams = mapGenerationParams;
        newGenParams.renderedMap = mapState;

        // push GA to all four corners of search space
        auto pop = geneticAlgorithm.runForMapElites(newGenParams, false, true);
        auto pop1 = geneticAlgorithm.runForMapElites(newGenParams, false, false);
        auto pop2 = geneticAlgorithm.runForMapElites(newGenParams, true, true);
        auto pop3 = geneticAlgorithm.runForMapElites(newGenParams, true, true);

        pop.insert( pop.end(), pop1.begin(), pop1.end() );
        pop.insert( pop.end(), pop2.begin(), pop2.end() );
        pop.insert( pop.end(), pop2.begin(), pop2.end() );

        for (auto& state : pop)
        {
            if (state.feasible)
            {
                auto fitnessX = GAParams::getDimensionValue(mapGenerationParams.mapElitesXAxis, state) / 100;
                auto fitnessY = GAParams::getDimensionValue(mapGenerationParams.mapElitesYAxis, state) / 100;

                int xCoord = map.getCellXPosition(fitnessX);
                int yCoord = map.getCellYPosition(fitnessY);

                if (compareElites(xCoord, yCoord, state))
                    map.setCell(xCoord, yCoord, state);

//                std::cout << "x: " << xCoord << ", y: " << yCoord << "\n";
//
//                for(auto& stat : state.fitnessMapStats)
//                    std::cout << stat.first << ": " << stat.second << "    ";
//
//                for(auto& cell : map.cellHits)
//                    std::cout << "(x: " << cell.first.x << ", y: " << cell.first.y << ")= " << cell.second ;
//
//                std::cout << "Size: " << map.occupied.size() << "\n=================================\n";
            }
        }
    }

}