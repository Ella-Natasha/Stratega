#include <OverlayLayer.h>

TBSOverlayLayer::TBSOverlayLayer(AssetCache& assetCache, std::vector<SGA::TBSAction>& actionSpace, SGA::Vector2f& currentMousePos) :
	RenderLayer(assetCache), actionSpace(actionSpace), currentMousePos(currentMousePos)
{

}

void TBSOverlayLayer::init(SGA::TBSGameState& state)
{
	

}

void TBSOverlayLayer::update(SGA::TBSGameState& state)
{
	//Clean previous sprites
	circleShapes.clear();
	sprites.clear();

	//Add actions if we have actions to draw
	if(actionSpace.size()>0)
	{
		for(const auto& action : actionSpace)
		{
			sf::CircleShape shape(15);
			sf::Vector2f temp = toISO(action.targetPosition.x, action.targetPosition.y);
		
			shape.setPosition(temp+sf::Vector2f(TILE_OFFSET_ORIGIN_X,TILE_OFFSET_ORIGIN_Y));
			switch (action.type)
			{
				case SGA::TBSActionType::Attack: shape.setFillColor(sf::Color::Red); break;
				case SGA::TBSActionType::Move:  break;
				case SGA::TBSActionType::Heal: shape.setFillColor(sf::Color::Green);  break;
				case SGA::TBSActionType::Push: shape.setFillColor(sf::Color::Black);  break;
				case SGA::TBSActionType::EndTurn:  shape.setFillColor(sf::Color::Magenta); break;
				default: throw std::runtime_error("Tried adding an action with an not supported action-type");
			}
			circleShapes.emplace_back(shape);
		}		
	}

	//Add selected tile
	sf::Vector2i mouseGridPos = toGrid(sf::Vector2f(currentMousePos.x, currentMousePos.y));
	
	if(state.isInBounds(SGA::Vector2i(mouseGridPos.x,mouseGridPos.y)))
	{
		sf::Texture& texture = assetCache.getTexture("selected");
		sf::Vector2f origin(TILE_ORIGIN_X, TILE_ORIGIN_Y);
		sf::Sprite selectedTile(texture);
	
		selectedTile.setPosition(toISO(mouseGridPos.x, mouseGridPos.y));
		selectedTile.setOrigin(origin);
		sprites.emplace_back(selectedTile);
	}
	
}

void TBSOverlayLayer::draw(SGA::TBSGameState& state, sf::RenderWindow& window) const
{
	for (const auto& circleShape : circleShapes)
	{
		window.draw(circleShape);
	}

	for (const auto& sprite : sprites)
	{
		window.draw(sprite);
	}
}