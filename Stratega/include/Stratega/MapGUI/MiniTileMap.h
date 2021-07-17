#pragma once
#include <SFML/Graphics.hpp>
#include <Stratega/GUI/TextureAtlas.h>
#include <unordered_map>
#include <Stratega/Representation/Grid2D.h>
#include <Stratega/Representation/Entity.h>

namespace SGA
{
    struct GameState;
    struct RenderConfig;
    struct GameConfig;
    struct Tile;

    class MiniTileMap : public sf::Drawable, public sf::Transformable, public sf::Texture
    {
    public:
        inline static const std::vector<sf::Color> ENTITY_COLORS = {
                sf::Color::Red,
                sf::Color::Blue,
                sf::Color{0, 128, 128}, // Teal
                sf::Color{128,0,128}, // Purple
                sf::Color::Yellow,
                sf::Color{255,165,0}, // Orange
                sf::Color::Green,
                sf::Color{255,192,203}, // Pink
                sf::Color{128,128,128}, // Grey
                sf::Color{173,216,230}, // Light Blue
                sf::Color{0,100,0}, // Dark Green
                sf::Color{139,69,19} // Brown
        };

        void init(const GameState& state, const GameConfig& gameConfig, const RenderConfig& renderConfig);
        void update(const Grid2D<Tile>& board, const std::vector<Entity>& entities);

    private:
        void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
        void updateTileQuad(sf::Vertex* quadPtr, const Tile& tile, const sf::Color& quadColor, const int& entityOwnerID) const;

        sf::VertexArray m_vertices;
        TextureAtlas miniTileset;
        std::unordered_map<int, std::string> miniTileSpritePaths;
    };
}

