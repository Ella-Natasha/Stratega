#pragma once
#include <random>
#include <Stratega/Representation/GameState.h>
#include <Stratega/ForwardModel/EntityActionSpace.h>

#include "Condition.h"
#include "Effect.h"

namespace SGA
{
	enum class WinConditionType
	{
		LastManStanding,
		UnitAlive
	};
	
	struct OnTickEffect
	{
		std::unordered_set<EntityTypeID> validTargets;
		std::vector<std::shared_ptr<Condition>> conditions;
		std::vector<std::shared_ptr<Effect>> effects;
	};

	struct OnEntitySpawnEffect
	{
		std::unordered_set<EntityTypeID> validTargets;
		std::vector<std::shared_ptr<Condition>> conditions;
		std::vector<std::shared_ptr<Effect>> effects;
	};

	class RTSAction;
	
	class EntityForwardModel
	{
	public:
		std::shared_ptr<EntityActionSpace> actionSpace;
		std::vector<OnTickEffect> onTickEffects;
		std::vector<OnEntitySpawnEffect> onEntitySpawnEffects;

		//Conditions
		std::vector<std::vector<std::shared_ptr<Condition>>> winConditions;
		std::vector<std::vector<std::shared_ptr<Condition>>> loseConditions;
		
		std::vector<std::pair<TargetType, std::vector<std::shared_ptr<Condition>>>> actionTargets;
		
		//RTS
		float deltaTime;
		
		virtual ~EntityForwardModel() = default;
		EntityForwardModel()
			: deltaTime(1. / 60.)
		{
			
		}

		//Advance game
		virtual void advanceGameState(GameState& state, const RTSAction& action) const;
		virtual void advanceGameState(GameState& state, const Action& action) const;

		virtual std::vector<Action> generateActions(GameState& state) const=0;
		virtual std::vector<Action> generateActions(GameState& state, int playerID) const=0;

		//bool isValid(const GameState& state, const Action& action) const;

		//Common stuff
		virtual bool checkGameIsFinished(GameState& state) const=0;
		
		bool canPlayerPlay(const GameState& state, Player& player) const;
		bool checkPlayerWon(const GameState& state, Player& player) const;
		void executeAction(GameState& state, const Action& action) const;
		void endTick(GameState& state) const;
		void spawnEntity(GameState& state, const EntityType& entityType, int playerID, const Vector2f& position) const;

		std::unique_ptr<EntityActionSpace> generateDefaultActionSpace()
		{
			return std::make_unique<EntityActionSpace>();
		}
		
		void setActionSpace(std::unique_ptr<EntityActionSpace> actionSpace)
		{
			this->actionSpace = std::move(actionSpace);
		}
	};

}
