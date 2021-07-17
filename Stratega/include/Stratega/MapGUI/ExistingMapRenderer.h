#pragma once
#include <Stratega/MapGUI/MapRenderer.h>
#include <Stratega/MapGUI/EditableTileMap.h>
#include <Stratega/MapGUI/MapState.h>
#include <Stratega/MapGUI/Utils/SGA/SimpleGeneticAlgorithm.h>

#include <Stratega/GUI/AssetCache.h>
//#include <Stratega/GUI/EntityRenderer.h>
#include <Stratega/MapGUI/MapEntityRenderer.h>
#include <Stratega/GUI/Widgets/FogOfWarController.h>
#include <Stratega/GUI/Widgets/ActionsController.h>
#include <Stratega/Configuration/GameConfigParser.h>

#include <Stratega/ForwardModel/ActionAssignment.h>

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>

namespace SGA
{
    class ExistingMapRenderer : public MapRenderer
    {
    public:
        ExistingMapRenderer();

        void init(const GameState& initialState, const GameConfig& gameConfig) override;
        void update(const GameState& state) override;
        void render() override;
        void setPlayerPointOfView(int playerID) override;
        ActionAssignment getPlayerActions() override;

        bool isActionAvailable() const;
        bool isGameEndRequested() override;

    private:
        void updateFow();
        void handleInput();
        void renderLayers();

        void getDefaultTile();

        // Event Handling
        void mouseScrolled(const sf::Event& event);
        void mouseButtonReleased(const sf::Event& event);
        void mouseButtonPressed(const sf::Event& event);
        void mouseMoved(const sf::Event& event);

        // ImGUI
        void createHUD();
        void createWindowInfo();
        void createEditWindow();
        void createActionWindow();
        void createMainMapViewWindow();
        void createAvailableMapWindow();
        void createEditConfigWindow(int configTypeKey);
        void createRunResultsWindow();
        void createSaveBeforeCloseWindow();
        void showMapStats(MapState mapSate, float barWidth = 200.0f, float barHeight = 50.0f);

        void createNewMap(std::string& newMapName, int& newMapWidth, int& newMapHeight);
        void saveTileMap(auto& selectionArray, bool& saveSelected, auto fileName);
        void runTileMap(int& seed, int& numberOfGames);
        void updateMiniMapTexture();
        void saveMiniMapTexture(std::unordered_map<std::string, MapState>& location, std::string& mapName);

        std::unique_ptr<GameConfig> m_GameConfig;
        bool exitMapMaker = false;
        bool tileSelected = false;
        bool entitySelected = false;
        MapState* renderedMap;
        TileType defaultTile;
        TileType m_CurrentTileSelection;
        EntityType m_CurrentEntitySelection;
        int entityOwnerID;
        std::vector<std::string> mapNames;
        std::unordered_map<std::string, MapState> m_availableMaps;
        std::unordered_map<std::string, MapState> m_generatedMaps;
        std::string newMapName;
        int newMapWidth = 20;
        int newMapHeight = 20;
        bool gameRun = false;
        YAML::Node runOutputLog;
        bool editConfig = false;
        int configKey = 0;

        const GameConfig* config;
        GameState state;
        GameState fowState;
        std::optional<Action> selectedAction;
        bool endGameRequested = false;

        sf::RenderWindow window;
        sf::Clock deltaClock;
        int pointOfViewPlayerID;
        AssetCache assetCache;
        Widgets::FogOfWarSettings fowSettings;
        EditableTileMap tileMap;
        MapEntityRenderer entityRenderer;
//        MapTileMap minimap;// MiniMap
        sf::RenderTexture minimapTexture;
        sf::VertexArray gridLines;// MiniMap
        // Variables for input handling
        float zoomValue;
        bool dragging;
        sf::Vector2f oldMousePosition;
    };
}
