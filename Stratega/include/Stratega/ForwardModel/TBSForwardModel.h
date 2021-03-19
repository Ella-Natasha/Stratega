#pragma once
#include <Stratega/ForwardModel/EntityForwardModel.h>

namespace  SGA
{
	struct GameState;
	
	class TBSForwardModel : public EntityForwardModel
	{
	public:
		void advanceGameState(GameState& state, const Action& action) const;
		
		std::vector<Action> generateActions(GameState& state) const override;
		std::vector<Action> generateActions(GameState& state, int playerID) const override;

		bool isValid(const GameState& state, const Action& action) const;
		
		void endTurn(GameState& state) const;
		
		bool checkGameIsFinished(GameState& state) const override;		
	};
}
