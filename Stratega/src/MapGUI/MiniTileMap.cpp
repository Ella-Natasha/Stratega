#include <Stratega/MapGUI/MiniTileMap.h>
#include <Stratega/GUI/GridUtils.h>
#include <Stratega/Configuration/RenderConfig.h>
#include <Stratega/Configuration/GameConfig.h>
#include <Stratega/Representation/GameState.h>
#include <Stratega/MapGUI/MapState.h>

namespace SGA
{
    void MiniTileMap::init(const GameState& state, const GameConfig& gameConfig, const RenderConfig& renderConfig)
    {
        // Initialise atlas
        std::vector<std::string> minifilePaths;
        for (const auto& namePathPair : renderConfig.tileSpritePaths)
        {
            auto path = namePathPair.second;
            auto pos = path.rfind('/');
            if (pos != std::string::npos) {
                auto miniPath = "../MapBuilder/Assets" + path.substr(pos);
                minifilePaths.emplace_back(miniPath);
                miniTileSpritePaths.emplace(gameConfig.getTileID(namePathPair.first), miniPath);
            }
        }
        minifilePaths.emplace_back("../MapBuilder/Assets/entity.png");
        miniTileSpritePaths.emplace(-2, "../MapBuilder/Assets/entity.png");
        miniTileset.init(minifilePaths);

        // resize the vertex array to fit the level size
        const auto& board = state.board;
        m_vertices.setPrimitiveType(sf::Quads);
        m_vertices.resize(board.getWidth() * board.getHeight() * 4);
    }

    void MiniTileMap::update(const Grid2D<Tile>& board, const std::vector<Entity>& entities)
    {
        m_vertices.resize(board.getWidth() * board.getHeight() * 4);

        // populate the vertex array, with one quad per tile
        for (int x = 0; x < static_cast<int>(board.getWidth()); ++x)
        {
            for (int y = 0; y < static_cast<int>(board.getHeight()); ++y)
            {
                // get the current tile
                const auto& tile = board[{x, y}];
                int entityOwnerID = -2;

                for(int i = 0; i < static_cast<int>(entities.size()); i++)
                {
                    const auto& entity = entities.at(i);
                    if (entity.position == Vector2f((double)x, (double)y))
                    {
                        entityOwnerID = entity.ownerID;
                        break;
                    }
                }
                sf::Color colour = (entityOwnerID < 0 ) ? sf::Color::White : ENTITY_COLORS[entityOwnerID];
                // get a pointer to the current tile's quad
                sf::Vertex* quad = &m_vertices[(x + y * board.getWidth()) * 4];
                updateTileQuad(quad, tile, colour, entityOwnerID);

            }
        }

    }

    void MiniTileMap::updateTileQuad(sf::Vertex* quadPtr, const Tile& tile, const sf::Color& quadColor, const int& entityOwnerID) const
    {
        // define the 4 corners
        auto tileSize = miniTileset.getSpriteSize();
        auto start = sf::Vector2f(tile.position.x * tileSize.x, tile.position.y* tileSize.y);

        quadPtr[0].position = sf::Vector2f(start.x, start.y);
        quadPtr[1].position = sf::Vector2f(start.x + tileSize.x, start.y);
        quadPtr[2].position = sf::Vector2f(start.x + tileSize.x, start.y + tileSize.y);
        quadPtr[3].position = sf::Vector2f(start.x, start.y + tileSize.y);

        // define the 4 texture coordinates
        sf::Rect<float> rect;
        if(entityOwnerID == -2)
            rect = miniTileset.getSpriteRect(miniTileSpritePaths.at(tile.tileTypeID));
        else
            rect = miniTileset.getSpriteRect(miniTileSpritePaths.at(-2));

        quadPtr[0].texCoords = sf::Vector2f(rect.left, rect.top);
        quadPtr[1].texCoords = sf::Vector2f(rect.left + rect.width, rect.top);
        quadPtr[2].texCoords = sf::Vector2f(rect.left + rect.width, rect.top + rect.height);
        quadPtr[3].texCoords = sf::Vector2f(rect.left, rect.top + rect.height);

        // define the 4 colors
        quadPtr[0].color = quadColor;
        quadPtr[1].color = quadColor;
        quadPtr[2].color = quadColor;
        quadPtr[3].color = quadColor;
    }

    void MiniTileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        // apply the transform
        states.transform *= getTransform();

        // apply the tileset texture
        states.texture = &miniTileset.getAtlasTexture();

        // draw the vertex array
        target.draw(m_vertices, states);
    }
}

