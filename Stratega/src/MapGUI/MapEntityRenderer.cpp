#include <Stratega/MapGUI/MapEntityRenderer.h>
#include <Stratega/Configuration/RenderConfig.h>
#include <Stratega/Configuration/GameConfig.h>

#include <Stratega/GUI/GridUtils.h>

namespace SGA
{
    MapEntityRenderer::MapEntityRenderer()
            : atlas(2)
    {
    }

    void MapEntityRenderer::init(const GameConfig& gameConfig, const RenderConfig& renderConfig)
    {
        // Initialise atlas
        std::vector<std::string> filePaths;
        for (const auto& namePathPair : renderConfig.entitySpritePaths)
        {
            filePaths.emplace_back(namePathPair.second);
            entitySpritePaths.emplace(gameConfig.getEntityID(namePathPair.first), namePathPair.second);
        }

        atlas.init(filePaths);

        // Initialise vertex array
        vertices.setPrimitiveType(sf::Quads);

        // Initialise shader
        outlineShader.loadFromFile(renderConfig.outlineShaderPath, sf::Shader::Fragment);
        outlineShader.setUniform("texture", sf::Shader::CurrentTexture);
        outlineShader.setUniform("outlineThickness", 1.9f);
        outlineShader.setUniform("textureSize", sf::Glsl::Vec2(atlas.getAtlasTexture().getSize()));
    }

    void MapEntityRenderer::update(const std::vector<Entity>& entities)
    {
        // Ensure the vertex array has the correct size
        vertices.resize(4 * entities.size());

        for(size_t i = 0; i < entities.size(); i++)
        {
            const auto& entity = entities.at(i);
            auto* quadPtr = &vertices[i * 4];

            // define the 4 corners
            auto spriteSize = atlas.getSpriteSize();
            auto start = toISO(entity.position.x, entity.position.y) - sf::Vector2f{ spriteSize.x / 4.f, spriteSize.y / 1.4f };
            quadPtr[0].position = sf::Vector2f(start.x, start.y);
            quadPtr[1].position = sf::Vector2f(start.x + spriteSize.x, start.y);
            quadPtr[2].position = sf::Vector2f(start.x + spriteSize.x, start.y + spriteSize.y);
            quadPtr[3].position = sf::Vector2f(start.x, start.y + spriteSize.y);

            // define the 4 texture coordinates
            auto rect = atlas.getSpriteRect(entitySpritePaths.at(entity.typeID));
            quadPtr[0].texCoords = sf::Vector2f(rect.left, rect.top);
            quadPtr[1].texCoords = sf::Vector2f(rect.left + rect.width, rect.top);
            quadPtr[2].texCoords = sf::Vector2f(rect.left + rect.width, rect.top + rect.height);
            quadPtr[3].texCoords = sf::Vector2f(rect.left, rect.top + rect.height);

            quadPtr[0].color = entity.isNeutral() ? sf::Color::Transparent : PLAYER_COLORS[entity.ownerID];
            quadPtr[1].color = entity.isNeutral() ? sf::Color::Transparent : PLAYER_COLORS[entity.ownerID];
            quadPtr[2].color = entity.isNeutral() ? sf::Color::Transparent : PLAYER_COLORS[entity.ownerID];
            quadPtr[3].color = entity.isNeutral() ? sf::Color::Transparent : PLAYER_COLORS[entity.ownerID];
        }
    }

    void MapEntityRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        // apply the transform
        states.transform *= getTransform();

        // apply the tileset texture
        states.texture = &atlas.getAtlasTexture();
        states.shader = &outlineShader;

        // draw the vertex array
        target.draw(vertices, states);
    }
}
