#pragma once

#include "CoreMinimal.h"
#include "AbyssLockTypes.h"
#include "GameFramework/GameStateBase.h"
#include "AbyssLockGameState.generated.h"

USTRUCT(BlueprintType)
struct FAbyssShipSystemStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Ship")
    EAbyssShipSystem System = EAbyssShipSystem::Hull;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Ship")
    float Condition = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Ship")
    bool bOffline = false;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Ship")
    bool bSabotaged = false;
};

UCLASS()
class ABYSSLOCK_API AAbyssLockGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AAbyssLockGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Match")
    bool IsMatchActive() const;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Match")
    EAbyssMatchPhase GetMatchPhase() const { return MatchPhase; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Match")
    EAbyssTeam GetWinningTeam() const { return WinningTeam; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Match")
    FString GetMatchEndReason() const { return MatchEndReason; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Ship")
    bool GetShipSystemStatus(EAbyssShipSystem System, FAbyssShipSystemStatus& OutStatus) const;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Ship")
    bool HasFatalShipSystem() const;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Objective")
    float GetRouteProgress() const { return RouteProgress; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Voyage")
    bool IsFurnaceLit() const { return bFurnaceLit; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Voyage")
    bool IsHelmManned() const { return bHelmManned; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Ship")
    float GetFloodingPressure() const;

    void SetMatchPhase(EAbyssMatchPhase NewPhase);
    void SetMatchId(const FString& NewMatchId);
    void SetMatchTimeRemaining(float NewTimeRemainingSeconds);
    void SetMatchResult(EAbyssTeam NewWinningTeam, const FString& EndReason);
    void SetFurnaceLit(bool bLit);
    void SetHelmManned(bool bManned);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Ship")
    void SetShipSystemStatus(EAbyssShipSystem System, float Condition, bool bOffline, bool bSabotaged);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Objective")
    float AddRouteProgress(float Delta);

protected:
    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Abyss|Match")
    EAbyssMatchPhase MatchPhase;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Match")
    FString MatchId;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Match")
    int32 BuildNumber;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Match")
    float MatchTimeRemainingSeconds;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Match")
    EAbyssTeam WinningTeam;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Match")
    FString MatchEndReason;

    UPROPERTY(ReplicatedUsing = OnRep_ShipSystems, BlueprintReadOnly, Category = "Abyss|Ship")
    TArray<FAbyssShipSystemStatus> ShipSystems;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Objective")
    float RouteProgress;

    // Voyage: the furnace is lit (fuel burning) and a player is manning the helm. Both are needed,
    // along with Fuel-system condition > 0, for the ship to advance — it never advances on its own.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Voyage")
    bool bFurnaceLit;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Voyage")
    bool bHelmManned;

    UFUNCTION()
    void OnRep_MatchPhase();

    UFUNCTION()
    void OnRep_ShipSystems();

private:
    void InitializeShipSystems();
};
