#include <Stratega/MapGUI/ExistingMapRenderer.h>
#include <Stratega/GUI/GridUtils.h>
#include <Stratega/Configuration/GameConfig.h>

// Log arena run
#include <Stratega/Configuration/GameConfigParser.h>
#include <Stratega/Logging/Log.h>
#include "../../Arena/include/Arena.h"
#include "../../Arena/include/InputParser.h"
#include "yaml-cpp/yaml.h"

#include <SFML/Window.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <sstream>

namespace SGA
{
    ExistingMapRenderer::ExistingMapRenderer()
            : renderedMap(nullptr),
              config(nullptr),
              state(),
              window(sf::VideoMode().getDesktopMode(),
                     "Stratega Tile Map Editor",
                     sf::Style::Default | sf::Style::Titlebar),
              zoomValue(5.f),
              dragging(false)
    {
        // Initialize View
        sf::View view = window.getView();
        view.setCenter(
                (float)window.getSize().x / 2.,
                static_cast<float>(window.getSize().y) / 2.);
        view.setSize(window.getDefaultView().getSize()); // Reset the size
        view.zoom(zoomValue); // Apply the zoom level (this transforms the view)
        window.setView(view);
    }

    void ExistingMapRenderer::init(const GameState& initialState, const GameConfig& gameConfig)
    {
        config = &gameConfig;

        // Load textures: entities
        for (const auto& namePathPair : gameConfig.renderConfig->entitySpritePaths)
        {
            assetCache.loadTexture(namePathPair.first, namePathPair.second);
        }

        // Load textures:
        for (const auto& namePathPair : gameConfig.renderConfig->tileSpritePaths)
        {
            assetCache.loadTexture(namePathPair.first, namePathPair.second);
        }

        assetCache.loadTexture("selected", "../GUI/Assets/Tiles/selected.png");
        assetCache.loadTexture("locked", "../MapBuilder/Assets/lockedTile.png");
        assetCache.loadFont("font", "../GUI/Assets/arial.ttf");

        tileMap.init(initialState, gameConfig, *gameConfig.renderConfig);
        entityRenderer.init(gameConfig, *gameConfig.renderConfig);
        lockRenderer.init();


        ImGui::SFML::Init(window);

        update(initialState);

        int i = 1;
        for (auto& levelDef : config->levelDefinitions)
        {
            auto& board = levelDef.second.board;
            std::vector<Tile> tiles;

            //Instance Tiles
            for (size_t y = 0; y < board.getHeight(); y++)
            {
                for (size_t x = 0; x < board.getWidth(); x++)
                {
                    tiles.emplace_back(board.get(x,y)->toTile(x, y));
                }
            }

            auto mapState = SGA::MapState(Grid2D<Tile>(board.getWidth(), tiles.begin(), tiles.end()));

            std::string& mapName = mapState.mapName;
            mapName = "Original Map " + std::to_string(i);

            //Instance Entities
            for (auto& entity : levelDef.second.entityPlacements)
            {
                mapState.addEntity(*entity.entityType, entity.ownerID, entity.position);
            }

            m_availableMaps.insert({mapName, mapState});
            auto& newMap = m_availableMaps.at(mapName);
            mapNames.push_back(mapName);
            newMap.miniMap.init(initialState, gameConfig, *gameConfig.renderConfig);
            newMap.calcMetrics();
            i++;
        }
        renderedMap = &m_availableMaps.at("Original Map 1");
        renderedMap->calcMetrics();

        mapGenerationParams.tiles = *state.gameInfo->tileTypes;
        mapGenerationParams.entities = *state.gameInfo->entityTypes;
        mapGenerationParams.renderedMap = renderedMap;
        mapGenerationParams.generationSize = 0;
        mapGenerationParams.populationSize = 10;

        mapElites = MapElites(mapGenerationParams);
        generateMapSuggestions();
//        for (int j = 0; j < 0; ++j) {
//            mapGenerationParams.chooseRandomElite = true;
//            generateMapSuggestions();
//        }
    }

    void ExistingMapRenderer::update(const GameState& state)
    {
        this->state = state;
        state.gameInfo->tileTypes->erase(-1); // remove fog of war tile
    }

    void ExistingMapRenderer::handleInput()
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered())
                continue;

            switch (event.type)
            {
                case sf::Event::Closed: {exitMapMaker = true; break; }
                case sf::Event::MouseWheelScrolled: { mouseScrolled(event); break; }
                case sf::Event::MouseButtonReleased: { mouseButtonReleased(event); break; }
                case sf::Event::MouseButtonPressed: { mouseButtonPressed(event); break; }
                case sf::Event::MouseMoved: { mouseMoved(event); 	break; }
                    //case sf::Event::KeyPressed: {keyPressed(event); break;	}
                default:  break;
            }
        }
    }

    void ExistingMapRenderer::render()
    {
        handleInput();
        window.clear();
        renderLayers();
        ImGui::SFML::Update(window, deltaClock.restart());
        createHUD();
        ImGui::SFML::Render(window);
        window.display();
    }

    void ExistingMapRenderer::renderLayers()
    {
        tileMap.update(renderedMap, renderOverlayView, overlayType, explorationPlayerID);
        window.draw(tileMap);

        for (auto& map : m_availableMaps)
        {
            saveMiniMapTexture(m_availableMaps, const_cast<std::string &>(map.first));
        }

        //Add selected tileactionSettings.waitingForPosition
        auto currentMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Vector2i mouseGridPos = toGrid(currentMousePos);

        if (tileSelected)
        {
            sf::Texture& texture = assetCache.getTexture(m_CurrentTileSelection.name);
            sf::Vector2f origin(TILE_ORIGIN_X, TILE_ORIGIN_Y);
            sf::Sprite selectedTile(texture);

            selectedTile.setPosition(toISO(mouseGridPos.x, mouseGridPos.y));
            selectedTile.setOrigin(origin);
            window.draw(selectedTile);
        }

        if (lockSelected)
        {
            sf::Texture& texture = assetCache.getTexture("locked");
            sf::Vector2f origin(TILE_ORIGIN_X, TILE_ORIGIN_Y);
            sf::Sprite selectedTile(texture);

            selectedTile.setPosition(toISO(mouseGridPos.x, mouseGridPos.y));
            selectedTile.setOrigin(origin);
            window.draw(selectedTile);
        }

        if (entitySelected)
        {
            sf::Texture& texture = assetCache.getTexture(m_CurrentEntitySelection.name);
            sf::Vector2f origin(TILE_ORIGIN_X, TILE_ORIGIN_Y);
            sf::Sprite selectedTile(texture);

            // minus numbers = bad hack to get position on lower tile render level
            // FIX = resize entity tiles from 512X512px to 256X256px (not implemented)
            selectedTile.setPosition(toISO(mouseGridPos.x-2, mouseGridPos.y-1));
            selectedTile.setOrigin(origin);
            window.draw(selectedTile);
        }

        if (renderedMap->board.isInBounds(Vector2i(mouseGridPos.x, mouseGridPos.y)))
        {
            sf::Texture& texture = assetCache.getTexture("selected");
            sf::Vector2f origin(TILE_ORIGIN_X, TILE_ORIGIN_Y);
            sf::Sprite selectedTile(texture);

            selectedTile.setPosition(toISO(mouseGridPos.x, mouseGridPos.y));
            selectedTile.setOrigin(origin);
            window.draw(selectedTile);
        }

        //Draw entities
        entityRenderer.update(renderedMap->entities);
        window.draw(entityRenderer);

        //Draw tile locks
        lockRenderer.update(renderedMap->lockedTiles);
        window.draw(lockRenderer);

    }

    void ExistingMapRenderer::mouseScrolled(const sf::Event& event)
    {
        // Determine the scroll direction and adjust the zoom level
        if (event.mouseWheelScroll.delta <= -1)
            zoomValue = std::min(10.f, zoomValue + .1f);
        else if (event.mouseWheelScroll.delta >= 1)
            zoomValue = std::max(.5f, zoomValue - .1f);

        // Update our view
        sf::View view = window.getView();
        view.setSize(window.getDefaultView().getSize()); // Reset the size
        view.zoom(zoomValue); // Apply the zoom level (this transforms the view)
        window.setView(view);
    }
    void ExistingMapRenderer::mouseButtonReleased(const sf::Event& event)
    {
        // Mouse button is released, no longer move
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            dragging = false;
        }
    }
    void ExistingMapRenderer::mouseButtonPressed(const sf::Event& event)
    {
        // Mouse button is pressed, get the position and set moving as active
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            auto point = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
            sf::Vector2i pos = toGrid(window.mapPixelToCoords(point));

            int posX = (int)pos.x;
            int posY = (int)pos.y;
            bool onBoard = renderedMap->board.isInBounds(posX, posY);

            // If tile selected add to board
            if (tileSelected && onBoard)
            {
                renderedMap->board[{pos.x, pos.y}] = m_CurrentTileSelection.toTile(pos.x, pos.y);
                tileMap.update(renderedMap, renderOverlayView, overlayType, explorationPlayerID);
                updateMiniMapTexture();
                renderedMap->calcMetrics();
//                runSimpleGeneticAlgorithm();
                generateMapSuggestions();
            }

            if (tileSelected && !onBoard)
                tileSelected = false;

            // If entity selected add to board
            if (entitySelected && onBoard)
            {
                renderedMap->addEntity(
                        m_CurrentEntitySelection,
                        entityOwnerID,
                        Vector2f(pos.x, pos.y));
                tileMap.update(renderedMap, renderOverlayView, overlayType, explorationPlayerID);
                entityRenderer.update(renderedMap->entities);
                updateMiniMapTexture();
                renderedMap->calcMetrics();
                generateMapSuggestions();
//                runSimpleGeneticAlgorithm();
            }

            if (entitySelected && !onBoard)
                entitySelected = false;

            // If tilelock selected add to board
            if (lockSelected && onBoard)
                renderedMap->addTileLock(Vector2f(pos.x, pos.y));

            if (lockSelected && !onBoard)
                lockSelected = false;

            dragging = true;
            oldMousePosition = window.mapPixelToCoords(point);
        }
        if (event.mouseButton.button == sf::Mouse::Right)
        {
            auto point = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
            sf::Vector2i pos = toGrid(window.mapPixelToCoords(point));
            tileSelected = false;
            entitySelected = false;
            lockSelected = false;

            if (renderedMap->board.isInBounds((int)pos.x,(int)pos.y))
            {
                auto vec = Vector2f(pos.x, pos.y);

                if (renderedMap->tileLockAt(vec))
                {
                    renderedMap->removeTileLockAt(vec);
                }
                else if (renderedMap->getEntityAt(vec))
                {
                    renderedMap->removeEntityAt(vec);
                    renderedMap->calcMetrics();
                    updateMiniMapTexture();
                    generateMapSuggestions();
//                    runSimpleGeneticAlgorithm();
                }
            }
        }
    }

    void ExistingMapRenderer::mouseMoved(const sf::Event& event)
    {
        if (!dragging)
            return;

        // Determine how the cursor has moved
        auto point = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
        auto newPos = window.mapPixelToCoords(point);
        auto deltaPos = oldMousePosition - newPos;

        // Move our view accordingly and update the window
        sf::View view = window.getView();
        view.setCenter(view.getCenter() + deltaPos);
        window.setView(view);

        // Save the new position as the old one
        // We're recalculating this, since we've changed the view
        oldMousePosition = window.mapPixelToCoords(point);
    }

    void ExistingMapRenderer::createHUD()
    {
        createWindowInfo();
        createEditWindow();
        createAvailableMapWindow();
        createMainMapViewWindow();

        if (showMapActions)
            createActionWindow();
        if(gameRun)
            createRunResultsWindow();
        if(editConfig)
            createEditConfigWindow(configKey);
        if(exitMapMaker)
            createSaveBeforeCloseWindow();
    }

    void ExistingMapRenderer::createWindowInfo()
    {
        ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

        std::filesystem::path filePath = config->yamlPath;

        ImGui::Begin("Map Information");

        ImGui::Separator();
        ImGui::Text("Game Config");
        ImGui::Separator();

        ImGui::BeginGroup();
        ImGui::Indent();
        yPadding();
        std::string text = "Game Type: " + filePath.parent_path().stem().string();
        ImGui::Text("%s", text.c_str());
        text = "Game Name: " + filePath.stem().string();
        ImGui::Text("%s", text.c_str());

        int i = 0;
        for (auto& agent : config->agentParams)
        {
            text = "Agent " + std::to_string(i) + ": " + agent.first;
            ImGui::Text("%s", text.c_str());
            i++;
        }
        yPadding();
        ImGui::EndGroup();
        ImGui::SameLine(250.0f, ImGui::GetStyle().ItemInnerSpacing.x);

        ImGui::BeginGroup();

        yPadding();
        if (ImGui::Button("Edit Game Config", ImVec2(150, 30)))
        {
            configKey = 0;
            editConfig = true;
        }
        ImGui::Unindent();
        ImGui::EndGroup();

        auto& name = renderedMap->mapName;
        heading(name);
        ImGui::Indent();
        showMapStats(m_availableMaps.at(name));
        ImGui::Unindent();
        ImGui::End();
    }

    void ExistingMapRenderer::createEditWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Edit Tile Map");

        ImGui::BeginChild("Scrolling");

        if (ImGui::Button("Clear All Tiles", ImVec2(300, 30)))
        {

            auto& board = renderedMap->board;

            //Instance Tiles
            for (size_t y = 0; y < board.getHeight(); y++)
            {
                for (size_t x = 0; x < board.getWidth(); x++)
                {
                    board[{int(x), int(y)}] = config->tileTypes.at(2).toTile(x, y);
                }
            }
            tileMap.update(renderedMap, renderOverlayView, overlayType, explorationPlayerID);
            updateMiniMapTexture();
            generateMapSuggestions();
//            runSimpleGeneticAlgorithm();
        }

        if (ImGui::Button("Clear All Entities", ImVec2(300, 30)))
        {
            renderedMap->clearAllEntities();
            entityRenderer.update(renderedMap->entities);
            updateMiniMapTexture();
        }

        heading("Available Tiles: ");

        if (ImGui::Button("Lock Tile", ImVec2(300, 30)))
        {
            lockSelected = true;
        }
        yPadding();

        for (auto& entity : *state.gameInfo->tileTypes)
        {
            std::string walk = (entity.second.isWalkable) ? "Yes": "No";
            std::string block = (entity.second.blocksSight) ? "Yes": "No";
            std::string def = (entity.second.isDefaultTile) ? "Yes": "No";

            //Add units
            sf::Texture& texture = assetCache.getTexture(entity.second.name);

            if (ImGui::ImageButton(texture, ImVec2(100, 100)))
            {
                entitySelected = false;
                tileSelected = true;
                m_CurrentTileSelection = entity.second;
            }
            ImGui::SameLine();
            ImGui::BeginGroup();
            std::string name = "Tile Name: " + entity.second.name;
            ImGui::Text("%s", name.c_str());
            std::string walkable = "Tile Walkable: " + walk;
            ImGui::Text("%s", walkable.c_str());
            std::string blocks = "Tile Blocks Sight: " + block;
            ImGui::Text("%s", blocks.c_str());
            std::string text = "Default Tile: " + def;
            ImGui::Text("%s", text.c_str());
            ImGui::EndGroup();
        }

        heading("Available Entities: ");
        if (ImGui::Button("Edit Entity Parameters", ImVec2(300, 30)))
        {
            configKey = 1;
            editConfig = true;
        }

        yPadding();

        for (auto& entity : *state.gameInfo->entityTypes)
        {
            //Add units
            sf::Texture& texture = assetCache.getTexture(entity.second.name);

            if (ImGui::ImageButton(texture, ImVec2(100, 100)))
            {
                tileSelected = false;
                m_CurrentEntitySelection = entity.second;
                ImGui::OpenPopup("Select Entity Owner");
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            std::string name = "Entity Name: " + entity.second.name;
            ImGui::Text("%s", name.c_str());
            std::string sight = "Line of Sight: " + std::to_string(int(entity.second.lineOfSight));
            ImGui::Text("%s", sight.c_str());

            for (auto& param : entity.second.parameters)
            {
                std::string paramStr = param.second.name + ": " + std::to_string(int(param.second.defaultValue));
                ImGui::Text("%s", paramStr.c_str());
            }

            if(!entity.second.cost.empty()){ImGui::Text("Cost:");}

            for (auto& cost : entity.second.cost)
            {
                for(auto& parameter : config->parameters)
                {
                    if(cost.first == parameter.second)
                    {
                        std::string paramStr = "  " +std::to_string(int(cost.second)) + " " + parameter.first;
                        ImGui::Text("%s", paramStr.c_str());
                        break;
                    }
                }
            }

            ImGui::EndGroup();
        }
        if (ImGui::BeginPopupModal("Select Entity Owner", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            int i = 0;
            for (auto& agent : config->agentParams)
            {
                std::string buttonName = "Agent " + std::to_string(i) + ": " + agent.first;
                if (ImGui::Button(buttonName.c_str()))
                {
                    entitySelected = true;
                    entityOwnerID = i;
                    ImGui::CloseCurrentPopup();
                }
                i++;
                ImGui::SameLine();
            }

            if (ImGui::Button("Neutral"))
            {
                entitySelected = true;
                entityOwnerID = -1;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::End();

    }

    void ExistingMapRenderer::createActionWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Map Actions");

        ImGui::TextWrapped("All available maps will be saved/run, if you only wish to save/run a few "
                    "please select the maps you would like to save/run below");
        yPadding();
        ImGui::Separator();

        static bool saveMultiple = false;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Select Maps to Save or Run", &saveMultiple);
        yPadding();
        ImGui::PopStyleVar();
        static bool selection[8] = { false, false, false, false, false, false, false, false };

        if (saveMultiple)
        {
            int n = 0;
            for (auto& map: mapNames)
            {
                char buf[32];
                sprintf(buf, "Name: %s", map.c_str());
                if (ImGui::Selectable(buf, selection[n]))
                {
                    if (!ImGui::GetIO().KeyCtrl)    // Clear selection when CTRL is not held
                        memset(selection, 0, sizeof(selection));
                    selection[n] ^= 1;
                }
                n++;
            }
        }

        yPadding();

        if (ImGui::Button("Save TileMaps", ImVec2(200, 30)))
        {
            ImGui::OpenPopup("Save");
        }
        if (ImGui::BeginPopupModal("Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("Please provide a name for the map YAML file. "
                        "The name given will be appended with the name of the game.");
            yPadding();

            static char fileName[50];
            ImGui::InputText("File Name", fileName, 50);
            yPadding();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                saveTileMap(selection, saveMultiple, fileName);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Run Game with TileMaps", ImVec2(200, 30)))
        {
            ImGui::OpenPopup("Run");
        }
        if (ImGui::BeginPopupModal("Run", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("The results of any runs are provided on screen and in the YAML output logs.");
            static int seedValue = 0;
            static int gameNumValue = 1;
            ImGui::PushItemWidth(80.0f);
            ImGui::InputInt("Seed", &seedValue);
            ImGui::InputInt("Number of Games", &gameNumValue);ImGui::PopItemWidth();
            yPadding();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                saveTileMap(selection, saveMultiple, "run");
                runTileMap(seedValue, gameNumValue);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(200, 30)))
        {
            showMapActions = false;
        }

        ImGui::End();
    }

    void ExistingMapRenderer::createMainMapViewWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Change Map View");

        ImGui::BeginChild("Scrolling");

        static int e = 0;
        ImGui::RadioButton("Default", &e, 0); ImGui::SameLine();
        if (renderedMap->feasible)
        {
            ImGui::RadioButton("Exploration", &e, 1); ImGui::SameLine();
            ImGui::RadioButton("Safe Areas", &e, 3); ImGui::SameLine();
//            ImGui::RadioButton("Unused Space", &e, 4); ImGui::SameLine();
        }
//        ImGui::RadioButton("Segments", &e, 5);

        if (e != 0)
        {
            renderOverlayView = true;
            if ( e == 1)
            {
                heading("Exploration for:");
                overlayType = OverlayRenderType::Exploration;
                for (auto& player : renderedMap->playerExploration)
                {
                    char buf[32];
                    sprintf(buf, "Agent: %d", player.first);
                    ImGui::RadioButton(buf, &explorationPlayerID, player.first); ImGui::SameLine();
                }
            }
            if ( e == 3)
                overlayType = OverlayRenderType::SafeAreas;
        }
        else
        {
            renderOverlayView = false;
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void ExistingMapRenderer::genMapButton(std::string& mapName, MapState& mapState)
    {
        sf::Texture &mapTex = assetCache.getTexture("minimap " + mapName);

//        if (mapState.isElite)
//        {
//            int i = 0;
//            ImGui::PushID(i);
//            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(i/7.0f, 0.6f, 0.8f));
//            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i/7.0f, 0.7f, 0.95f));
//            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i/7.0f, 0.8f, 0.95f));
//            ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(i/7.0f, 0.5f, 0.95f));
//            if (ImGui::ImageButton(mapTex, ImVec2(
//                    static_cast<float>((mapState.board.getWidth() * 32)) / 2,
//                    static_cast<float>((mapState.board.getHeight() * 32)) / 2)))
//            {
//                ImGui::OpenPopup("Generated Map Actions");
//            }
//            ImGui::PopStyleColor(4);
//            ImGui::PopID();
//        }
//        else
//        {
            if (ImGui::ImageButton(mapTex, ImVec2(
                    static_cast<float>((mapState.board.getWidth() * 32)) / 2,
                    static_cast<float>((mapState.board.getHeight() * 32)) / 2)))
            {
                ImGui::OpenPopup("Generated Map Actions");
            }
//        }
    }

    void ExistingMapRenderer::availableMaps()
    {
        yPadding();
        if (!ImGui::CollapsingHeader("Current Available Maps"))
            return;

        ImGui::PushID("Current Available Maps");
        ImGui::Indent();
        for (auto& map : m_availableMaps) {
            if(map.first != renderedMap->mapName)
            {
                auto& mapKey = map.first;
                auto& mapState = map.second;
                sf::Texture &mapTex = assetCache.getTexture("minimap " + mapKey);

                heading("Map Name: " + mapKey);
                if (ImGui::ImageButton(mapTex, ImVec2(
                        static_cast<float>((mapState.board.getWidth() * 32)) / 2,
                        static_cast<float>((mapState.board.getHeight() * 32)) / 2)))
                {
                    renderedMap = &m_availableMaps.at(mapKey);
                    tileMap.update(renderedMap, renderOverlayView, overlayType, explorationPlayerID);
                    entityRenderer.update(renderedMap->entities);
//                    runSimpleGeneticAlgorithm();
                    generateMapSuggestions();
                }
                ImGui::SameLine();
                ImGui::BeginGroup();
                showMapStats(mapState, 150.0f, 20.0f);
                ImGui::EndGroup();
            }
        }
        ImGui::Unindent();
        ImGui::PopID();
    }

    void ExistingMapRenderer::createAvailableMapWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Available Maps");

        ImGui::BeginChild("Scrolling");
        availableMaps();
        yPadding();

        if (ImGui::Button("Run Games with or Save Maps", ImVec2(300, 30)))
            showMapActions = true;

        ImGui::SameLine();

        if (ImGui::Button("Create New Map", ImVec2(300, 30)))
            ImGui::OpenPopup("New Map Setup");

        if (ImGui::BeginPopupModal("New Map Setup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PopStyleVar();

            static char infoName[50];
            ImGui::InputText("Tile Map Name", infoName, 50);
            ImGui::InputInt("Map Width in Tiles", &newMapWidth);
            ImGui::InputInt("Map Height in Tiles", &newMapHeight);

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                if (newMapWidth >= 1 && newMapHeight >= 1 && infoName[0] != '\0')
                {
                    newMapName = infoName;
                    createNewMap(newMapName, newMapWidth, newMapHeight);
                    renderedMap = &m_availableMaps.at(newMapName);
                    renderedMap->calcMetrics();
                    entityRenderer.update(renderedMap->entities);
                    tileMap.update(renderedMap, renderOverlayView, overlayType, explorationPlayerID);
//                    runSimpleGeneticAlgorithm();
                    generateMapSuggestions();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Indent();
        heading("AI Generated Map Suggestions");
        ImGui::RadioButton("AI Suggestions On", &mapGenerationParams.aiEnabled, 1); ImGui::SameLine();
        ImGui::RadioButton("AI Suggestions Off", &mapGenerationParams.aiEnabled, 0);
        ImGui::Unindent();

        if(mapGenerationParams.aiEnabled) {
            ImGui::Indent();
            yPadding();
            ImGui::TextWrapped("The current map suggestions are produced using default algorithm parameters,"
                               " to expose and change these parameters please select 'Use Custom Parameters'. "
                               "Higher maximum mutation and generation size will produce busier maps, while"
                               " population size will determine how may suggestions are produced. "
                               "Maps can be generated with or without entities.");
            yPadding();

            static int useDefault = 0;
            ImGui::RadioButton("Use Default Parameters", &useDefault, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Use Custom Parameters", &useDefault, 1);

            if (useDefault) {
                yPadding();
                ImGui::Separator();
                yPadding();
                ImGui::RadioButton("Use MAP-Elites", &mapGenerationParams.useMapElites, 1);
                ImGui::SameLine();
                ImGui::RadioButton("No MAP-Elites", &mapGenerationParams.useMapElites, 0);
                yPadding();
                if (mapGenerationParams.useMapElites) {
                    ImGui::Text("MAP-Elites x-axis");
                    const char *items[] = {"Symmetry", "Player Balance", "Exploration", "Safe Areas",
                                           "Resource Safety"};
                    static int item_current_idx = 0;                    // Here our selection data is an index.
                    const char *combo_label = items[item_current_idx];  // Label to preview before opening the combo (technically could be anything)(
                    if (ImGui::BeginCombo("X Dimension", combo_label)) {
                        for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                            const bool is_selected = (item_current_idx == n);
                            if (ImGui::Selectable(items[n], is_selected))
                            {
                                item_current_idx = n;
                                mapGenerationParams.dimensionChanged = true;
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    mapGenerationParams.mapElitesXAxis = MapElitesDimensions(item_current_idx);
                    ImGui::InputInt("Number of Cells on x-axis", &mapGenerationParams.mapElitesMapWidth);
                    yPadding();
                    ImGui::Text("MAP-Elites y-axis");
                    const char *y_items[] = {"Symmetry", "Player Balance", "Exploration", "Safe Areas",
                                             "Resource Safety"};
                    static int y_item_current_idx = 1;                    // Here our selection data is an index.
                    const char *y_combo_label = y_items[y_item_current_idx];  // Label to preview before opening the combo (technically could be anything)(
                    if (ImGui::BeginCombo("Y Dimension", y_combo_label)) {
                        for (int n = 0; n < IM_ARRAYSIZE(y_items); n++) {
                            const bool y_is_selected = (y_item_current_idx == n);
                            if (ImGui::Selectable(y_items[n], y_is_selected))
                            {
                                y_item_current_idx = n;
                                mapGenerationParams.dimensionChanged = true;
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (y_is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    mapGenerationParams.mapElitesYAxis = MapElitesDimensions(y_item_current_idx);
                    ImGui::InputInt("Number of Cells on y-axis", &mapGenerationParams.mapElitesMapHeight);
                    yPadding();
                }
                ImGui::Text("Genetic Algorithm Parameters");
                ImGui::InputInt("Generation Size: Default value: 10", &mapGenerationParams.generationSize);
                ImGui::InputInt("Population Size: Default value: 25", &mapGenerationParams.populationSize);
                ImGui::InputInt("Max Mutation Size: Default value: 5", &mapGenerationParams.maxInitialMutation);

                //check maximum initial mutation numbers
                int maxSize = static_cast<int>((renderedMap->board.grid.size()));
                if (mapGenerationParams.maxInitialMutation > maxSize)
                    mapGenerationParams.maxInitialMutation = maxSize;

                if (mapGenerationParams.maxInitialMutation < 1)
                    mapGenerationParams.maxInitialMutation = 1;

                yPadding();
                if (!mapGenerationParams.useMapElites) {
                    ImGui::RadioButton("Include Entities", &mapGenerationParams.generateEntities, 1);
                    ImGui::SameLine();
                    ImGui::RadioButton("Exclude Entities", &mapGenerationParams.generateEntities, 0);
                }
                yPadding();
                ImGui::Separator();
                yPadding();
            }

            ImGui::TextWrapped("The following map suggestions show alternatives "
                               "to the current map along with metrics compared to the current map. "
                               "These maps can be previewed, saved as new map or applied to the "
                               "current open map.");
            yPadding();

            std::string label = "Re-run Generation";
            if (mapGenerationParams.useMapElites) {
                label = "Run Generation with Random Elite";
                if (mapGenerationParams.dimensionChanged)
                {
                    if (ImGui::Button("Apply Dimension Change", ImVec2(200, 30)))
                    {
                        mapGenerationParams.dimensionChanged = false;
                        mapElites = MapElites(mapGenerationParams);
                        generateMapSuggestions();
                    }ImGui::SameLine();
                    ImGui::TextWrapped("WARNING: Applying a dimension change will remove all current elites");
                }

            } yPadding();
            if (ImGui::Button(label.c_str(), ImVec2(400, 30)))
            {
                if (mapGenerationParams.useMapElites)
                    mapGenerationParams.chooseRandomElite = true;
                generateMapSuggestions();
            }
            yPadding();
            ImGui::Separator();

            if (mapGenerationParams.useMapElites) {
                yPadding();
                ImGui::InputInt("Zoom Level", &mapGenerationParams.zoomLevel);
                if (mapGenerationParams.zoomLevel > 10)
                    mapGenerationParams.zoomLevel = 10;
                if (mapGenerationParams.zoomLevel < 2)
                    mapGenerationParams.zoomLevel = 2;
                yPadding();
                ImVec2 scrolling_child_size = ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 35 + 30);
                ImGui::BeginChild("scrolling", scrolling_child_size, false, ImGuiWindowFlags_HorizontalScrollbar);
                showGeneratedMaps(mapGenerationParams.mapElitesGenMaps, true);
                ImGui::EndChild();
            } else
                showGeneratedMaps(mapGenerationParams.generatedMaps, false);
            ImGui::Unindent();
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void ExistingMapRenderer::createEditConfigWindow(int configTypeKey)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Edit Config File");

        ImGui::BeginChild("Scrolling");

        ImGui::TextWrapped("Any edits made to the following details will "
                    "change the current game configuration. "
                    "As a result any changes applied will be saved into a new YAML file.");
        yPadding();

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            //TODO add save config func
            editConfig = false;
        } ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            editConfig = false;
        }
        yPadding();
        ImGui::Separator();
        yPadding();

        static int e = configTypeKey;
        ImGui::RadioButton("Agent Config", &e, 0); ImGui::SameLine();
        ImGui::RadioButton("Entity Config", &e, 1); ImGui::SameLine();
        ImGui::RadioButton("Tile Config", &e, 2);
        yPadding();
        ImGui::Separator();

        if(e == 0)
        {
            yPadding();
            ImGui::TextWrapped("Please Select all agents you wish to use from "
                        "the list below (ctrl click to select multiple)");
            yPadding();
            static const char* agents[] = {
                    "Random Agent",
                    "OSLA Agent",
                    "Do Nothing Agent",
                    "MCTS Agent",
                    "DFS agent",
                    "BFS Agent",
                    "Beam Search Agent",
                    "RHEA Agent"
            };

            static bool selection[8] = { false, false, false, false, false, false, false, false };

            int n = 0;
            for (auto& agent: agents)
            {
                char buf[32];
                sprintf(buf, "Name: %s", agent);
                if (ImGui::Selectable(buf, selection[n]))
                {
                    if (!ImGui::GetIO().KeyCtrl)    // Clear selection when CTRL is not held
                        memset(selection, 0, sizeof(selection));
                    selection[n] ^= 1;
                }
                n++;
            }

        }
        if(e == 1)
        {
            for (auto& entity : *state.gameInfo->entityTypes)
            {
                auto& entityType = entity.second;

                //Add units
                yPadding();
                std::string name = "Entity Name: " + entityType.name;
                sf::Texture& texture = assetCache.getTexture(entityType.name);
                ImGui::Image(texture, ImVec2(150, 150));

                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("%s", name.c_str());
                std::string sight = "Current Value: " + std::to_string(int(entityType.lineOfSight));
                int newValue = int(entityType.lineOfSight);
                ImGui::PushItemWidth(80.0f);
                ImGui::InputInt("Line of Sight:", &newValue);
                ImGui::SameLine();
                ImGui::PopItemWidth();

                ImGui::Text("%s", sight.c_str());


                for (auto& param : entityType.parameters)
                {
                    auto& parameter = param.second;
                    std::string curParamStr = "Current Value: " + std::to_string(int(parameter.defaultValue));
                    std::string newParamStr = parameter.name + ": ";
                    int newParamValue = int(parameter.defaultValue);
                    ImGui::PushItemWidth(80.0f);
                    ImGui::InputInt(newParamStr.c_str(), &newParamValue);
                    ImGui::SameLine();
                    ImGui::PopItemWidth();
                    ImGui::Text("%s", curParamStr.c_str());
                }

                for (auto& cost : entity.second.cost)
                {
                    for(auto& parameter : config->parameters)
                    {
                        if(cost.first == parameter.second)
                        {
                            std::string paramStr = "Current Value: " + std::to_string(int(cost.second)) + " " + parameter.first;
                            std::string newParamStr = "Cost in " + parameter.first + ": ";
                            int newCostValue = int(cost.second);
                            ImGui::PushItemWidth(80.0f);
                            ImGui::InputInt(newParamStr.c_str(), &newCostValue);ImGui::SameLine();
                            ImGui::PopItemWidth();
                            ImGui::Text("%s", paramStr.c_str());
                            break;
                        }
                    }
                }

                ImGui::EndGroup();
            }
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void ExistingMapRenderer::createRunResultsWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Game Run Results");

        ImGui::BeginChild("Scrolling");

        std::map<std::string, std::string> logs;
        std::string gameRunName;
        int i = 1;
        int nameKey = 0;

        std::map<std::string, std::vector<YAML::Node>> mapNodes;
        std::vector<std::string> players;

        for (YAML::detail::iterator_value logNode : runOutputLog) {
            auto gameName = logNode.first.as<std::string>();

//            if (gameRunName != gameName) {
//                i = 1;
//                gameRunName = gameName;
//                auto mapName = "Map " + logNode.second["Map"].as<std::string>() + ": " + mapNames[nameKey];
//                ImGui::Separator();
//                ImGui::Text("%s", mapName.c_str());
//                ImGui::Separator();
//                nameKey++;
//            }
//
//            yPadding();
//            std::string gameNum = "Game " + std::to_string(i) + ":";
//            ImGui::Text("%s", gameNum.c_str());
//            i++;

            for (const auto &nameValPair : logNode.second) {
                auto param = nameValPair.first.as<std::string>();
                if (param.find("Map") != std::string::npos) {
                    std::string fullParam = param + ": " + nameValPair.second.as<std::string>();
                    if (mapNodes.contains(fullParam))
                        mapNodes.at(fullParam).push_back(logNode.second);
                    else
                        mapNodes.insert({fullParam, std::vector({logNode.second})});
                }
            }
        }
        for (auto& mapNode : mapNodes)
            {
                auto mapName = mapNode.first;
                heading(mapName.c_str());
                ImGui::Indent();

                auto& mapKey = mapNames[nameKey];
                auto& mapState = m_availableMaps.at(mapNames[nameKey]);
                ImGui::BeginGroup();
                sf::Texture &selectedMap = assetCache.getTexture("minimap " + mapKey);
                ImGui::Image(selectedMap, ImVec2(
                        static_cast<float>((mapState.board.getWidth() * 32)) / 2,
                        static_cast<float>((mapState.board.getHeight() * 32)) / 2));
                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::Text("%s", mapKey.c_str());
                showMapStats(mapState, 170.0f, 20.0f);
                ImGui::EndGroup();
                nameKey++;

                int j = 0;
                for(YAML::Node& node : mapNode.second)
                {
                    j++;
                    ImGui::Indent();
                    ImGui::BeginGroup();
                    heading("Game " + std::to_string(j));

                    for (YAML::detail::iterator_value nameValPair : node)
                    {
                        auto param = nameValPair.first.as<std::string>();
                        if (param == "Battle")
                        {
                            auto val = (nameValPair.second.as<int>()) ? "True" : "False";
                            std::string fullParam = param + ": " + val;
                            ImGui::Text("%s", fullParam.c_str());
                        }
                        if (param == "Seed" || param == "Turns")
                        {
                            std::string fullParam = param + ": " + nameValPair.second.as<std::string>();
                            ImGui::Text("%s", fullParam.c_str());
                        }
                        if (param == "PlayerAssignment")
                        {
                            players = nameValPair.second.as<std::vector<std::string>>(std::vector<std::string>());
                            std::string fullParam = param + ": ";
                            int j = 0;
                            for (const auto& val : players)
                            {
                                fullParam += "Player " + std::to_string(j) + " = " + val + "     ";
                                j++;
                            }
                            ImGui::Text("%s", fullParam.c_str());
                            yPadding();
                        }
                        if (param == "ActivePlayer" || param == "ActionCount")
                        {
                            auto vec = nameValPair.second.as<std::vector<std::string>>(std::vector<std::string>());
                            std::string vals = "[";
                            for (const auto& val : vec)
                            {
                                vals.append(val + ", ");
                            }
                            vals.append("]");
                            std::string fullParam = param + ": " + vals;
                            ImGui::TextWrapped("%s", fullParam.c_str());
                            yPadding();
                        }
                        if (param == "WinnerID")
                        {
                            int winnerID = nameValPair.second.as<int>();
                            std::string winnerName = (winnerID != -1) ? "Player " + std::to_string(winnerID) + ": " + players[winnerID].c_str() : "Draw";
                            std::string fullParam = "Winner: " + winnerName;
                            ImGui::Text("%s", fullParam.c_str());
                        }
                    }
                    ImGui::EndGroup();
                    ImGui::Unindent();
                }
                ImGui::Unindent();
            yPadding();
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void ExistingMapRenderer::createSaveBeforeCloseWindow()
    {
        ImGui::OpenPopup("Save Before Quit?");
        if (ImGui::BeginPopupModal("Save Before Quit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("All those beautiful maps will be deleted. "
                        "This operation cannot be undone!");
            yPadding();

            if (ImGui::Button("OK", ImVec2(180, 0)))
            {
                exitMapMaker = false;
                ImGui::CloseCurrentPopup();
                ImGui::OpenPopup("Save");
            }
            ImGui::SetItemDefaultFocus();ImGui::SameLine();
            if (ImGui::Button("Quit Without Save", ImVec2(180, 0)))
            {
                //TODO close whole while loop
                exitMapMaker = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void ExistingMapRenderer::progressBars(std::map<std::string, int> stats, std::string title, float barWidth, float barHeight)
    {
        int j = 0;
        heading(title);
        ImGui::BeginGroup(); // main group
        ImGui::BeginGroup(); // group closed in loop
        for (auto& stat : stats)
        {
            j++;
            std::string text = stat.first + ": ";
            ImGui::Text("%s", text.c_str());
            float progress = float(stat.second)/100.0f, progress_dir = 1.0f;
            ImGui::ProgressBar(progress, ImVec2(barWidth, barHeight));
            if (stats.size() / 2 == j)
            {
                ImGui::EndGroup();
                ImGui::SameLine(250.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::BeginGroup();
            }
        }
        ImGui::EndGroup(); // group closed in loop
        ImGui::EndGroup(); // main group
    }

    void ExistingMapRenderer::metricBreakdown(MapState mapSate, float barWidth, float barHeight)
    {
        yPadding();
        std::string label = "Show Map Fitness Breakdown for " + mapSate.mapName;
        if (!ImGui::CollapsingHeader(label.c_str()))
            return;

        ImGui::PushID(label.c_str());
        ImGui::Indent();

        if (mapSate.feasible) {
            progressBars(mapSate.entityMapStats, "Entity Related Metrics", barWidth, barHeight);
        }
        progressBars(mapSate.symmetryMapStats, "Symmetry Related Metrics", barWidth, barHeight);

        ImGui::Unindent();
        ImGui::PopID();
    }

    void ExistingMapRenderer::showMapStats(MapState mapSate, float barWidth, float barHeight)
    {
        ImGui::BeginGroup();
        if (!mapSate.feasible) {
            ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4) ImColor::HSV(0 / 7.0f, 0.6f, 0.8f));
            ImGui::TextWrapped("This map is not feasible; at least two agent owned "
                               "entities must be provided and all entities must be "
                               "reachable from all other entities");
            ImGui::PopStyleColor(1);
        }
        yPadding();
        std::string text = "Map Width: " + std::to_string(mapSate.board.getWidth());
        ImGui::Text("%s", text.c_str());
        text = "Map Height: " + std::to_string(mapSate.board.getHeight());
        ImGui::Text("%s", text.c_str());
        yPadding();
        if (mapSate.feasible) {
            ImGui::BeginGroup();
            int i = 0;
            for (auto &stat : mapSate.basicMapStats) {
                i++;
                std::string text = stat.first + ": " + std::to_string(stat.second);
                ImGui::Text("%s", text.c_str());
                if ((mapSate.basicMapStats.size()) / 2 == i) {
                    ImGui::EndGroup();
                    ImGui::SameLine(250.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                    ImGui::BeginGroup();
                }
            }
            ImGui::EndGroup();
            progressBars(mapSate.fitnessMapStats, "Map Fitness", barWidth, barHeight);
        }
        else {
            heading("Map Fitness");
            auto& stat = mapSate.fitnessMapStats.at("Overall Symmetry");
            ImGui::Text("Map Symmetry");
            float progress = float(stat)/100.0f, progress_dir = 1.0f;
            ImGui::ProgressBar(progress, ImVec2(barWidth, barHeight));
            yPadding();
        }
        metricBreakdown(mapSate, barWidth, barHeight);
        ImGui::EndGroup();
    }

    void ExistingMapRenderer::createNewMap(std::string& mapName, int& mapWidth, int& mapHeight)
    {
        for (auto& tileType : config->tileTypes)
        {
            if(tileType.second.isDefaultTile)
            {
                defaultTile = tileType.second;
            }
        }
        std::vector<Tile> tiles;
        for (int y = 0; y < static_cast<int>(mapHeight); y++)
        {
            for (int x = 0; x < static_cast<int>(mapWidth); x++)
            {
                tiles.emplace_back(defaultTile.toTile(x, y));
            }
        }

        auto newMap = SGA::MapState(Grid2D<Tile>(mapWidth, tiles.begin(), tiles.end()));
        newMap.mapName = mapName;
        m_availableMaps.insert({mapName, newMap});
        m_availableMaps.at(mapName).miniMap.init(this->state, *config, *config->renderConfig);
        saveMiniMapTexture(m_availableMaps, mapName);
        mapNames.push_back(mapName);
    }

    void ExistingMapRenderer::saveTileMap(auto& selectionArray, bool& saveSelected, auto fileName)
    {
        std::filesystem::path filePath = config->yamlPath;
        std::ofstream mapLogPath("../../gameConfigs/" + filePath.parent_path().stem().string() + "/" + fileName + filePath.stem().string() + ".yaml");

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Maps";
        out << YAML::BeginMap;
        int i = 0;
        for(auto& mapName : mapNames)
        {
            if(selectionArray[i] || !saveSelected){
                auto& map = m_availableMaps.at(mapNames[i]);
                auto board = map.getBoardString(config);
                out << YAML::Key << "Map" + std::to_string(i);
                out << YAML::Value << YAML::Literal << board;
            }
            i++;
        }
        out << YAML::EndMap << YAML::EndMap;
        mapLogPath << out.c_str();

    }

    void ExistingMapRenderer::runTileMap(int& seed, int& numberOfGames)
    {
        std::filesystem::path filePath = config->yamlPath;
        std::string gameType = filePath.parent_path().stem().string();
        std::string gameName = filePath.stem().string();
        std::string fullPath = "../../gameConfigs/" + gameType + "/" + gameName + ".yaml";
        m_GameConfig = SGA::loadConfigFromYAML(fullPath);

        int playerCount = 2;
        std::string logPath = "./sgaLog.yaml";
        std::string mapsPath = "../../gameConfigs/" + gameType + "/run" + gameName + ".yaml";

        // Run games
        SGA::Log::setDefaultLogger(std::make_unique<SGA::FileLogger>(logPath));
        Arena arena(*m_GameConfig);

        if(mapsPath.empty())
        {
            arena.runGames(playerCount, seed, numberOfGames);
        }
        else
        {
            //Load level definitions and exchange them in the game config
            m_GameConfig->levelDefinitions = SGA::loadLevelsFromYAML(mapsPath, *m_GameConfig);
            //Run combinations per map
            const int mapNumber = m_GameConfig->levelDefinitions.size();
            arena.runGames(playerCount, seed, numberOfGames, mapNumber);
        }

        gameRun = true;
        auto configNode = YAML::LoadFile("sgaLog.yaml");
        runOutputLog = configNode;
    }

    void ExistingMapRenderer::updateMiniMapTexture()
    {
        renderedMap->miniMap.update(renderedMap->board, renderedMap->entities);
        if (!minimapTexture.create(renderedMap->board.getWidth()*32, renderedMap->board.getHeight()*32))
            std::cout << "Could not create the minimap texture";

        minimapTexture.clear(sf::Color::White);
        minimapTexture.draw(renderedMap->miniMap);
        minimapTexture.display();
        assetCache.updateTexture("minimap " + renderedMap->mapName, minimapTexture.getTexture());
    }

    void ExistingMapRenderer::saveMiniMapTexture(std::unordered_map<std::string, MapState>& location, std::string& mapName)
    {
        location.at(mapName).miniMap.update(location.at(mapName).board, location.at(mapName).entities);
        if (!minimapTexture.create(location.at(mapName).board.getWidth()*32, location.at(mapName).board.getHeight()*32))
            std::cout << "Could not create the minimap texture";

        minimapTexture.clear(sf::Color::White);
        minimapTexture.draw(location.at(mapName).miniMap);
        minimapTexture.display();
        assetCache.saveTexture( "minimap " + mapName, minimapTexture.getTexture());
    }

    void ExistingMapRenderer::updateGeneratedMiniMapTexture(std::map<std::string, MapState>& location, std::string& mapName)
    {
        location.at(mapName).miniMap.update(location.at(mapName).board, location.at(mapName).entities);
        if (!minimapTexture.create(location.at(mapName).board.getWidth()*32, location.at(mapName).board.getHeight()*32))
            std::cout << "Could not create the minimap texture";

        minimapTexture.clear(sf::Color::White);
        minimapTexture.draw(location.at(mapName).miniMap);
        minimapTexture.display();
        auto& texture = minimapTexture.getTexture();
        assetCache.updateTexture( "minimap " + mapName, texture);

    }

    void ExistingMapRenderer::showGeneratedMaps(std::map<std::string, MapState> mapStateArray, bool useMapElites)
    {
        int i = 1;
        std::pair<std::string, MapState> clickedMapState;
        if (useMapElites && !mapStateArray.empty())
        {
            auto& board = mapElites.map;
            for (int y = 0; y < (int)board.getHeight(); ++y)
            {
                ImGui::BeginGroup();
                if(y==0)
                    ImGui::Text("\n\n");
                int yStart = y * board.incrementHeightVal * 100;
                int yEnd = yStart + board.incrementHeightVal * 100;
                auto yDim = (int)mapGenerationParams.mapElitesYAxis;
                ImGui::Text("%s%s", enum_str[yDim], ":");
                ImGui::Text("%i%s%i%s", yStart, "-", yEnd, "%");
                ImGui::EndGroup();
                ImGui::SameLine();
                for (int x = 0; x < (int)board.getWidth(); ++x)
                {
                    ImGui::BeginGroup();
                    if (y==0)
                    {
                        int start = x * board.incrementWidthVal * 100;
                        int end = start + board.incrementWidthVal * 100;
                        auto xDim = (int)mapGenerationParams.mapElitesXAxis;
                        ImGui::Text("%s%s", enum_str[xDim], ":");
                        ImGui::Text("%i%s%i%s", start, "-", end, "%");
                    }

                    const auto& mapState = board[{x, y}];
                    clickedMapState = std::pair(mapState.mapName, mapStateArray.at(mapState.mapName));
                    sf::Texture &texture1 = assetCache.getTexture("minimap " + mapState.mapName);
                    if (ImGui::ImageButton(texture1, ImVec2(
                            static_cast<float>((renderedMap->board.getWidth() * 32)) / mapGenerationParams.zoomLevel,
                            static_cast<float>((renderedMap->board.getHeight() * 32)) / mapGenerationParams.zoomLevel)))
                    {
                        if(mapState.board.grid.size() != 0)
                        {
                            if (!popupOpen)
                                selectedMiniMap = clickedMapState;
                            popupOpen = true;
                            ImGui::OpenPopup("Generated Map Actions");
                        }
//                            ImGui::OpenPopup("Generated Map Actions");
//                        break;
                    } ImGui::EndGroup();
                    if (x != (int)board.getWidth()-1)
                        ImGui::SameLine();

                }
            }
        }
        else
        {
            for (auto &map : mapStateArray)
            {
                clickedMapState = map;
                auto& mapState = map.second;
                sf::Texture &texture1 = assetCache.getTexture("minimap " + map.first);
                if (ImGui::ImageButton(texture1, ImVec2(
                        static_cast<float>((renderedMap->board.getWidth() * 32)) / 2,
                        static_cast<float>((renderedMap->board.getHeight() * 32)) / 2)))
                {
                    if(mapState.mapName != "")
                        ImGui::OpenPopup("Generated Map Actions");
                    break;
                }

                if (i % 3)
                    ImGui::SameLine();
                i++;
            }
        }

        if (ImGui::BeginPopupModal(
                "Generated Map Actions",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize))
        {
            yPadding();
            ImGui::BeginGroup();

            if (!popupOpen)
                selectedMiniMap = clickedMapState;
            popupOpen = true;

            auto& mapKey = selectedMiniMap.first;
            auto& mapState = selectedMiniMap.second;
            sf::Texture &selectedMap = assetCache.getTexture("minimap " + mapKey);
            ImGui::Image(selectedMap, ImVec2(
                    static_cast<float>((mapState.board.getWidth() * 32)) / 2,
                    static_cast<float>((mapState.board.getHeight() * 32)) / 2));
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("%s", mapKey.c_str());
            static char infoName[50];
            ImGui::InputText("New Map Name (*required for saving to available maps)", infoName, 50);


            showMapStats(mapState, 170.0f, 20.0f);
            ImGui::EndGroup();
            yPadding();
            ImGui::BeginGroup();
            if(mapGenerationParams.useMapElites)
            {
                if (ImGui::Button("Use as Elite", ImVec2(200, 0))) {
                    mapGenerationParams.manualEliteSelection = true;
                    mapElites.chosenElite = &mapState;
                    runMapElitesAlgorithm();
                    ImGui::CloseCurrentPopup();
                    popupOpen = false;
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Apply to current map", ImVec2(200, 0))) {
                std::string currentMapName = renderedMap->mapName;
                m_availableMaps.at(currentMapName) = mapState;
                m_availableMaps.at(currentMapName).mapName = currentMapName;
                renderedMap->calcMetrics();
                tileMap.update(renderedMap, renderOverlayView, overlayType, explorationPlayerID);
                entityRenderer.update(renderedMap->entities);
                updateMiniMapTexture();
//                runSimpleGeneticAlgorithm();
                generateMapSuggestions();
                ImGui::CloseCurrentPopup();
                popupOpen = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save to available maps", ImVec2(200, 0))) {
                if (infoName[0] != '\0')
                {
                    newMapName = infoName;
                    mapState.mapName = infoName;
                    m_availableMaps.insert({newMapName, mapState});
                    m_availableMaps.at(newMapName).miniMap.init(
                            this->state,
                            *config,
                            *config->renderConfig);
                    saveMiniMapTexture(m_availableMaps, newMapName);
                    mapNames.push_back(newMapName);
                    ImGui::CloseCurrentPopup();
                    popupOpen = false;
                    newMapName = "";
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(200, 0))) {
                ImGui::CloseCurrentPopup();
                popupOpen = false;
            }
            ImGui::EndGroup();
            ImGui::EndPopup();
        }
    }

    void ExistingMapRenderer::setPlayerPointOfView(int playerID)
    {
    }

    ActionAssignment ExistingMapRenderer::getPlayerActions()
    {
    }

    bool ExistingMapRenderer::isGameEndRequested()
    {
        return endGameRequested;
    }

    void ExistingMapRenderer::runSimpleGeneticAlgorithm()
    {
        if (mapGenerationParams.aiEnabled)
        {
            auto& generatedMaps = mapGenerationParams.generatedMaps;

            // run the algorithm
            auto simpleGA = SimpleGeneticAlgorithm(mapGenerationParams);
            auto generated = simpleGA.run(mapGenerationParams);

            // remove old map suggestions (not map elites)
            generatedMaps.clear();

            //add new suggestions and create their minimaps
            int i = 1;
            for(auto& genMap : generated)
            {
                std::string name = genMap.mapName + " " + std::to_string(i);
                genMap.mapName = name;
                generatedMaps.insert({name, genMap});
                generatedMaps.at(name).miniMap.init(
                        this->state,
                        *config,
                        *config->renderConfig);
                updateGeneratedMiniMapTexture(generatedMaps, name);
                i++;
            }
        }
    }

    void ExistingMapRenderer::runMapElitesAlgorithm()
    {
        std::string eliteUsed;
        if (mapGenerationParams.manualEliteSelection)
        {
            eliteUsed = mapElites.chosenElite->mapName;
            advanceMapElitesAlgorithm(mapElites.chosenElite);
        }

        else if (mapGenerationParams.chooseRandomElite)
        {
            mapElites.chooseRandomElite();
            eliteUsed = mapElites.chosenElite->mapName;
            advanceMapElitesAlgorithm(mapElites.chosenElite);
            mapGenerationParams.chooseRandomElite = false;
        }
        else
        {
            eliteUsed = renderedMap->mapName;
            advanceMapElitesAlgorithm(renderedMap);
        }

        std::cout << "\n=========================================\n" << eliteUsed << "\n";
        for (auto& hit: mapElites.map.cellHits)
        {
            std::cout << "pos: [" << hit.first.x << ", " << hit.first.y << "] hit no. :" << hit.second << "\n";
        }
        std::cout << "\n=========================================\n";

        auto& generatedMaps = mapGenerationParams.mapElitesGenMaps;

        int i = 1;
        for (int x = 0; x < (int)mapElites.map.getWidth(); ++x)
        {
            for (int y = 0; y < (int)mapElites.map.getHeight(); ++y)
            {
                i++;
                // get the current state
                auto& mapState = mapElites.map[{x, y}];
                std::string name = "MAP-Elites [" + std::to_string(x) + "," + std::to_string(y) + "]";
                mapState.mapName = name;

                if(generatedMaps.contains(name))
                    generatedMaps.at(name) = mapState;
                else
                    generatedMaps.insert({name, mapState});
                generatedMaps.at(name).miniMap.init(
                        this->state,
                        *config,
                        *config->renderConfig);
                updateGeneratedMiniMapTexture(generatedMaps, name);
            }
        }
    }

    void ExistingMapRenderer::advanceMapElitesAlgorithm(MapState* chosenElite)
    {
        mapElites.runGeneration(chosenElite);
        mapGenerationParams.manualEliteSelection = false;
    }

    void ExistingMapRenderer::generateMapSuggestions()
    {
        if(mapGenerationParams.aiEnabled)
        {
            if (mapGenerationParams.useMapElites)
                runMapElitesAlgorithm();
            else
                runSimpleGeneticAlgorithm();
        }
    }

    void ExistingMapRenderer::yPadding()
    {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
    }

    void ExistingMapRenderer::heading(std::string title)
    {
        yPadding();
        ImGui::Separator();
        ImGui::Text("%s", title.c_str());
        ImGui::Separator();
        yPadding();
    }
}
