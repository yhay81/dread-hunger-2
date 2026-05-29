#pragma once

#include "CoreMinimal.h"
#include "AbyssLockTypes.h"
#include "GameFramework/GameModeBase.h"
#include "AbyssLockGameMode.generated.h"

UCLASS()
class ABYSSLOCK_API AAbyssLockGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAbyssLockGameMode();

    virtual void BeginPlay() override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Match")
    void AssignRolesForCurrentPlayers();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Match")
    bool EvaluateMatchEnd();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Lobby")
    bool TryStartMatchFromReady();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Lobby")
    bool TryStartMatchFromHost(APlayerController* RequestingPlayer);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|SinglePlayer")
    bool TryStartSoloMatchFromMenu(APlayerController* RequestingPlayer);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Practice")
    bool TryStartPracticeMatch();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|SinglePlayer")
    bool TryStartSinglePlayerMatch();

    // Active match configuration (mode + difficulty + resolved knobs). Read-only for clients.
    UFUNCTION(BlueprintCallable, Category = "Abyss|Match")
    FAbyssMatchConfig GetActiveMatchConfig() const { return ActiveMatchConfig; }

    // Host-side configuration entry point: set before starting the match (e.g. from the host lobby UI).
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Match")
    void SetActiveMatchConfig(const FAbyssMatchConfig& NewConfig);

    // Build a config from a mode + difficulty preset (fills auto counts and tuning multipliers).
    UFUNCTION(BlueprintCallable, Category = "Abyss|Match")
    FAbyssMatchConfig ResolveMatchConfig(EAbyssMatchMode Mode, EAbyssDifficulty Difficulty) const;

private:
    bool bDevAutoStarted = false;
    bool bSoloUrlRequested = false;
    FAbyssMatchConfig ActiveMatchConfig;
    FTimerHandle MatchTimerHandle;
    float MatchDurationSeconds = 25.0f * 60.0f;
    double MatchEndWorldSeconds = 0.0;

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
    APawn* FindPawnForTeam(EAbyssTeam Team) const;
    bool IsPracticeModeEnabled() const;
    bool IsSinglePlayerModeEnabled() const;
    int32 CalculateSaboteurCount(int32 PlayerCount) const;
    int32 GetRequiredCrewSurvivors(int32 PlayerCount) const;
    int32 GetSaboteurEliminationThreshold(int32 PlayerCount) const;
};
