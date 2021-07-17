#include <cassert>

#include <Stratega/MapGUI/MapRenderer.h>
#include <Stratega/MapGUI/NewMapRenderer.h>
#include <Stratega/MapGUI/ExistingMapRenderer.h>

namespace SGA
{
    std::unique_ptr<MapRenderer> createMapRenderer(const bool& editConfigFileMap)
    {
        if (editConfigFileMap)
        {
            return std::make_unique<ExistingMapRenderer>();
        }
        else
        {
            return std::make_unique<NewMapRenderer>();
        }

        assert(false);
        return nullptr;
    }

}