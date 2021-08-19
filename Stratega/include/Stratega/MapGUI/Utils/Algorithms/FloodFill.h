#pragma once
#include <unordered_map>
#include <Stratega/MapGUI/MapState.h>

namespace SGA
{
    class FloodFill
    {
    public:
        std::unordered_map<std::string,std::vector<SGA::Vector2i>> floodFillMetricHelper(double startX, double startY);

    private:
        void floodFillMetricHelperUtil(double x, double y, std::vector<SGA::Vector2i>& visited, std::vector<SGA::Vector2i>& visitedEntities);
    };
}

