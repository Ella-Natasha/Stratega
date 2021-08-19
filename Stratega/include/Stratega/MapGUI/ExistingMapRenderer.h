#pragma once
#include <Stratega/MapGUI/MapRenderer.h>
#include <Stratega/MapGUI/EditableTileMap.h>
#include <Stratega/MapGUI/MapState.h>
#include <Stratega/MapGUI/Utils/SGA/SimpleGeneticAlgorithm.h>
#include <Stratega/MapGUI/Utils/SGA/GenticAlgorithmParams.h>
#include <Stratega/MapGUI/Utils/MapElites/MapElites.h>

#include <Stratega/GUI/AssetCache.h>
#include <Stratega/MapGUI/MapEntityRenderer.h>
#include <Stratega/MapGUI/MapLockRenderer.h>
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
        bool isGameEndRequested() override;

    private:
        void handleInput();
        void renderLayers();

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
        void availableMaps();
        void createEditConfigWindow(int configTypeKey);
        void createRunResultsWindow();
        void createSaveBeforeCloseWindow();
        void showMapStats(MapState mapSate, float barWidth = 200.0f, float barHeight = 50.0f);
        void progressBars(std::map<std::string, int> stats, std::string title, float barWidth, float barHeight);
        void metricBreakdown(MapState mapSate, float barWidth, float barHeight);
        void yPadding();
        void heading(std::string title);
        void genMapButton(std::string& mapName, MapState& mapState);

        void createNewMap(std::string& newMapName, int& newMapWidth, int& newMapHeight);
        void saveTileMap(auto& selectionArray, bool& saveSelected, auto fileName);
        void runTileMap(int& seed, int& numberOfGames);
        void updateMiniMapTexture();
        void saveMiniMapTexture(std::unordered_map<std::string, MapState>& location, std::string& mapName);
        void updateGeneratedMiniMapTexture(std::map<std::string, MapState>& location, std::string& mapName);
        void showGeneratedMaps(std::map<std::string, MapState> mapStateArray, bool useMapElites);
        void runSimpleGeneticAlgorithm();
        void runMapElitesAlgorithm();
        void advanceMapElitesAlgorithm(MapState* chosenElite);
        void generateMapSuggestions();

        // initial setup
        std::unique_ptr<GameConfig> m_GameConfig;
        bool exitMapMaker = false;
        bool tileSelected = false;
        bool entitySelected = false;
        bool lockSelected = false;

        // map overlays
        bool renderOverlayView = false;
        OverlayRenderType overlayType = OverlayRenderType::Exploration;
        int explorationPlayerID = -1;

        MapState* renderedMap;
        TileType defaultTile;
        TileType m_CurrentTileSelection;
        EntityType m_CurrentEntitySelection;
        int entityOwnerID = -1;
        std::vector<std::string> mapNames;
        std::unordered_map<std::string, MapState> m_availableMaps;
        const GameConfig* config;
        GameState state;
        bool endGameRequested = false;

        std::string newMapName;
        int newMapWidth = 20;
        int newMapHeight = 20;
        bool gameRun = false;
        YAML::Node runOutputLog;
        bool editConfig = false;
        bool showMapActions = false;
        int configKey = 0;

        // Map generation
        GAParams mapGenerationParams;
        MapElites mapElites;

        // Generated maps popup stats
        bool popupOpen = false;
        std::pair<std::string, MapState> selectedMiniMap;

        // Rendering
        sf::RenderWindow window;
        sf::Clock deltaClock;
        AssetCache assetCache;
        AssetCache mapGenAssetCache;
        EditableTileMap tileMap;
        MapEntityRenderer entityRenderer;
        MapLockRenderer lockRenderer;
        sf::RenderTexture minimapTexture;

        // UI
        float zoomValue;
        bool dragging;
        sf::Vector2f oldMousePosition;
    };
}
