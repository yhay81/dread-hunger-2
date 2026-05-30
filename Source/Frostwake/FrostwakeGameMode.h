#pragma once

#include "CoreMinimal.h"
#include "FrostwakeTypes.h"
#include "GameFramework/GameModeBase.h"
#include "FrostwakeGameMode.generated.h"

UCLASS()
class FROSTWAKE_API AFrostwakeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AFrostwakeGameMode();

    virtual void BeginPlay() override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Match")
    void AssignRolesForCurrentPlayers();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Match")
    bool EvaluateMatchEnd();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Lobby")
    bool TryStartMatchFromReady();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Lobby")
    bool TryStartMatchFromHost(APlayerController* RequestingPlayer);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|SinglePlayer")
    bool TryStartSoloMatchFromMenu(APlayerController* RequestingPlayer);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Practice")
    bool TryStartPracticeMatch();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|SinglePlayer")
    bool TryStartSinglePlayerMatch();

    // Active match configuration (mode + difficulty + resolved knobs). Read-only for clients.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Match")
    FFrostwakeMatchConfig GetActiveMatchConfig() const { return ActiveMatchConfig; }

    // Host-side configuration entry point: set before starting the match (e.g. from the host lobby UI).
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Match")
    void SetActiveMatchConfig(const FFrostwakeMatchConfig& NewConfig);

    // Build a config from a mode + difficulty preset (fills auto counts and tuning multipliers).
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Match")
    FFrostwakeMatchConfig ResolveMatchConfig(EFrostwakeMatchMode Mode, EFrostwakeDifficulty Difficulty) const;

private:
    bool bDevAutoStarted = false;
    bool bSoloUrlRequested = false;
    FFrostwakeMatchConfig ActiveMatchConfig;
    FTimerHandle MatchTimerHandle;
    float MatchDurationSeconds = 30.0f * 60.0f;
    double MatchEndWorldSeconds = 0.0;
    float LastLoggedRouteProgress = 0.0f; // throttles voyage_progress telemetry
    bool bVoyageStalledLogged = false;     // one-shot ship_stalled log per stall

    void AssignRolesForCurrentPlayersInternal(int32 SaboteurCountOverride, int32 MadmanCount = 0);
    void TryAutoStartMatchForDev();
    void StartMatchTimer();
    void StopMatchTimer();
    void HandleMatchTimerTick();
    void RunDevSmokeTaskInteraction();
    void RunDevSmokeDownRescue();
    void RunDevSmokeContainment();
    void RunDevSmokeRouteComplete();
    void RunDevSmokeFatalSabotage();
    void RunDevSmokeBulkheadLock();
    void RunDevSmokePumpFlooding();
    void RunDevSmokePveEnemy();
    void RunDevSmokeItemDrop();
    void RunDevSmokeCombinedSystems();
    void RunDevSmokeLifeAction();
    void RunDevSmokeQaBot();
    void RunDevSmokeQaPlayerBot();
    void RunDevSmokeQaTaskBot();
    APawn* FindPawnForTeam(EFrostwakeTeam Team) const;
    bool IsPracticeModeEnabled() const;
    bool IsSinglePlayerModeEnabled() const;
    int32 CalculateSaboteurCount(int32 PlayerCount) const;
    int32 GetRequiredCrewSurvivors(int32 PlayerCount) const;
    int32 GetSaboteurEliminationThreshold(int32 PlayerCount) const;
    // Returns a copy of a `{...}` telemetry JSON object with "mode"/"difficulty" fields appended.
    FString AppendMatchConfigTelemetry(const FString& JsonObject) const;
    // Voyage tick (called each second): while underway (fueled), burn fuel and advance the route;
    // flip to FinalApproach when the route is complete. This is the ~30-min objective arc.
    void TickVoyage(class AFrostwakeGameState& FrostwakeGameState);

    // Decoupling-spine subscriber (plan §9.1 step 7): the match hub's OnPlayerDied calls this so a player
    // going down re-evaluates the match end immediately (event-driven), not only on the next 1s timer poll.
    UFUNCTION()
    void HandleSpinePlayerDied(AController* Player);
};
