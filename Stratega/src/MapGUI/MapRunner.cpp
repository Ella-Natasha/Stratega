#include <cassert>
#include <Stratega/MapGUI/MapRunner.h>
#include <Stratega/MapGUI/ExistingMapRunner.h>
#include <Stratega/MapGUI/NewMapRunner.h>
#include <Stratega/MapGUI/MapRenderer.h>

//#include <Stratega/Representation/GameState.h>
//#include <Stratega/ForwardModel/EntityForwardModel.h>
#include <Stratega/Configuration/GameConfig.h>

#include <Stratega/MapGUI/ExistingMapRenderer.h>


namespace SGA
{
    MapRunner::MapRunner(const GameConfig& config, const bool& editConfigFileMap)
            : currentState(),
              config(&config),
              editConfigFileMap(editConfigFileMap)
    {
        reset();
    }

    void MapRunner::reset()
    {
        currentState = config->generateGameState();
    }

    void MapRunner::render()
    {
        ensureRendererInitialized();
        renderer->render();
    }

    void MapRunner::play()
    {
        ensureRendererInitialized();
        runGUI();
    }

    void MapRunner::ensureRendererInitialized()
    {
        if (renderer == nullptr)
        {
//            renderer = createMapRenderer(editConfigFileMap);
            renderer = std::make_unique<ExistingMapRenderer>();
            renderer->init(*currentState, *config);
        }
    }

    std::unique_ptr<MapRunner> createMapRunner(const GameConfig& config, const bool& editConfigFileMap)
    {
//        if (editConfigFileMap)
//        {
            return std::make_unique<ExistingMapRunner>(config, editConfigFileMap);
//        }
//        else
//        {
//            return std::make_unique<NewMapRunner>(config, editConfigFileMap);
//        }
    }

}
