#pragma once
#include <SFML/Graphics.hpp>
#include <Stratega/GUI/TextureAtlas.h>
#include <Stratega/Representation/Vector2.h>

namespace SGA
{
    struct GameConfig;
    struct RenderConfig;
    struct Entity;

    class MapLockRenderer : public sf::Drawable, public sf::Transformable
    {
    public:
        MapLockRenderer();
        void init();
        void update(const std::vector<Vector2f>& locked);

    private:
        void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

        sf::VertexArray vertices;
        TextureAtlas atlas;
        std::unordered_map<int, std::string> lockSpritePaths;
    };
}
