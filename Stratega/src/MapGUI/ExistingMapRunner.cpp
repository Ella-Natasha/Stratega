#pragma once
#include <Stratega/MapGUI/ExistingMapRunner.h>
#include <Stratega/MapGUI/ExistingMapRenderer.h>

//#include <Stratega/Game/AgentThread.h>


namespace SGA
{
    ExistingMapRunner::ExistingMapRunner(const GameConfig& config, const bool& editConfigFileMap)
            : MapRunner(config, editConfigFileMap)
    {
    }

    void ExistingMapRunner::runGUI()
    {
        auto* mapRenderer = dynamic_cast<ExistingMapRenderer*>(renderer.get());
        while(!currentState->isGameOver)
        {
            mapRenderer->render();
        }
    }
}
