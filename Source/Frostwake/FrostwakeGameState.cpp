#include "FrostwakeGameState.h"
#include "ActionSystem/FrostwakeMatchSubsystem.h"
#include "Net/UnrealNetwork.h"

AFrostwakeGameState::AFrostwakeGameState()
    : MatchPhase(EFrostwakeMatchPhase::WaitingForPlayers)
    , BuildNumber(0)
    , MatchTimeRemainingSeconds(0.0f)
    , WinningTeam(EFrostwakeTeam::Unassigned)
    , RouteProgress(0.0f)
    , bFurnaceLit(false)
    , bHelmManned(false)
{
    InitializeShipSystems();
}

void AFrostwakeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFrostwakeGameState, MatchPhase);
    DOREPLIFETIME(AFrostwakeGameState, MatchId);
    DOREPLIFETIME(AFrostwakeGameState, BuildNumber);
    DOREPLIFETIME(AFrostwakeGameState, MatchTimeRemainingSeconds);
    DOREPLIFETIME(AFrostwakeGameState, WinningTeam);
    DOREPLIFETIME(AFrostwakeGameState, MatchEndReason);
    DOREPLIFETIME(AFrostwakeGameState, ShipSystems);
    DOREPLIFETIME(AFrostwakeGameState, RouteProgress);
    DOREPLIFETIME(AFrostwakeGameState, bFurnaceLit);
    DOREPLIFETIME(AFrostwakeGameState, bHelmManned);
}

bool AFrostwakeGameState::IsMatchActive() const
{
    return MatchPhase == EFrostwakeMatchPhase::InProgress || MatchPhase == EFrostwakeMatchPhase::FinalApproach;
}

bool AFrostwakeGameState::GetShipSystemStatus(EFrostwakeShipSystem System, FFrostwakeShipSystemStatus& OutStatus) const
{
    for (const FFrostwakeShipSystemStatus& Status : ShipSystems)
    {
        if (Status.System == System)
        {
            OutStatus = Status;
            return true;
        }
    }

    return false;
}

float AFrostwakeGameState::GetFloodingPressure() const
{
    FFrostwakeShipSystemStatus FloodingStatus;
    if (!GetShipSystemStatus(EFrostwakeShipSystem::Flooding, FloodingStatus))
    {
        return 0.0f;
    }

    return FMath::Clamp(1.0f - FloodingStatus.Condition, 0.0f, 1.0f);
}

bool AFrostwakeGameState::HasFatalShipSystem() const
{
    for (const FFrostwakeShipSystemStatus& Status : ShipSystems)
    {
        if (Status.Condition <= 0.0f)
        {
            return true;
        }
    }

    return false;
}

void AFrostwakeGameState::SetMatchPhase(EFrostwakeMatchPhase NewPhase)
{
    if (HasAuthority())
    {
        MatchPhase = NewPhase;
        // Decoupling spine (plan §9.1 step 7): self-registered systems observe phase changes through the
        // match hub instead of the GameMode hard-referencing each of them.
        if (UFrostwakeMatchSubsystem* MatchSubsystem = UFrostwakeMatchSubsystem::Get(this))
        {
            MatchSubsystem->NotifyMatchPhaseChanged(NewPhase);
        }
    }
}

void AFrostwakeGameState::SetMatchId(const FString& NewMatchId)
{
    if (HasAuthority())
    {
        MatchId = NewMatchId;
    }
}

void AFrostwakeGameState::SetMatchTimeRemaining(float NewTimeRemainingSeconds)
{
    if (HasAuthority())
    {
        MatchTimeRemainingSeconds = NewTimeRemainingSeconds;
    }
}

void AFrostwakeGameState::SetMatchResult(EFrostwakeTeam NewWinningTeam, const FString& EndReason)
{
    if (HasAuthority())
    {
        WinningTeam = NewWinningTeam;
        MatchEndReason = EndReason;
        MatchPhase = EFrostwakeMatchPhase::MatchEnded;
        // Resolve the match on the spine: Crew completing the voyage are the "explorers" who escaped.
        if (UFrostwakeMatchSubsystem* MatchSubsystem = UFrostwakeMatchSubsystem::Get(this))
        {
            MatchSubsystem->NotifyMatchEnded(NewWinningTeam == EFrostwakeTeam::Crew);
            MatchSubsystem->NotifyMatchPhaseChanged(EFrostwakeMatchPhase::MatchEnded);
        }
    }
}

void AFrostwakeGameState::SetFurnaceLit(bool bLit)
{
    if (HasAuthority())
    {
        bFurnaceLit = bLit;
    }
}

void AFrostwakeGameState::SetHelmManned(bool bManned)
{
    if (HasAuthority())
    {
        bHelmManned = bManned;
    }
}

void AFrostwakeGameState::SetShipSystemStatus(EFrostwakeShipSystem System, float Condition, bool bOffline, bool bSabotaged)
{
    if (!HasAuthority())
    {
        return;
    }

    for (FFrostwakeShipSystemStatus& Status : ShipSystems)
    {
        if (Status.System == System)
        {
            Status.Condition = FMath::Clamp(Condition, 0.0f, 1.0f);
            Status.bOffline = bOffline;
            Status.bSabotaged = bSabotaged;
            return;
        }
    }

    FFrostwakeShipSystemStatus NewStatus;
    NewStatus.System = System;
    NewStatus.Condition = FMath::Clamp(Condition, 0.0f, 1.0f);
    NewStatus.bOffline = bOffline;
    NewStatus.bSabotaged = bSabotaged;
    ShipSystems.Add(NewStatus);
}

float AFrostwakeGameState::AddRouteProgress(float Delta)
{
    if (!HasAuthority())
    {
        return RouteProgress;
    }

    RouteProgress = FMath::Clamp(RouteProgress + FMath::Max(0.0f, Delta), 0.0f, 1.0f);
    return RouteProgress;
}

void AFrostwakeGameState::OnRep_MatchPhase()
{
    // Intentionally empty: the HUD reads the phase by polling each tick (UFrostwakeHudWidget::NativeTick),
    // so no event-driven client work is needed here. If a one-shot client reaction is ever wanted (a sound
    // or transition on phase change), fire it from here.
}

void AFrostwakeGameState::OnRep_ShipSystems()
{
    // Intentionally empty: the ship-system gauges are polled by the HUD each tick (see OnRep_MatchPhase).
}

void AFrostwakeGameState::InitializeShipSystems()
{
    ShipSystems.Reset();

    const EFrostwakeShipSystem DefaultSystems[] = {
        EFrostwakeShipSystem::Hull,
        EFrostwakeShipSystem::Fuel,
        EFrostwakeShipSystem::Engine,
        EFrostwakeShipSystem::Power,
        EFrostwakeShipSystem::Radio,
        EFrostwakeShipSystem::Route,
        EFrostwakeShipSystem::Heat,
        EFrostwakeShipSystem::Flooding,
    };

    for (const EFrostwakeShipSystem System : DefaultSystems)
    {
        FFrostwakeShipSystemStatus Status;
        Status.System = System;
        Status.Condition = 1.0f;
        ShipSystems.Add(Status);
    }
}
