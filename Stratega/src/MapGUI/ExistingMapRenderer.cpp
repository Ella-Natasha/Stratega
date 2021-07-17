#include <Stratega/MapGUI/ExistingMapRenderer.h>
#include <Stratega/GUI/GridUtils.h>
//#include <Stratega/Configuration/RenderConfig.h>
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
            : config(nullptr),
              state(),
              fowState(),
              selectedAction(),
              window(sf::VideoMode().getDesktopMode(), "Stratega Tile Map Editor", sf::Style::Default | sf::Style::Titlebar),
              pointOfViewPlayerID(0),
              fowSettings(),
              zoomValue(5.f),
              dragging(false),
              renderedMap(nullptr)
    {
        fowSettings.selectedPlayerID = pointOfViewPlayerID;
        fowSettings.renderFogOfWar = false;
        fowSettings.renderType = FogRenderType::Fog;

        // Initialize View
        sf::View view = window.getView();
        view.setCenter(window.getSize().x / 2., window.getSize().y / 2.);
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
        assetCache.loadFont("font", "../GUI/Assets/arial.ttf");

        tileMap.init(initialState, gameConfig, *gameConfig.renderConfig);
        entityRenderer.init(gameConfig, *gameConfig.renderConfig);
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
            mapState.mapName = "Original Map " + std::to_string(i);

            //Instance Entities
            for (auto& entity : levelDef.second.entityPlacements)
            {
                mapState.addEntity(*entity.entityType, entity.ownerID, entity.position);
            }

            m_availableMaps.insert({"Original Map " + std::to_string(i), mapState});
            mapNames.push_back("Original Map " + std::to_string(i));
            m_availableMaps.at("Original Map " + std::to_string(i)).miniMap.init(initialState, gameConfig, *gameConfig.renderConfig);
            m_availableMaps.at("Original Map " + std::to_string(i)).calcMetrics();
            i++;
        }
        renderedMap = &m_availableMaps.at("Original Map 1");
        renderedMap->calcMetrics();
    }


    void ExistingMapRenderer::update(const GameState& state)
    {
        this->state = state;
        updateFow();
        selectedAction.reset();

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
        renderedMap->validateMap();
        tileMap.update(renderedMap->board);
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

        //Draw entities
        entityRenderer.update(renderedMap->entities);
        window.draw(entityRenderer);

        if (entitySelected)
        {
            sf::Texture& texture = assetCache.getTexture(m_CurrentEntitySelection.name);
            sf::Vector2f origin(TILE_ORIGIN_X, TILE_ORIGIN_Y);
            sf::Sprite selectedTile(texture);

            // minus numbers = bad hack to get position on lower render level
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
            sf::Vector2i pos = toGrid(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

            // If tile selected add to board
            if (tileSelected && renderedMap->board.isInBounds((int)pos.x,(int)pos.y))
            {
                renderedMap->board[{pos.x, pos.y}] = m_CurrentTileSelection.toTile(pos.x, pos.y);
                renderedMap->calcMetrics();
                tileMap.update(renderedMap->board);
                updateMiniMapTexture();

                auto simpleGA = SimpleGeneticAlgorithm(state.gameInfo->tileTypes, state.gameInfo->entityTypes, renderedMap);
                auto generated = simpleGA.run(25, 10);
                int i = 1;
                for(auto& genMap : generated)
                {
                    std::string name = genMap.mapName + std::to_string(i);
                    m_generatedMaps.insert({name, genMap});
                    m_generatedMaps.at(name).miniMap.init(this->state, *config, *config->renderConfig);
                    saveMiniMapTexture(m_generatedMaps, name);

                    i++;
                }
            }

            if (tileSelected && !renderedMap->board.isInBounds((int)pos.x,(int)pos.y))
            {
                tileSelected = false;
            }

            // If entity selected add to board
            if (entitySelected && renderedMap->board.isInBounds((int)pos.x,(int)pos.y))
            {
                renderedMap->addEntity(m_CurrentEntitySelection, entityOwnerID,Vector2f(pos.x, pos.y));
                renderedMap->calcMetrics();
                entityRenderer.update(renderedMap->entities);
                updateMiniMapTexture();
            }

            if (entitySelected && !renderedMap->board.isInBounds((int)pos.x,(int)pos.y))
            {
                entitySelected = false;
            }

            dragging = true;
            oldMousePosition = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
        }
        if (event.mouseButton.button == sf::Mouse::Right)
        {
            sf::Vector2i pos = toGrid(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));
            tileSelected = false;
            entitySelected = false;

            if (renderedMap->board.isInBounds((int)pos.x,(int)pos.y) && renderedMap->getEntityAt(Vector2f(pos.x, pos.y)))
            {
                renderedMap->removeEntityAt(Vector2f(pos.x, pos.y));
                updateMiniMapTexture();
            }

        }
    }

    void ExistingMapRenderer::mouseMoved(const sf::Event& event)
    {
        if (!dragging)
            return;

        // Determine how the cursor has moved
        auto newPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
        auto deltaPos = oldMousePosition - newPos;

        // Move our view accordingly and update the window
        sf::View view = window.getView();
        view.setCenter(view.getCenter() + deltaPos);
        window.setView(view);

        // Save the new position as the old one
        // We're recalculating this, since we've changed the view
        oldMousePosition = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
    }

    void ExistingMapRenderer::createHUD()
    {
        createWindowInfo();
        createEditWindow();
        createActionWindow();
        createAvailableMapWindow();
        createMainMapViewWindow();
        if(gameRun){ createRunResultsWindow(); }
        if(editConfig){ createEditConfigWindow(configKey); }
        if(exitMapMaker){createSaveBeforeCloseWindow();}
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

        std::string text = "\nGame Type: " + filePath.parent_path().stem().string();
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
        ImGui::Text("\n");
        ImGui::EndGroup();ImGui::SameLine(250.0f, ImGui::GetStyle().ItemInnerSpacing.x);

        ImGui::BeginGroup();
        ImGui::Text("\n");
        if (ImGui::Button("Edit Game Config", ImVec2(150, 30)))
        {
            configKey = 0;
            editConfig = true;
        }
        ImGui::EndGroup();

        ImGui::Separator();
        text = "Map Stats: for " + renderedMap->mapName;
        ImGui::Text("%s", text.c_str());
        ImGui::Separator();
        showMapStats(m_availableMaps.at(renderedMap->mapName));

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
            tileMap.update(renderedMap->board);
            updateMiniMapTexture();
        }

        if (ImGui::Button("Clear All Entities", ImVec2(300, 30)))
        {
            renderedMap->clearAllEntities();
            entityRenderer.update(renderedMap->entities);
            updateMiniMapTexture();
        }
        ImGui::Text("\n");
        ImGui::Separator();
        ImGui::Text("Available Tiles: ");
        ImGui::Separator();
        ImGui::Text("\n");

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
        ImGui::Text("\n\n");
        ImGui::Separator();
        ImGui::Text("Available Entities: ");
        ImGui::Separator();
        ImGui::Text("\n");

        if (ImGui::Button("Edit Entity Parameters", ImVec2(300, 30)))
        {
            configKey = 1;
            editConfig = true;
        }

        ImGui::Text("\n");

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
        if (ImGui::BeginPopupModal("Select Entity Owner", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            int i = 0;
            for (auto& agent : config->agentParams)
            {
                std::string buttonName = "Agent " + std::to_string(i) + ": " + agent.first.c_str();
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

            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
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

        ImGui::Text("All available maps will be saved/run, if you only wish to save/run a few\n"
                    "please select the maps you would like to save/run below\n\n");
        ImGui::Separator();

        static bool saveMultiple = false;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Select Maps to Save or Run\n\n", &saveMultiple);
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

        ImGui::Text("\n");

        if (ImGui::Button("Save TileMaps", ImVec2(200, 30)))
        {
            ImGui::OpenPopup("Save");
        }
        if (ImGui::BeginPopupModal("Save", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Please provide a name for the map YAML file.\n"
                        "The name given will be appended with the name of the game.\n");

            static char fileName[50];
            ImGui::InputText("File Name", fileName, 50);
            ImGui::Text("\n");

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
        if (ImGui::BeginPopupModal("Run", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("The results of any runs are provided on screen and in the YAML output logs.");
            static int seedValue = 0;
            static int gameNumValue = 1;
            ImGui::PushItemWidth(80.0f);
            ImGui::InputInt("Seed", &seedValue);ImGui::SameLine();
            ImGui::InputInt("Number of Games", &gameNumValue);ImGui::PopItemWidth();

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
        ImGui::RadioButton("Navmesh", &e, 1); ImGui::SameLine();
        ImGui::RadioButton("Resource Safety", &e, 2); ImGui::SameLine();
        ImGui::RadioButton("Safe Areas", &e, 3); ImGui::SameLine();
        ImGui::RadioButton("Unused Space", &e, 4); ImGui::SameLine();
        ImGui::RadioButton("Segments", &e, 5);

        ImGui::EndChild();
        ImGui::End();
    }

    void ExistingMapRenderer::createAvailableMapWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Available Maps");

        ImGui::BeginChild("Scrolling");

        for (auto& map : m_availableMaps) {
            if(map.first != renderedMap->mapName)
            {
                sf::Texture &maptexture = assetCache.getTexture("minimap " + map.first);
                if (ImGui::ImageButton(maptexture, ImVec2((map.second.board.getWidth() * 32) / 2,
                                                          (map.second.board.getHeight() * 32) / 2))) {
                    renderedMap = &m_availableMaps.at(map.first);
                    tileMap.update(renderedMap->board);
                    entityRenderer.update(renderedMap->entities);
                }
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Separator();
                std::string mapName = "Map Name: " + map.first;
                ImGui::Text("%s", mapName.c_str());
                ImGui::Separator();
                showMapStats(map.second, 150.0f, 20.0f);
                ImGui::EndGroup();
            }
        }

        if (ImGui::Button("Create New Map", ImVec2(200, 30)))
        {
            ImGui::OpenPopup("New Map Setup");
        }
        if (ImGui::BeginPopupModal("New Map Setup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
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
                    tileMap.update(renderedMap->board);
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
        ImGui::Text("\n");
        ImGui::Separator();
        ImGui::Text("AI Generated Map Sugestions");
        ImGui::Separator();
        ImGui::Text("\nThe following map suggestions show alternatives "
                    "to the current map along with \nmetrics compared to the current map."
                    "\nThese maps can be previewed, saved as new map or applied to the "
                    "current open map.\n\n");

        int i = 1;
        for (auto& map : m_generatedMaps)
        {
//            if (map.first == renderedMap->mapName)
//            {
                sf::Texture& texture1 = assetCache.getTexture("minimap " + map.first);
                if (ImGui::ImageButton(texture1, ImVec2((map.second.board.getWidth()*32)/2, (map.second.board.getHeight()*32)/2)))
                {
                    ImGui::OpenPopup("Generated Map Actions");
                }
                if (ImGui::BeginPopupModal("Generated Map Actions", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::BeginGroup();
                    showMapStats(map.second, 170.0f, 20.0f);
                    if (ImGui::Button("Preview", ImVec2(100, 0)))
                    {

                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Apply", ImVec2(100, 0)))
                    {

                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Save", ImVec2(100, 0)))
                    {

                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(100, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndGroup();
                    ImGui::EndPopup();
                }
                if(i % 3)
                    ImGui::SameLine();
                i++;
//            }
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

        ImGui::Text("Any edits made to the following details will change the current game configuration.\n"
                    "As a result any changes applied will be saved into a new YAML file.\n\n");

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            //TODO add save config func
            editConfig = false;
        } ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            editConfig = false;
        }
        ImGui::Text("\n");
        ImGui::Separator();
        ImGui::Text("\n");

        static int e = configTypeKey;
        ImGui::RadioButton("Agent Config", &e, 0); ImGui::SameLine();
        ImGui::RadioButton("Entity Config", &e, 1); ImGui::SameLine();
        ImGui::RadioButton("Tile Config", &e, 2);
        ImGui::Text("\n");
        ImGui::Separator();

        if(e == 0)
        {
            ImGui::Text("\nPlease Select all agents you wish to use from the list below\n(ctrl click to select multiple)\n\n");
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
                //Add units
                std::string name = "\nEntity Name: " + entity.second.name;
                sf::Texture& texture = assetCache.getTexture(entity.second.name);
                ImGui::Image(texture, ImVec2(150, 150));

                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("%s", name.c_str());
                std::string sight = "Current Value: " + std::to_string(int(entity.second.lineOfSight));
                int newValue = int(entity.second.lineOfSight);
                ImGui::PushItemWidth(80.0f);
                ImGui::InputInt("Line of Sight:", &newValue);ImGui::SameLine();
                ImGui::PopItemWidth();

                ImGui::Text("%s", sight.c_str());


                for (auto& param : entity.second.parameters)
                {
                    std::string curParamStr = "Current Value: " + std::to_string(int(param.second.defaultValue));
                    std::string newParamStr = param.second.name + ": ";
                    int newParamValue = int(param.second.defaultValue);
                    ImGui::PushItemWidth(80.0f);
                    ImGui::InputInt(newParamStr.c_str(), &newParamValue);ImGui::SameLine();
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
        std::string gameRunName = "";
        int i = 1;
        int nameKey = 0;
        for (YAML::detail::iterator_value logNode : runOutputLog)
        {
            auto gameName = logNode.first.as<std::string>();
            std::vector<std::string> players;
            if (gameRunName != gameName)
            {
                i = 1;
                gameRunName = gameName;
                auto mapName = "Map " + logNode.second["Map"].as<std::string>() + ": " + mapNames[nameKey];
                ImGui::Separator();
                ImGui::Text("%s", mapName.c_str());
                ImGui::Separator();
                nameKey++;
            }

            std::string gameNum = "\nGame " + std::to_string(i) + ":";
            ImGui::Text("%s", gameNum.c_str());
            i++;

            for (const auto& nameValPair : logNode.second)
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
                    ImGui::Text("%s", fullParam.c_str());
                }
                if (param == "WinnerID")
                {
                    int winnerID = nameValPair.second.as<int>();
                    std::string winnerName = (winnerID != -1) ? players[winnerID] : "Draw";
                    std::string fullParam = "Winner: " + winnerName;
                    ImGui::Text("%s", fullParam.c_str());
                }
            }
            ImGui::Text("\n");
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void ExistingMapRenderer::createSaveBeforeCloseWindow()
    {
        ImGui::OpenPopup("Save Before Quit?");
        if (ImGui::BeginPopupModal("Save Before Quit?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("All those beautiful maps will be deleted.\nThis operation cannot be undone!\n\n");

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

    void ExistingMapRenderer::showMapStats(MapState mapSate, float barWidth, float barHeight)
    {
        std::string text = "\nMap Width: " + std::to_string(mapSate.board.getWidth());
        ImGui::Text("%s", text.c_str());
        text = "Map Height: " + std::to_string(mapSate.board.getHeight()) + "\n\n";
        ImGui::Text("%s", text.c_str());

        if (mapSate.feasible)
        {
            ImGui::BeginGroup();
            int i = 0;
            for (auto& stat : mapSate.basicMapStats)
            {   i++;
                text = stat.first + ": " + std::to_string(stat.second);
                ImGui::Text("%s", text.c_str());
                if ((mapSate.basicMapStats.size()) / 2 == i)
                {
                    ImGui::EndGroup();
                    ImGui::SameLine(250.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                    ImGui::BeginGroup();
                }

            }
            ImGui::EndGroup();
            ImGui::Text("\n");

            int j = 0;
            ImGui::BeginGroup();
            for (auto& stat : mapSate.progressbarMapStats)
            {
                j++;
                text = stat.first + ": ";
                ImGui::Text("%s", text.c_str());
                float progress = float(stat.second)/100.0f, progress_dir = 1.0f;
                ImGui::ProgressBar(progress, ImVec2(barWidth, barHeight));
                if (mapSate.basicMapStats.size() / 2 == j)
                {
                    ImGui::EndGroup();
                    ImGui::SameLine(250.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                    ImGui::BeginGroup();
                }
            }
            ImGui::EndGroup();
        }
        else
        {
            ImGui::Text("This map is not feasible; at least two agent owned\n"
                        "entities must be provided and all entities must be\n"
                        "reachable from all other entities");
        }
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

    void ExistingMapRenderer::setPlayerPointOfView(int playerID)
    {
        pointOfViewPlayerID = playerID;
        fowSettings.selectedPlayerID = playerID;
        update(state);
    }

    ActionAssignment ExistingMapRenderer::getPlayerActions()
    {
        ActionAssignment returnValue;
        if(isActionAvailable())
        {
            returnValue.assignActionOrReplace(selectedAction.value());
        }
        return returnValue;
    }

    bool ExistingMapRenderer::isActionAvailable() const
    {
        return selectedAction.has_value();
    }

    void ExistingMapRenderer::updateFow()
    {
        fowState = state;
        fowState.applyFogOfWar(fowSettings.selectedPlayerID);
    }

    bool ExistingMapRenderer::isGameEndRequested()
    {
        return endGameRequested;
    }

}
