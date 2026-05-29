#pragma once

#include "CoreMinimal.h"
#include "FrostwakeTypes.h"
#include "GameFramework/GameStateBase.h"
#include "FrostwakeGameState.generated.h"

USTRUCT(BlueprintType)
struct FFrostwakeShipSystemStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Ship")
    EFrostwakeShipSystem System = EFrostwakeShipSystem::Hull;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Ship")
    float Condition = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Ship")
    bool bOffline = false;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Ship")
    bool bSabotaged = false;
};

UCLASS()
class FROSTWAKE_API AFrostwakeGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AFrostwakeGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Match")
    bool IsMatchActive() const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Match")
    EFrostwakeMatchPhase GetMatchPhase() const { return MatchPhase; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Match")
    EFrostwakeTeam GetWinningTeam() const { return WinningTeam; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Match")
    FString GetMatchEndReason() const { return MatchEndReason; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Ship")
    bool GetShipSystemStatus(EFrostwakeShipSystem System, FFrostwakeShipSystemStatus& OutStatus) const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Ship")
    bool HasFatalShipSystem() const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Objective")
    float GetRouteProgress() const { return RouteProgress; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Voyage")
    bool IsFurnaceLit() const { return bFurnaceLit; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Voyage")
    bool IsHelmManned() const { return bHelmManned; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Ship")
    float GetFloodingPressure() const;

    void SetMatchPhase(EFrostwakeMatchPhase NewPhase);
    void SetMatchId(const FString& NewMatchId);
    void SetMatchTimeRemaining(float NewTimeRemainingSeconds);
    void SetMatchResult(EFrostwakeTeam NewWinningTeam, const FString& EndReason);
    void SetFurnaceLit(bool bLit);
    void SetHelmManned(bool bManned);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Ship")
    void SetShipSystemStatus(EFrostwakeShipSystem System, float Condition, bool bOffline, bool bSabotaged);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Objective")
    float AddRouteProgress(float Delta);

protected:
    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Frostwake|Match")
    EFrostwakeMatchPhase MatchPhase;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Match")
    FString MatchId;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Match")
    int32 BuildNumber;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Match")
    float MatchTimeRemainingSeconds;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Match")
    EFrostwakeTeam WinningTeam;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Match")
    FString MatchEndReason;

    UPROPERTY(ReplicatedUsing = OnRep_ShipSystems, BlueprintReadOnly, Category = "Frostwake|Ship")
    TArray<FFrostwakeShipSystemStatus> ShipSystems;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Objective")
    float RouteProgress;

    // Voyage: the furnace is lit (fuel burning) and a player is manning the helm. Both are needed,
    // along with Fuel-system condition > 0, for the ship to advance — it never advances on its own.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Voyage")
    bool bFurnaceLit;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Voyage")
    bool bHelmManned;

    UFUNCTION()
    void OnRep_MatchPhase();

    UFUNCTION()
    void OnRep_ShipSystems();

private:
    void InitializeShipSystems();
};
