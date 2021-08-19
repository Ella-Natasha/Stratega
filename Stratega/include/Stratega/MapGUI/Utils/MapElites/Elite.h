//#pragma once
//#include <vector>
//#include <Stratega/MapGUI/MapState.h>
//#include <Stratega/MapGUI/Utils/MapElites/Feature.h>
//
//namespace SGA
//{
//    class Elite{
//        explicit Elite(std::vector<double> weightList, std::vector<MapState> allStats);
//        Elite();
//
//    public:
//        std::vector<double> genome;
//        std::vector<MapState> allStats;
//        std::map<Feature, double> featureValues;
//
//    public:
//        void setFeatureValue(Feature feature);
//        int getFeatureIdx(Feature feature);
//        bool isBetterThan(Elite other);
//        std::string getPerformance();
//        void copyWeightsListValues(std::vector<double> weightsList);
//        std::string printWeights();
//        void printInfo(std::string statsResultsFileName);
//        void printInfoConsole();
//        void saveToFile(Path path);
//        std::vector<std::string> getBasicEliteInfo();
//
//    private:
//        std::string weightsString(std::string separator);
//        std::vector<std::string> featuresString(std::string separator);
//
//    };
//}
