#include <string>
#include <Stratega/GUI/AssetCache.h>


void AssetCache::loadTexture(std::string name, std::string fileName) {
	sf::Texture tex;

	if (tex.loadFromFile(fileName))
	{
		this->textures.emplace(name, tex);
	}
	else
	{
		throw std::runtime_error("file not found");
	}
}

void AssetCache::saveTexture(std::string name, sf::Texture texture) {
	this->textures.emplace(name, texture);
}

void AssetCache::updateTexture(std::string name, sf::Texture texture) {
    if (textures.contains(name))
        this->textures.at(name) = texture;
    else
        this->saveTexture(name, texture);
}

void AssetCache::loadFont(std::string name, std::string fileName) {
	sf::Font font;

	if (font.loadFromFile(fileName)) {

		this->fonts.emplace(name, font);
	}
	else
	{
		throw std::runtime_error("file not found");
	}		
}

sf::Texture& AssetCache::getTexture(std::string name) {
	return this->textures.at(name);
}

sf::Font& AssetCache::getFont(std::string name) {
	return this->fonts.at(name);
}

