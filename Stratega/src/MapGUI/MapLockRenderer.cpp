#include <Stratega/MapGUI/MapLockRenderer.h>
#include <Stratega/Configuration/RenderConfig.h>
#include <Stratega/Configuration/GameConfig.h>

#include <Stratega/GUI/GridUtils.h>

namespace SGA
{
    MapLockRenderer::MapLockRenderer()
            : atlas()
    {
    }

    void MapLockRenderer::init()
    {
        // Initialise atlas
        std::vector<std::string> filePaths;
        filePaths.emplace_back("../MapBuilder/Assets/lockedTile.png");
        lockSpritePaths.emplace(1, "../MapBuilder/Assets/lockedTile.png");

        atlas.init(filePaths);

        // Initialise vertex array
        vertices.setPrimitiveType(sf::Quads);

    }

    void MapLockRenderer::update(const std::vector<Vector2f>& locked)
    {
        // Ensure the vertex array has the correct size
        vertices.resize(4 * locked.size());

        for(size_t i = 0; i < locked.size(); i++)
        {
            const auto& lock = locked.at(i);
            auto* quadPtr = &vertices[i * 4];

            // define the 4 corners
            auto spriteSize = atlas.getSpriteSize();
            auto start = toISO(lock.x, lock.y) - sf::Vector2f{ TILE_ORIGIN_X, TILE_ORIGIN_Y };
            quadPtr[0].position = sf::Vector2f(start.x, start.y);
            quadPtr[1].position = sf::Vector2f(start.x + spriteSize.x, start.y);
            quadPtr[2].position = sf::Vector2f(start.x + spriteSize.x, start.y + spriteSize.y);
            quadPtr[3].position = sf::Vector2f(start.x, start.y + spriteSize.y);

            // define the 4 texture coordinates
            auto rect = atlas.getSpriteRect(lockSpritePaths.at(1));
            quadPtr[0].texCoords = sf::Vector2f(rect.left, rect.top);
            quadPtr[1].texCoords = sf::Vector2f(rect.left + rect.width, rect.top);
            quadPtr[2].texCoords = sf::Vector2f(rect.left + rect.width, rect.top + rect.height);
            quadPtr[3].texCoords = sf::Vector2f(rect.left, rect.top + rect.height);

            // define the 4 colors
            quadPtr[0].color = sf::Color::Red;
            quadPtr[1].color = sf::Color::Red;
            quadPtr[2].color = sf::Color::Red;
            quadPtr[3].color = sf::Color::Red;
        }
    }

    void MapLockRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        // apply the transform
        states.transform *= getTransform();

        // apply the tileset texture
        states.texture = &atlas.getAtlasTexture();

        // draw the vertex array
        target.draw(vertices, states);
    }
}
