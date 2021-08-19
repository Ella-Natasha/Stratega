#pragma once
#include <Stratega/Representation/Vector2.h>
namespace SGA
{
	struct Tile
	{
		Tile(int typeID, int x, int y):
			tileTypeID(typeID),
			isWalkable(true),
			blocksSight(false),
			locked(false),
			position(x, y)
		{

		}

		int tileTypeID;
		bool isWalkable;
		bool blocksSight;
		bool locked;
		Vector2i position;
	};

}