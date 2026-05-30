#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FrostwakeTypes.h"
#include "FrostwakeMatchSubsystem.generated.h"

class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFrostwakeMatchPhaseChangedSignature,
	EFrostwakeMatchPhase, NewPhase, EFrostwakeMatchPhase, OldPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFrostwakeMatchEndedSignature, bool, bExplorersWon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFrostwakePlayerDiedSignature, AController*, Player);

/**
 * Decoupling spine (plan §9.1-4). A world-scoped, server-authoritative event hub so gameplay
 * systems SELF-REGISTER for match events instead of the GameMode hard-referencing each of them.
 *
 * Wave 1 systems subscribe to OnMatchPhaseChanged / OnMatchEnded / OnPlayerDied in their BeginPlay;
 * the authority (GameMode / GameState) calls the Notify* entry points. This lets Wave 1 land new
 * systems WITHOUT editing the shared FrostwakeGameMode — the one-line GameMode call that drives the
 * hub is the only shared-file touch and is left for the lead to wire (reported, not done here).
 *
 * Authoritative/server-side hub: client-side mirroring of phase continues to use GameState replication.
 */
UCLASS()
class FROSTWAKE_API UFrostwakeMatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Broadcast when the match phase advances. */
	UPROPERTY(BlueprintAssignable, Category="Frostwake|Match")
	FFrostwakeMatchPhaseChangedSignature OnMatchPhaseChanged;

	/** Broadcast once when the match resolves (true = explorers/crew escaped; false = saboteurs won). */
	UPROPERTY(BlueprintAssignable, Category="Frostwake|Match")
	FFrostwakeMatchEndedSignature OnMatchEnded;

	/** Broadcast when a player dies (for systems that react to deaths without touching the character). */
	UPROPERTY(BlueprintAssignable, Category="Frostwake|Match")
	FFrostwakePlayerDiedSignature OnPlayerDied;

	/** Authority entry point: set the current phase and notify subscribers. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Match")
	void NotifyMatchPhaseChanged(EFrostwakeMatchPhase NewPhase);

	/** Authority entry point: resolve the match and notify subscribers. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Match")
	void NotifyMatchEnded(bool bExplorersWon);

	/** Authority entry point: a player died. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Match")
	void NotifyPlayerDied(AController* Player);

	UFUNCTION(BlueprintCallable, Category="Frostwake|Match")
	EFrostwakeMatchPhase GetCurrentPhase() const { return CurrentPhase; }

	/** Convenience accessor from any world context object. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Match", meta=(WorldContext="WorldContext"))
	static UFrostwakeMatchSubsystem* Get(const UObject* WorldContext);

private:
	EFrostwakeMatchPhase CurrentPhase = EFrostwakeMatchPhase::WaitingForPlayers;
};
