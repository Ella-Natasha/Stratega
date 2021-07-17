#include <Stratega/MapGUI/EditableTileMap.h>
#include <Stratega/GUI/GridUtils.h>
#include <Stratega/Configuration/RenderConfig.h>
#include <Stratega/Configuration/GameConfig.h>
#include <Stratega/Representation/GameState.h>

namespace SGA
{
    void EditableTileMap::init(const GameState& state, const GameConfig& gameConfig, const RenderConfig& renderConfig)
    {
        // Initialise atlas
        std::vector<std::string> filePaths;
        for (const auto& namePathPair : renderConfig.tileSpritePaths)
        {
            filePaths.emplace_back(namePathPair.second);
            tileSpritePaths.emplace(gameConfig.getTileID(namePathPair.first), namePathPair.second);
        }
        tileset.init(filePaths);

        // resize the vertex array to fit the level size
        const auto& board = state.board;
        m_vertices.setPrimitiveType(sf::Quads);
        m_vertices.resize(board.getWidth() * board.getHeight() * 4);
    }

    void EditableTileMap::update(const GameState& state)
    {
        const auto& board = state.board;
//        const auto& fowBoard = fowState.board;
        m_vertices.resize(board.getWidth() * board.getHeight() * 4);

        // populate the vertex array, with one quad per tile
        for (int x = 0; x < board.getWidth(); ++x)
        {
            for (int y = 0; y < board.getHeight(); ++y)
            {
                // get the current tile
                const auto& tile = board[{x, y}];

                // get a pointer to the current tile's quad
                sf::Vertex* quad = &m_vertices[(x + y * board.getWidth()) * 4];

                updateTileQuad(quad, tile, sf::Color::White);
            }
        }
    }

    void EditableTileMap::update(const Grid2D<Tile>& board)
    {
        m_vertices.resize(board.getWidth() * board.getHeight() * 4);

        // populate the vertex array, with one quad per tile
        for (int x = 0; x < board.getWidth(); ++x)
        {
            for (int y = 0; y < board.getHeight(); ++y)
            {
                // get the current tile
                const auto& tile = board[{x, y}];

                // get a pointer to the current tile's quad
                sf::Vertex* quad = &m_vertices[(x + y * board.getWidth()) * 4];
                updateTileQuad(quad, tile, sf::Color::White);

            }
        }
    }

    void EditableTileMap::updateTileQuad(sf::Vertex* quadPtr, const Tile& tile, const sf::Color& quadColor) const
    {
        // define the 4 corners
        auto start = toISO(tile.position.x, tile.position.y) - sf::Vector2f{TILE_ORIGIN_X, TILE_ORIGIN_Y};
        auto tileSize = tileset.getSpriteSize();
        quadPtr[0].position = sf::Vector2f(start.x, start.y);
        quadPtr[1].position = sf::Vector2f(start.x + tileSize.x, start.y);
        quadPtr[2].position = sf::Vector2f(start.x + tileSize.x, start.y + tileSize.y);
        quadPtr[3].position = sf::Vector2f(start.x, start.y + tileSize.y);

        // define the 4 texture coordinates
        auto rect = tileset.getSpriteRect(tileSpritePaths.at(tile.tileTypeID));
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

    void EditableTileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        // apply the transform
        states.transform *= getTransform();

        // apply the tileset texture
        states.texture = &tileset.getAtlasTexture();

        // draw the vertex array
        target.draw(m_vertices, states);
    }
}

