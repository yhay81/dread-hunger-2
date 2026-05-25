#include "AbyssShipTaskActor.h"
#include "AbyssLockGameMode.h"
#include "AbyssLockGameState.h"
#include "AbyssLockLog.h"
#include "AbyssLockPlayerState.h"
#include "AbyssTelemetrySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAbyssShipTaskActor::AAbyssShipTaskActor()
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

    TargetSystem = EAbyssShipSystem::Hull;
    TaskMode = EAbyssShipTaskMode::Repair;
    ConditionDelta = 0.35f;
    bSingleUse = false;
    bSaboteurOnly = false;
    bSetOfflineOnSabotage = false;
    bCompleted = false;
    InteractionText = FText::FromString(TEXT("Ship Task"));
}

void AAbyssShipTaskActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssShipTaskActor, bCompleted);
    DOREPLIFETIME(AAbyssShipTaskActor, TargetSystem);
    DOREPLIFETIME(AAbyssShipTaskActor, TaskMode);
    DOREPLIFETIME(AAbyssShipTaskActor, ConditionDelta);
    DOREPLIFETIME(AAbyssShipTaskActor, bSingleUse);
    DOREPLIFETIME(AAbyssShipTaskActor, bSaboteurOnly);
    DOREPLIFETIME(AAbyssShipTaskActor, bSetOfflineOnSabotage);
}

bool AAbyssShipTaskActor::CanInteract(const APawn* InstigatorPawn) const
{
    if (!Super::CanInteract(InstigatorPawn) || (bSingleUse && bCompleted))
    {
        return false;
    }

    const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
    if (!AbyssGameState || !AbyssGameState->IsMatchActive())
    {
        return false;
    }

    return IsInstigatorAllowed(InstigatorPawn);
}

bool AAbyssShipTaskActor::ConfigureTaskByName(FName InTargetSystem, bool bInSabotage, float InConditionDelta, bool bInSingleUse, bool bInSaboteurOnly, bool bInSetOfflineOnSabotage)
{
    EAbyssShipSystem ResolvedSystem = EAbyssShipSystem::Hull;
    if (!ResolveShipSystem(InTargetSystem, ResolvedSystem))
    {
        return false;
    }

    TargetSystem = ResolvedSystem;
    TaskMode = bInSabotage ? EAbyssShipTaskMode::Sabotage : EAbyssShipTaskMode::Repair;
    ConditionDelta = FMath::Clamp(InConditionDelta, 0.0f, 1.0f);
    bSingleUse = bInSingleUse;
    bSaboteurOnly = bInSaboteurOnly;
    bSetOfflineOnSabotage = bInSetOfflineOnSabotage;
    ApplyTaskConfigState();
    return true;
}

bool AAbyssShipTaskActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !CanInteract(InstigatorPawn))
    {
        return false;
    }

    AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
    if (!AbyssGameState)
    {
        return false;
    }

    FAbyssShipSystemStatus Status;
    if (!AbyssGameState->GetShipSystemStatus(TargetSystem, Status))
    {
        return false;
    }

    const float SignedDelta = TaskMode == EAbyssShipTaskMode::Repair ? ConditionDelta : -ConditionDelta;
    const float NewCondition = FMath::Clamp(Status.Condition + SignedDelta, 0.0f, 1.0f);
    const float FloodingPressureBefore = TargetSystem == EAbyssShipSystem::Flooding ? FMath::Clamp(1.0f - Status.Condition, 0.0f, 1.0f) : 0.0f;
    bool bNewOffline = Status.bOffline;
    bool bNewSabotaged = Status.bSabotaged;

    if (TaskMode == EAbyssShipTaskMode::Repair)
    {
        bNewOffline = false;
        bNewSabotaged = TargetSystem == EAbyssShipSystem::Flooding ? NewCondition < 1.0f : false;
    }
    else
    {
        bNewSabotaged = true;
        bNewOffline = bSetOfflineOnSabotage || NewCondition <= 0.0f;
    }

    AbyssGameState->SetShipSystemStatus(TargetSystem, NewCondition, bNewOffline, bNewSabotaged);

    if (TaskMode == EAbyssShipTaskMode::Repair && TargetSystem == EAbyssShipSystem::Route)
    {
        const float RouteProgress = AbyssGameState->AddRouteProgress(ConditionDelta);
        UE_LOG(LogAbyssGameplay, Log, TEXT("route_progress task=%s progress=%.2f"), *GetNameSafe(this), RouteProgress);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("route_progress"),
                    FString::Printf(TEXT("{\"task\":\"%s\",\"progress\":%.3f}"), *GetNameSafe(this), RouteProgress));
            }
        }

        if (RouteProgress >= 1.0f && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress)
        {
            AbyssGameState->SetMatchPhase(EAbyssMatchPhase::FinalApproach);
            UE_LOG(LogAbyssGameplay, Log, TEXT("final_approach_started progress=%.2f"), RouteProgress);
            if (UGameInstance* GameInstance = GetGameInstance())
            {
                if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
                {
                    TelemetrySubsystem->LogEvent(TEXT("final_approach_started"), TEXT("{\"progress\":1.0}"));
                }
            }
        }
    }
    else if (TargetSystem == EAbyssShipSystem::Flooding)
    {
        const float FloodingPressure = FMath::Clamp(1.0f - NewCondition, 0.0f, 1.0f);
        const float PressureDelta = FloodingPressure - FloodingPressureBefore;
        UE_LOG(
            LogAbyssGameplay,
            Log,
            TEXT("flooding_pressure_changed task=%s mode=%d pressure=%.2f deltaPressure=%.2f condition=%.2f"),
            *GetNameSafe(this),
            static_cast<int32>(TaskMode),
            FloodingPressure,
            PressureDelta,
            NewCondition);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
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
        LogAbyssGameplay,
        Log,
        TEXT("ship_task_applied task=%s system=%d mode=%d condition=%.2f pawn=%s"),
        *GetNameSafe(this),
        static_cast<int32>(TargetSystem),
        static_cast<int32>(TaskMode),
        NewCondition,
        *GetNameSafe(InstigatorPawn));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
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

    if (AAbyssLockGameMode* AbyssGameMode = GetWorld()->GetAuthGameMode<AAbyssLockGameMode>())
    {
        AbyssGameMode->EvaluateMatchEnd();
    }

    return true;
}

void AAbyssShipTaskActor::OnRep_TaskState()
{
    ApplyCompletedState();
}

void AAbyssShipTaskActor::OnRep_TaskConfig()
{
    ApplyTaskConfigState();
}

bool AAbyssShipTaskActor::IsInstigatorAllowed(const APawn* InstigatorPawn) const
{
    const AController* Controller = InstigatorPawn ? InstigatorPawn->GetController() : nullptr;
    const AAbyssLockPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AAbyssLockPlayerState>() : nullptr;
    if (!PlayerState || PlayerState->GetLifeState() != EAbyssLifeState::Alive)
    {
        return false;
    }

    const EAbyssTeam Team = PlayerState->GetSecretTeamForOwner();
    if (Team == EAbyssTeam::Unassigned || Team == EAbyssTeam::Spectator)
    {
        return false;
    }

    if (bSaboteurOnly || TaskMode == EAbyssShipTaskMode::Sabotage)
    {
        return Team == EAbyssTeam::Saboteur;
    }

    return true;
}

void AAbyssShipTaskActor::ApplyCompletedState()
{
    if (bSingleUse)
    {
        SetActorEnableCollision(!bCompleted);
    }
}

void AAbyssShipTaskActor::ApplyTaskConfigState()
{
    InteractionText = FText::FromString(TaskMode == EAbyssShipTaskMode::Sabotage ? TEXT("Sabotage") : TEXT("Repair"));
}

bool AAbyssShipTaskActor::ResolveShipSystem(FName SystemName, EAbyssShipSystem& OutSystem)
{
    const FString Normalized = SystemName.ToString().ToLower();
    if (Normalized == TEXT("hull"))
    {
        OutSystem = EAbyssShipSystem::Hull;
        return true;
    }
    if (Normalized == TEXT("fuel"))
    {
        OutSystem = EAbyssShipSystem::Fuel;
        return true;
    }
    if (Normalized == TEXT("engine"))
    {
        OutSystem = EAbyssShipSystem::Engine;
        return true;
    }
    if (Normalized == TEXT("power"))
    {
        OutSystem = EAbyssShipSystem::Power;
        return true;
    }
    if (Normalized == TEXT("radio"))
    {
        OutSystem = EAbyssShipSystem::Radio;
        return true;
    }
    if (Normalized == TEXT("route"))
    {
        OutSystem = EAbyssShipSystem::Route;
        return true;
    }
    if (Normalized == TEXT("heat"))
    {
        OutSystem = EAbyssShipSystem::Heat;
        return true;
    }
    if (Normalized == TEXT("flooding"))
    {
        OutSystem = EAbyssShipSystem::Flooding;
        return true;
    }

    return false;
}
