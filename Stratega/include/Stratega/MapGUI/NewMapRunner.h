#pragma once
#include <Stratega/MapGUI/MapRunner.h>

namespace SGA
{
    class NewMapRunner final : public MapRunner
    {
    public:
        explicit NewMapRunner(const GameConfig& config, const bool& editConfigFileMap);

    protected:
        void runGUI() override;
//        void runInternal(std::vector<std::unique_ptr<Agent>>& agents, GameObserver& observer) override;
//        void playInternal(std::vector<std::unique_ptr<Agent>>& agents, int humanIndex) override;
    };
}