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
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Match")
    void AssignRolesForCurrentPlayers();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Match")
    bool EvaluateMatchEnd();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Lobby")
    bool TryStartMatchFromReady();

private:
    bool bDevAutoStarted = false;
    FTimerHandle MatchTimerHandle;
    float MatchDurationSeconds = 25.0f * 60.0f;
    double MatchEndWorldSeconds = 0.0;

    void AssignRolesForCurrentPlayersInternal(int32 SaboteurCountOverride);
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
    int32 CalculateSaboteurCount(int32 PlayerCount) const;
    int32 GetRequiredCrewSurvivors(int32 PlayerCount) const;
    int32 GetSaboteurEliminationThreshold(int32 PlayerCount) const;
};
