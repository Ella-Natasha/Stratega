#pragma once
#include <SFML/Graphics.hpp>
#include <Stratega/GUI/TextureAtlas.h>
#include <unordered_map>
#include <Stratega/Representation/Grid2D.h>

namespace SGA
{
    struct GameState;
    struct RenderConfig;
    struct GameConfig;
    struct Tile;

    enum class FogRenderType
    {
        Nothing,
        Fog,
        Tiles
    };

    class EditableTileMap : public sf::Drawable, public sf::Transformable
    {
    public:
        inline static const sf::Color FOG_COLOR = { 144, 161, 168 };

        void init(const GameState& state, const GameConfig& gameConfig, const RenderConfig& renderConfig);
        void update(const GameState& state);
        void update(const Grid2D<Tile>& board);

    private:
        void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
        void updateTileQuad(sf::Vertex* quadPtr, const Tile& tile, const sf::Color& quadColor) const;

        sf::VertexArray m_vertices;
        TextureAtlas tileset;
        std::unordered_map<int, std::string> tileSpritePaths;
    };
}
