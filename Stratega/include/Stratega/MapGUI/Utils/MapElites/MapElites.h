//#pragma once
//#include <utility>
//#include <vector>
//#include <Stratega/MapGUI/MapState.h>
//
///**
// * Contains the definition of the MAP-Elites as well as the methods required to
// * obtain the performance and features of a certain agent-heuristics
// * combination.
// *
// * For the MAP-Elites algorithm adaptation for automatic gameplay agents, the
// * performance and features information is retrieved from the stats of the game.
// * The specifics of the elements to consider for these will depend on the
// * configuration set when running the algorithm
// */
//
//namespace SGA
//{
//    class MapElites
//    {
//        explicit MapElites(Grid2D<Tile>&& board);
//        MapElites();
//
//    public:
//        bool ELITE_VERBOSE = true;
//        bool master = false;
//        bool fileBased = false;
//        Path path;
//    private:
//        class EliteIdx
//        {
//            std::vector<int> coordinates;
//
//            explicit EliteIdx(std::vector<int> coord) {
//                coordinates = std::move(coord);
////                System.arraycopy(coord, 0, coordinates, 0, coord.size());
//            }
//        };
//
//        std::vector<Feature> features;
//        std::vector<EliteIdx> occupiedCellsIdx;
//        Map mapElites;
//        MapRecord mapElitesRecord;
//        int nWeights;
//
//    public:
//    /**
//       * MAP elites algorithm - iterate nIterations times
//       * 1) get random elite from map
//       * 2) evolve weights taking random elite as base
//       * 3) create new elite (and get its stats to be able to get features and performance)
//       * 4) add elite to map (assign to cell and replace elite in assigned cell if the new performance is better)
//       * @param nTotalIterations number of iterations of the map elites algorithm
//       */
//        void runAlgorithm(int nTotalIterations, int nRandomInitialisations, Runner runner, std::string runStr);
//
//        /**
//         * Make all the data of the elites available: calculate all stats and set the data needed for serialisation.
//         * While the algorithm is running, not all gameStats data is being calculated so it is needed to got through
//         * all the occupied cells in the map to process all the data of the elite so it is available when their
//         * information is printed or serialised.
//         */
//        void processMapElitesData();
//
//    private:
//
//        /**
//         * Initialise map. Randomise nRandomInitialisations configurations (weights) and assign the resulting
//         * agents to the correspondent cell in the MAP
//         * @param nRandomInitialisations number of random initialisations requested
//         */
//        void initialiseMap(int nRandomInitialisations, Runner runner);
//
//        int getNCellsOccupied();
//
//        void createGameplayElite(std::vector<double> genome, Runner runner);
//
//        void refreshMapFromPath();
//
//        void addEliteToMap(Elite elite);
//
//        void printMapElitesInfo(std::string statsResultsFileName);
//    };
//}

#pragma once
#include <utility>
#include <vector>
#include <Stratega/MapGUI/Utils/SGA/GenticAlgorithmParams.h>
#include <Stratega/MapGUI/MapState.h>
#include <Stratega/MapGUI/Utils/MapElites/Map.h>
#include <Stratega/MapGUI/Utils/SGA/SimpleGeneticAlgorithm.h>

namespace SGA {

    class MapElites
    {
    public:
        explicit MapElites(GAParams& generationParams);
        MapElites();

        GAParams mapGenerationParams;
        Map2D<MapState> map;
        MapState* chosenElite;

        void runAlgorithm();
        void runGeneration(MapState* state);
        void chooseRandomElite();

    private:
        bool compareElites(int currentStateX, int currentStateY, MapState& potentialState);

        SimpleGeneticAlgorithm geneticAlgorithm;

    };
}
