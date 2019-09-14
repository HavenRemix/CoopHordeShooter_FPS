// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"


enum class EWaveState : uint8;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilled, AActor*, VictimActor, AActor*, KillerActor, AController*, KillerController);


/**
 * 
 */
UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:

// ------- VARIABLES ------- \\

//Timer Handles

	FTimerHandle TimerHandle_BotSpawner;

	FTimerHandle TimerHandle_NextWaveStart;

//Int32

	// Bots to spawn in current wave
	int32 NrOfBotsToSpawn;

	UPROPERTY(BlueprintReadOnly, Category = "GameMode")
	int32 WaveCount;

//Float

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	float TimeBetweenWaves;
	
protected:

// ------- FUNCTIONS ------- \\

	// Hook for BP to spawn a single bot
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	void SpawnBotTimerElapsed();

	// Start Spawning Bots
	void StartWave();

	// Stop Spawning Bots
	void EndWave();

	// Set timer for next startwave
	void PrepareForNextWave();

	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void GameOver();

	void SetWaveState(EWaveState NewState);

	void RestartDeadPlayers();

public:

	ASGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category = "GameMode")
	FOnActorKilled OnActorKilled;
};
