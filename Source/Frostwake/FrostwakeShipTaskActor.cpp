#include "FrostwakeShipTaskActor.h"
#include "FrostwakeGameMode.h"
#include "FrostwakeGameState.h"
#include "FrostwakeLog.h"
#include "FrostwakePlayerState.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "FrostwakeShipTaskActor"

namespace
{
FText GetShipSystemInteractionLabel(EFrostwakeShipSystem System)
{
    switch (System)
    {
    case EFrostwakeShipSystem::Hull:
        return LOCTEXT("ShipSystemHull", "Hull");
    case EFrostwakeShipSystem::Fuel:
        return LOCTEXT("ShipSystemFuel", "Fuel");
    case EFrostwakeShipSystem::Engine:
        return LOCTEXT("ShipSystemEngine", "Engine");
    case EFrostwakeShipSystem::Power:
        return LOCTEXT("ShipSystemPower", "Power");
    case EFrostwakeShipSystem::Radio:
        return LOCTEXT("ShipSystemRadio", "Radio");
    case EFrostwakeShipSystem::Route:
        return LOCTEXT("ShipSystemRouteProgress", "Route Progress");
    case EFrostwakeShipSystem::Heat:
        return LOCTEXT("ShipSystemHeat", "Heat");
    case EFrostwakeShipSystem::Flooding:
        return LOCTEXT("ShipSystemFlooding", "Flooding");
    default:
        return LOCTEXT("ShipSystemUnknown", "Ship System");
    }
}
}

AFrostwakeShipTaskActor::AFrostwakeShipTaskActor()
{
    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    MarkerMesh->SetupAttachment(SceneRoot);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MarkerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
    MarkerMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.45f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MarkerMesh->SetStaticMesh(CubeMesh.Object);
    }

    TargetSystem = EFrostwakeShipSystem::Hull;
    TaskMode = EFrostwakeShipTaskMode::Repair;
    ConditionDelta = 0.35f;
    bSingleUse = false;
    bSaboteurOnly = false;
    bSetOfflineOnSabotage = false;
    bCompleted = false;
    InteractionText = FText::FromString(TEXT("Ship Task"));
}

void AFrostwakeShipTaskActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFrostwakeShipTaskActor, bCompleted);
    DOREPLIFETIME(AFrostwakeShipTaskActor, TargetSystem);
    DOREPLIFETIME(AFrostwakeShipTaskActor, TaskMode);
    DOREPLIFETIME(AFrostwakeShipTaskActor, ConditionDelta);
    DOREPLIFETIME(AFrostwakeShipTaskActor, bSingleUse);
    DOREPLIFETIME(AFrostwakeShipTaskActor, bSaboteurOnly);
    DOREPLIFETIME(AFrostwakeShipTaskActor, bSetOfflineOnSabotage);
}

bool AFrostwakeShipTaskActor::CanInteract(const APawn* InstigatorPawn) const
{
    if (!Super::CanInteract(InstigatorPawn) || (bSingleUse && bCompleted))
    {
        return false;
    }

    const AFrostwakeGameState* FrostwakeGameState = GetWorld() ? GetWorld()->GetGameState<AFrostwakeGameState>() : nullptr;
    if (!FrostwakeGameState || !FrostwakeGameState->IsMatchActive())
    {
        return false;
    }

    return IsInstigatorAllowed(InstigatorPawn);
}

bool AFrostwakeShipTaskActor::ConfigureTaskByName(FName InTargetSystem, bool bInSabotage, float InConditionDelta, bool bInSingleUse, bool bInSaboteurOnly, bool bInSetOfflineOnSabotage)
{
    EFrostwakeShipSystem ResolvedSystem = EFrostwakeShipSystem::Hull;
    if (!ResolveShipSystem(InTargetSystem, ResolvedSystem))
    {
        return false;
    }

    TargetSystem = ResolvedSystem;
    TaskMode = bInSabotage ? EFrostwakeShipTaskMode::Sabotage : EFrostwakeShipTaskMode::Repair;
    ConditionDelta = FMath::Clamp(InConditionDelta, 0.0f, 1.0f);
    bSingleUse = bInSingleUse;
    bSaboteurOnly = bInSaboteurOnly;
    bSetOfflineOnSabotage = bInSetOfflineOnSabotage;
    ApplyTaskConfigState();
    return true;
}

bool AFrostwakeShipTaskActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !CanInteract(InstigatorPawn))
    {
        return false;
    }

    AFrostwakeGameState* FrostwakeGameState = GetWorld() ? GetWorld()->GetGameState<AFrostwakeGameState>() : nullptr;
    if (!FrostwakeGameState)
    {
        return false;
    }

    FFrostwakeShipSystemStatus Status;
    if (!FrostwakeGameState->GetShipSystemStatus(TargetSystem, Status))
    {
        return false;
    }

    // Difficulty scales sabotage severity; repairs are unaffected.
    const float SabotageMagnitude = ConditionDelta * (TaskMode == EFrostwakeShipTaskMode::Sabotage ? GetMatchSabotageIntensity() : 1.0f);
    const float SignedDelta = TaskMode == EFrostwakeShipTaskMode::Repair ? ConditionDelta : -SabotageMagnitude;
    const float NewCondition = FMath::Clamp(Status.Condition + SignedDelta, 0.0f, 1.0f);
    const float FloodingPressureBefore = TargetSystem == EFrostwakeShipSystem::Flooding ? FMath::Clamp(1.0f - Status.Condition, 0.0f, 1.0f) : 0.0f;
    bool bNewOffline = Status.bOffline;
    bool bNewSabotaged = Status.bSabotaged;

    if (TaskMode == EFrostwakeShipTaskMode::Repair)
    {
        bNewOffline = false;
        bNewSabotaged = TargetSystem == EFrostwakeShipSystem::Flooding ? NewCondition < 1.0f : false;
    }
    else
    {
        bNewSabotaged = true;
        bNewOffline = bSetOfflineOnSabotage || NewCondition <= 0.0f;
    }

    FrostwakeGameState->SetShipSystemStatus(TargetSystem, NewCondition, bNewOffline, bNewSabotaged);

    // The route is no longer advanced by interacting with a task; it progresses over time while the
    // ship is underway (see AFrostwakeGameMode::TickVoyage). The Route system is now vestigial here.
    if (TargetSystem == EFrostwakeShipSystem::Flooding)
    {
        const float FloodingPressure = FMath::Clamp(1.0f - NewCondition, 0.0f, 1.0f);
        const float PressureDelta = FloodingPressure - FloodingPressureBefore;
        UE_LOG(
            LogFrostwakeGameplay,
            Log,
            TEXT("flooding_pressure_changed task=%s mode=%d pressure=%.2f deltaPressure=%.2f condition=%.2f"),
            *GetNameSafe(this),
            static_cast<int32>(TaskMode),
            FloodingPressure,
            PressureDelta,
            NewCondition);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("flooding_pressure_changed"),
                    FString::Printf(
                        TEXT("{\"task\":\"%s\",\"mode\":%d,\"pressure\":%.3f,\"deltaPressure\":%.3f,\"delta\":%.3f,\"condition\":%.3f,\"pawn\":\"%s\"}"),
                        *GetNameSafe(this),
                        static_cast<int32>(TaskMode),
                        FloodingPressure,
                        PressureDelta,
                        PressureDelta,
                        NewCondition,
                        *GetNameSafe(InstigatorPawn)));
            }
        }
    }

    if (bSingleUse)
    {
        bCompleted = true;
        ApplyCompletedState();
    }

    UE_LOG(
        LogFrostwakeGameplay,
        Log,
        TEXT("ship_task_applied task=%s system=%d mode=%d condition=%.2f pawn=%s"),
        *GetNameSafe(this),
        static_cast<int32>(TargetSystem),
        static_cast<int32>(TaskMode),
        NewCondition,
        *GetNameSafe(InstigatorPawn));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("ship_task_applied"),
                FString::Printf(
                    TEXT("{\"task\":\"%s\",\"system\":%d,\"mode\":%d,\"condition\":%.3f,\"pawn\":\"%s\"}"),
                    *GetNameSafe(this),
                    static_cast<int32>(TargetSystem),
                    static_cast<int32>(TaskMode),
                    NewCondition,
                    *GetNameSafe(InstigatorPawn)));
        }
    }

    if (AFrostwakeGameMode* FrostwakeGameMode = GetWorld()->GetAuthGameMode<AFrostwakeGameMode>())
    {
        FrostwakeGameMode->EvaluateMatchEnd();
    }

    return true;
}

void AFrostwakeShipTaskActor::OnRep_TaskState()
{
    ApplyCompletedState();
}

void AFrostwakeShipTaskActor::OnRep_TaskConfig()
{
    ApplyTaskConfigState();
}

bool AFrostwakeShipTaskActor::IsInstigatorAllowed(const APawn* InstigatorPawn) const
{
    const AController* Controller = InstigatorPawn ? InstigatorPawn->GetController() : nullptr;
    const AFrostwakePlayerState* PlayerState = Controller ? Controller->GetPlayerState<AFrostwakePlayerState>() : nullptr;
    if (!PlayerState || PlayerState->GetLifeState() != EFrostwakeLifeState::Alive)
    {
        return false;
    }

    const EFrostwakeTeam Team = PlayerState->GetSecretTeamForOwner();
    if (Team == EFrostwakeTeam::Unassigned || Team == EFrostwakeTeam::Spectator)
    {
        return false;
    }

    if (bSaboteurOnly || TaskMode == EFrostwakeShipTaskMode::Sabotage)
    {
        return Team == EFrostwakeTeam::Saboteur;
    }

    return true;
}

void AFrostwakeShipTaskActor::ApplyCompletedState()
{
    if (bSingleUse)
    {
        SetActorEnableCollision(!bCompleted);
    }
}

float AFrostwakeShipTaskActor::GetMatchSabotageIntensity() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const AFrostwakeGameMode* GameMode = World->GetAuthGameMode<AFrostwakeGameMode>())
        {
            return FMath::Max(0.0f, GameMode->GetActiveMatchConfig().SabotageIntensityMultiplier);
        }
    }
    return 1.0f;
}

void AFrostwakeShipTaskActor::ApplyTaskConfigState()
{
    if (TaskMode == EFrostwakeShipTaskMode::Repair && TargetSystem == EFrostwakeShipSystem::Route)
    {
        InteractionText = LOCTEXT("AdvanceRouteProgressInteraction", "Advance Route Progress");
        return;
    }

    const FText ActionText = TaskMode == EFrostwakeShipTaskMode::Sabotage
        ? LOCTEXT("SabotageInteractionAction", "Sabotage")
        : LOCTEXT("RepairInteractionAction", "Repair");
    InteractionText = FText::Format(
        LOCTEXT("ShipTaskInteractionFormat", "{0} {1}"),
        ActionText,
        GetShipSystemInteractionLabel(TargetSystem));
}

bool AFrostwakeShipTaskActor::ResolveShipSystem(FName SystemName, EFrostwakeShipSystem& OutSystem)
{
    const FString Normalized = SystemName.ToString().ToLower();
    if (Normalized == TEXT("hull"))
    {
        OutSystem = EFrostwakeShipSystem::Hull;
        return true;
    }
    if (Normalized == TEXT("fuel"))
    {
        OutSystem = EFrostwakeShipSystem::Fuel;
        return true;
    }
    if (Normalized == TEXT("engine"))
    {
        OutSystem = EFrostwakeShipSystem::Engine;
        return true;
    }
    if (Normalized == TEXT("power"))
    {
        OutSystem = EFrostwakeShipSystem::Power;
        return true;
    }
    if (Normalized == TEXT("radio"))
    {
        OutSystem = EFrostwakeShipSystem::Radio;
        return true;
    }
    if (Normalized == TEXT("route"))
    {
        OutSystem = EFrostwakeShipSystem::Route;
        return true;
    }
    if (Normalized == TEXT("heat"))
    {
        OutSystem = EFrostwakeShipSystem::Heat;
        return true;
    }
    if (Normalized == TEXT("flooding"))
    {
        OutSystem = EFrostwakeShipSystem::Flooding;
        return true;
    }

    return false;
}

#undef LOCTEXT_NAMESPACE
