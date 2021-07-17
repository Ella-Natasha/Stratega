
#include <Stratega/Configuration/GameConfigParser.h>
#include <Stratega/MapGUI/MapRunner.h>
#include "../Arena/src/Arena.cpp"

int main(){

    std::string configPath("../../gameConfigs/TBS/CityCapturing.yaml");
    static bool editConfigFileMap = true;
    auto gameConfig = SGA::loadConfigFromYAML(configPath);

    // TODO change to only use ExistingMapRunner and ExistingMapRenderer
    // ignore NewMapRunner and NewMapRenderer they will not be used
    auto mapRunner = createMapRunner(*gameConfig, editConfigFileMap);
    mapRunner->play();

    return 0;
}
