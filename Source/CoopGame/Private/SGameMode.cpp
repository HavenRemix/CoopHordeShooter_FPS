// Fill out your copyright notice in the Description page of Project Settings.

#include "SGameMode.h"
#include "SHealthComponent.h"
#include "SGameState.h"
#include "SPlayerState.h"
#include "TimerManager.h"



ASGameMode::ASGameMode()
{
	//The amoun tof time to wait before the next wave starts
	TimeBetweenWaves = 2.0f;

	GameStateClass = ASGameState::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();

	//Event tick exists
	PrimaryActorTick.bCanEverTick = true;
	//The wait for each tick
	PrimaryActorTick.TickInterval = 1.0f;
}


void ASGameMode::StartWave()
{
	//Increase the wave every time we start a new wave
	WaveCount++;

	//Spawn the amount of bots that it 2 times the wave count
	NrOfBotsToSpawn = 2 * WaveCount;

	//Spawn elay so we dont spawn all the bots at the same time since its not suddle and can cause lag
	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f);

	//Change the state of the wave
	SetWaveState(EWaveState::WaveInProgress);
}


void ASGameMode::EndWave()
{
	//Clear the bot timer so we can recreate when the new wave starts
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);

	//Change the wave state
	SetWaveState(EWaveState::WaitingToComplete);
}


void ASGameMode::PrepareForNextWave()
{
	//Make a timer to set a delay between the waves
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave, TimeBetweenWaves, false);

	//Change the wave state
	SetWaveState(EWaveState::WaitingToStart);

	//Respawn any players that died during the wave
	RestartDeadPlayers();
}


void ASGameMode::CheckWaveState()
{
	//See if the wave delay timer still existes to see if we are in a wave or ready for the next one
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);

	//Only run if we still have to spawn bots or are currently preparing for a wave
	if (NrOfBotsToSpawn > 0 || bIsPreparingForWave)
	{
		//Return early so the rest of the code isn't run
		return;
	}

	//Make a bool to see if bots are still alive, set to false by default just in case
	bool bIsAnyBotAlive = false;

	//Basically see if any bots are still alive on the field
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}

		USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.0f)
		{
			bIsAnyBotAlive = true;
			break;
		}
	}
	 
	//Only run if no bots are still alive
	if (!bIsAnyBotAlive)
	{
		//Change the wave state
		SetWaveState(EWaveState::WaveComplete);

		//Run PrepareForNextWave()
		PrepareForNextWave();
	}
}


void ASGameMode::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			APawn* MyPawn = PC->GetPawn();
			USHealthComponent* HealthComp = Cast<USHealthComponent>(MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));
			if (ensure(HealthComp) && HealthComp->GetHealth() > 0.0f)
			{
				// A player is still alive.
				return;
			}
		}
	}

	// No player alive
	GameOver();
}


void ASGameMode::GameOver()
{
	EndWave();

	// @TODO: Finish up the match, present 'game over' to players.

	SetWaveState(EWaveState::GameOver);

	UE_LOG(LogTemp, Log, TEXT("GAME OVER! Players Died"));
}


void ASGameMode::SetWaveState(EWaveState NewState)
{
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(NewState);
	}
}


void ASGameMode::RestartDeadPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn() == nullptr)
		{
			RestartPlayer(PC);
		}
	}
}


void ASGameMode::StartPlay()
{
	Super::StartPlay();

	PrepareForNextWave();
}


void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckWaveState();
	CheckAnyPlayerAlive();
}

void ASGameMode::SpawnBotTimerElapsed()
{
	SpawnNewBot();

	NrOfBotsToSpawn--;

	if (NrOfBotsToSpawn <= 0)
	{
		EndWave();
	}
}
