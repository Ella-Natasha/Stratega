#pragma once
#include <Stratega/MapGUI/MapRunner.h>

namespace SGA
{
    class ExistingMapRunner final : public MapRunner
    {
    public:
        explicit ExistingMapRunner(const GameConfig& config, const bool& editConfigFileMap);

    protected:
        void runGUI() override;
    };
}