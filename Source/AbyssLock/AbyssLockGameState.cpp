#include "AbyssLockGameState.h"
#include "Net/UnrealNetwork.h"

AAbyssLockGameState::AAbyssLockGameState()
    : MatchPhase(EAbyssMatchPhase::WaitingForPlayers)
    , BuildNumber(0)
    , MatchTimeRemainingSeconds(0.0f)
    , WinningTeam(EAbyssTeam::Unassigned)
    , RouteProgress(0.0f)
{
    InitializeShipSystems();
}

void AAbyssLockGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssLockGameState, MatchPhase);
    DOREPLIFETIME(AAbyssLockGameState, MatchId);
    DOREPLIFETIME(AAbyssLockGameState, BuildNumber);
    DOREPLIFETIME(AAbyssLockGameState, MatchTimeRemainingSeconds);
    DOREPLIFETIME(AAbyssLockGameState, WinningTeam);
    DOREPLIFETIME(AAbyssLockGameState, MatchEndReason);
    DOREPLIFETIME(AAbyssLockGameState, ShipSystems);
    DOREPLIFETIME(AAbyssLockGameState, RouteProgress);
}

bool AAbyssLockGameState::IsMatchActive() const
{
    return MatchPhase == EAbyssMatchPhase::InProgress || MatchPhase == EAbyssMatchPhase::FinalApproach;
}

bool AAbyssLockGameState::GetShipSystemStatus(EAbyssShipSystem System, FAbyssShipSystemStatus& OutStatus) const
{
    for (const FAbyssShipSystemStatus& Status : ShipSystems)
    {
        if (Status.System == System)
        {
            OutStatus = Status;
            return true;
        }
    }

    return false;
}

float AAbyssLockGameState::GetFloodingPressure() const
{
    FAbyssShipSystemStatus FloodingStatus;
    if (!GetShipSystemStatus(EAbyssShipSystem::Flooding, FloodingStatus))
    {
        return 0.0f;
    }

    return FMath::Clamp(1.0f - FloodingStatus.Condition, 0.0f, 1.0f);
}

bool AAbyssLockGameState::HasFatalShipSystem() const
{
    for (const FAbyssShipSystemStatus& Status : ShipSystems)
    {
        if (Status.Condition <= 0.0f)
        {
            return true;
        }
    }

    return false;
}

void AAbyssLockGameState::SetMatchPhase(EAbyssMatchPhase NewPhase)
{
    if (HasAuthority())
    {
        MatchPhase = NewPhase;
    }
}

void AAbyssLockGameState::SetMatchId(const FString& NewMatchId)
{
    if (HasAuthority())
    {
        MatchId = NewMatchId;
    }
}

void AAbyssLockGameState::SetMatchTimeRemaining(float NewTimeRemainingSeconds)
{
    if (HasAuthority())
    {
        MatchTimeRemainingSeconds = NewTimeRemainingSeconds;
    }
}

void AAbyssLockGameState::SetMatchResult(EAbyssTeam NewWinningTeam, const FString& EndReason)
{
    if (HasAuthority())
    {
        WinningTeam = NewWinningTeam;
        MatchEndReason = EndReason;
        MatchPhase = EAbyssMatchPhase::MatchEnded;
    }
}

void AAbyssLockGameState::SetShipSystemStatus(EAbyssShipSystem System, float Condition, bool bOffline, bool bSabotaged)
{
    if (!HasAuthority())
    {
        return;
    }

    for (FAbyssShipSystemStatus& Status : ShipSystems)
    {
        if (Status.System == System)
        {
            Status.Condition = FMath::Clamp(Condition, 0.0f, 1.0f);
            Status.bOffline = bOffline;
            Status.bSabotaged = bSabotaged;
            return;
        }
    }

    FAbyssShipSystemStatus NewStatus;
    NewStatus.System = System;
    NewStatus.Condition = FMath::Clamp(Condition, 0.0f, 1.0f);
    NewStatus.bOffline = bOffline;
    NewStatus.bSabotaged = bSabotaged;
    ShipSystems.Add(NewStatus);
}

float AAbyssLockGameState::AddRouteProgress(float Delta)
{
    if (!HasAuthority())
    {
        return RouteProgress;
    }

    RouteProgress = FMath::Clamp(RouteProgress + FMath::Max(0.0f, Delta), 0.0f, 1.0f);
    return RouteProgress;
}

void AAbyssLockGameState::OnRep_MatchPhase()
{
}

void AAbyssLockGameState::OnRep_ShipSystems()
{
}

void AAbyssLockGameState::InitializeShipSystems()
{
    ShipSystems.Reset();

    const EAbyssShipSystem DefaultSystems[] = {
        EAbyssShipSystem::Hull,
        EAbyssShipSystem::Fuel,
        EAbyssShipSystem::Engine,
        EAbyssShipSystem::Power,
        EAbyssShipSystem::Radio,
        EAbyssShipSystem::Route,
        EAbyssShipSystem::Heat,
        EAbyssShipSystem::Flooding,
    };

    for (const EAbyssShipSystem System : DefaultSystems)
    {
        FAbyssShipSystemStatus Status;
        Status.System = System;
        Status.Condition = 1.0f;
        ShipSystems.Add(Status);
    }
}
