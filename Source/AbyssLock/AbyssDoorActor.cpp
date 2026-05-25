#include "AbyssDoorActor.h"
#include "AbyssLockGameState.h"
#include "AbyssLockLog.h"
#include "AbyssLockPlayerState.h"
#include "AbyssTelemetrySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAbyssDoorActor::AAbyssDoorActor()
{
    DoorPanelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanelMesh"));
    DoorPanelMesh->SetupAttachment(SceneRoot);
    DoorPanelMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
    DoorPanelMesh->SetRelativeScale3D(FVector(0.12f, 1.45f, 1.8f));
    DoorPanelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    DoorPanelMesh->SetCollisionObjectType(ECC_WorldDynamic);
    DoorPanelMesh->SetCollisionResponseToAllChannels(ECR_Block);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        DoorPanelMesh->SetStaticMesh(CubeMesh.Object);
    }

    DoorId = TEXT("Bulkhead");
    bIsOpen = false;
    bIsLocked = false;
    InteractionText = FText::FromString(TEXT("Open Bulkhead"));
}

void AAbyssDoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssDoorActor, DoorId);
    DOREPLIFETIME(AAbyssDoorActor, bIsOpen);
    DOREPLIFETIME(AAbyssDoorActor, bIsLocked);
}

bool AAbyssDoorActor::CanInteract(const APawn* InstigatorPawn) const
{
    return Super::CanInteract(InstigatorPawn) && IsInstigatorAliveAndAssigned(InstigatorPawn);
}

bool AAbyssDoorActor::Interact(APawn* InstigatorPawn)
{
    if (!CanInteract(InstigatorPawn) || !HasAuthority())
    {
        return false;
    }

    if (bIsLocked)
    {
        UE_LOG(LogAbyssGameplay, Log, TEXT("bulkhead_locked_interaction_blocked door=%s doorId=%s pawn=%s"), *GetNameSafe(this), *DoorId.ToString(), *GetNameSafe(InstigatorPawn));
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("bulkhead_locked_interaction_blocked"),
                    FString::Printf(
                        TEXT("{\"door\":\"%s\",\"doorId\":\"%s\",\"pawn\":\"%s\"}"),
                        *GetNameSafe(this),
                        *DoorId.ToString(),
                        *GetNameSafe(InstigatorPawn)));
            }
        }
        return false;
    }

    bIsOpen = !bIsOpen;
    ApplyDoorState();
    UE_LOG(LogAbyssGameplay, Log, TEXT("door_toggled door=%s open=%s pawn=%s"), *GetNameSafe(this), bIsOpen ? TEXT("true") : TEXT("false"), *GetNameSafe(InstigatorPawn));
    EmitDoorToggledEvent(InstigatorPawn);
    return true;
}

void AAbyssDoorActor::SetLocked(bool bNewLocked)
{
    if (HasAuthority())
    {
        bIsLocked = bNewLocked;
        if (bIsLocked)
        {
            bIsOpen = false;
        }
        ApplyDoorState();
        EmitDoorLockChangedEvent(nullptr, bIsLocked, TEXT("manual"));
    }
}

bool AAbyssDoorActor::LockForSabotage(APawn* InstigatorPawn)
{
    if (!HasAuthority() || bIsLocked || !IsInstigatorSaboteur(InstigatorPawn))
    {
        return false;
    }

    const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
    if (!AbyssGameState || !AbyssGameState->IsMatchActive())
    {
        return false;
    }

    bIsLocked = true;
    bIsOpen = false;
    ApplyDoorState();
    UE_LOG(LogAbyssGameplay, Log, TEXT("door_lock_changed door=%s doorId=%s locked=true reason=sabotage pawn=%s"), *GetNameSafe(this), *DoorId.ToString(), *GetNameSafe(InstigatorPawn));
    EmitDoorLockChangedEvent(InstigatorPawn, true, TEXT("sabotage"));
    return true;
}

bool AAbyssDoorActor::UnlockFromRepair(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !bIsLocked || !IsInstigatorAliveAndAssigned(InstigatorPawn))
    {
        return false;
    }

    const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
    if (!AbyssGameState || !AbyssGameState->IsMatchActive())
    {
        return false;
    }

    bIsLocked = false;
    ApplyDoorState();
    UE_LOG(LogAbyssGameplay, Log, TEXT("door_lock_changed door=%s doorId=%s locked=false reason=repair pawn=%s"), *GetNameSafe(this), *DoorId.ToString(), *GetNameSafe(InstigatorPawn));
    EmitDoorLockChangedEvent(InstigatorPawn, false, TEXT("repair"));
    return true;
}

void AAbyssDoorActor::ConfigureDoorByName(FName InDoorId)
{
    DoorId = InDoorId.IsNone() ? TEXT("Bulkhead") : InDoorId;
    ApplyDoorState();
}

void AAbyssDoorActor::OnRep_DoorState()
{
    ApplyDoorState();
}

void AAbyssDoorActor::ApplyDoorState()
{
    const bool bBlocks = !bIsOpen;
    if (DoorPanelMesh)
    {
        DoorPanelMesh->SetVisibility(bBlocks, true);
        DoorPanelMesh->SetHiddenInGame(!bBlocks, true);
        DoorPanelMesh->SetCollisionEnabled(bBlocks ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }

    if (bIsLocked)
    {
        InteractionText = FText::FromString(TEXT("Locked Bulkhead"));
    }
    else
    {
        InteractionText = FText::FromString(bIsOpen ? TEXT("Close Bulkhead") : TEXT("Open Bulkhead"));
    }
}

bool AAbyssDoorActor::IsInstigatorAliveAndAssigned(const APawn* InstigatorPawn) const
{
    const AController* Controller = InstigatorPawn ? InstigatorPawn->GetController() : nullptr;
    const AAbyssLockPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AAbyssLockPlayerState>() : nullptr;
    if (!PlayerState || PlayerState->GetLifeState() != EAbyssLifeState::Alive)
    {
        return false;
    }

    const EAbyssTeam Team = PlayerState->GetSecretTeamForOwner();
    return Team != EAbyssTeam::Unassigned && Team != EAbyssTeam::Spectator;
}

bool AAbyssDoorActor::IsInstigatorSaboteur(const APawn* InstigatorPawn) const
{
    if (!IsInstigatorAliveAndAssigned(InstigatorPawn))
    {
        return false;
    }

    const AController* Controller = InstigatorPawn ? InstigatorPawn->GetController() : nullptr;
    const AAbyssLockPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AAbyssLockPlayerState>() : nullptr;
    return PlayerState && PlayerState->GetSecretTeamForOwner() == EAbyssTeam::Saboteur;
}

void AAbyssDoorActor::EmitDoorLockChangedEvent(const APawn* InstigatorPawn, bool bNewLocked, const TCHAR* Reason) const
{
    const FString ReasonText = Reason ? FString(Reason) : FString(TEXT("unknown"));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("door_lock_changed"),
                FString::Printf(
                    TEXT("{\"door\":\"%s\",\"doorId\":\"%s\",\"locked\":%s,\"reason\":\"%s\",\"pawn\":\"%s\"}"),
                    *GetNameSafe(this),
                    *DoorId.ToString(),
                    bNewLocked ? TEXT("true") : TEXT("false"),
                    *ReasonText,
                    *GetNameSafe(InstigatorPawn)));

            if (bNewLocked && ReasonText.Equals(TEXT("sabotage"), ESearchCase::IgnoreCase))
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("bulkhead_lock_sabotaged"),
                    FString::Printf(
                        TEXT("{\"door\":\"%s\",\"doorId\":\"%s\",\"pawn\":\"%s\"}"),
                        *GetNameSafe(this),
                        *DoorId.ToString(),
                        *GetNameSafe(InstigatorPawn)));
            }
            else if (!bNewLocked && ReasonText.Equals(TEXT("repair"), ESearchCase::IgnoreCase))
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("bulkhead_lock_released"),
                    FString::Printf(
                        TEXT("{\"door\":\"%s\",\"doorId\":\"%s\",\"pawn\":\"%s\"}"),
                        *GetNameSafe(this),
                        *DoorId.ToString(),
                        *GetNameSafe(InstigatorPawn)));
            }
        }
    }
}

void AAbyssDoorActor::EmitDoorToggledEvent(const APawn* InstigatorPawn) const
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("door_toggled"),
                FString::Printf(
                    TEXT("{\"door\":\"%s\",\"doorId\":\"%s\",\"open\":%s,\"pawn\":\"%s\"}"),
                    *GetNameSafe(this),
                    *DoorId.ToString(),
                    bIsOpen ? TEXT("true") : TEXT("false"),
                    *GetNameSafe(InstigatorPawn)));
        }
    }
}
