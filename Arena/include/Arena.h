#pragma once
#include <Stratega/Logging/FileLogger.h>
#include <Stratega/Configuration/GameConfig.h>
#include <Stratega/Game/GameObserver.h>
#include <Stratega/Game/GameRunner.h>

class Arena : public SGA::GameObserver
{
public:
	Arena(const SGA::GameConfig& config);

	void runGames(int playerCount, int seed);
	void runGame(const std::vector<int>& agentAssignment, std::mt19937 rngEngine);

	void onGameStateAdvanced(const SGA::GameState& state, const SGA::EntityForwardModel& forwardModel) override;
	void onGameFinished(const SGA::GameState& finalState, const SGA::EntityForwardModel& forwardModel) override;
	
private:
	const SGA::GameConfig* config;
	std::unique_ptr<SGA::GameRunner> runner;
	int gameCount;
};