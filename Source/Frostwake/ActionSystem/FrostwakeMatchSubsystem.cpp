#include "ActionSystem/FrostwakeMatchSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

void UFrostwakeMatchSubsystem::NotifyMatchPhaseChanged(EFrostwakeMatchPhase NewPhase)
{
	if (NewPhase == CurrentPhase)
	{
		return;
	}
	const EFrostwakeMatchPhase OldPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	OnMatchPhaseChanged.Broadcast(NewPhase, OldPhase);
}

void UFrostwakeMatchSubsystem::NotifyMatchEnded(bool bExplorersWon)
{
	OnMatchEnded.Broadcast(bExplorersWon);
}

void UFrostwakeMatchSubsystem::NotifyPlayerDied(AController* Player)
{
	OnPlayerDied.Broadcast(Player);
}

UFrostwakeMatchSubsystem* UFrostwakeMatchSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine || !WorldContext)
	{
		return nullptr;
	}
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull))
	{
		return World->GetSubsystem<UFrostwakeMatchSubsystem>();
	}
	return nullptr;
}
