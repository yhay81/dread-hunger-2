#include "AbyssItemPickupActor.h"
#include "AbyssInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "AbyssLockLog.h"
#include "AbyssTelemetrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

AAbyssItemPickupActor::AAbyssItemPickupActor()
{
    ItemId = TEXT("Item");
    bConsumed = false;
    InteractionText = FText::FromString(TEXT("Pick Up"));

    // Small visible marker so the item can be seen and walked up to (hidden on pickup).
    DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
    DisplayMesh->SetupAttachment(RootComponent);
    DisplayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        DisplayMesh->SetStaticMesh(CubeMesh.Object);
        DisplayMesh->SetRelativeScale3D(FVector(0.3f));
    }
}

void AAbyssItemPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssItemPickupActor, ItemId);
    DOREPLIFETIME(AAbyssItemPickupActor, bConsumed);
}

bool AAbyssItemPickupActor::CanInteract(const APawn* InstigatorPawn) const
{
    return Super::CanInteract(InstigatorPawn) && !bConsumed && !ItemId.IsNone();
}

bool AAbyssItemPickupActor::Interact(APawn* InstigatorPawn)
{
    if (!CanInteract(InstigatorPawn) || !HasAuthority())
    {
        return false;
    }

    UAbyssInventoryComponent* InventoryComponent = InstigatorPawn->FindComponentByClass<UAbyssInventoryComponent>();
    if (!InventoryComponent || !InventoryComponent->TryAddItem(ItemId))
    {
        return false;
    }

    bConsumed = true;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    UE_LOG(LogAbyssGameplay, Log, TEXT("item_pickup item=%s pickup=%s pawn=%s"), *ItemId.ToString(), *GetNameSafe(this), *GetNameSafe(InstigatorPawn));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("item_pickup"),
                FString::Printf(
                    TEXT("{\"item\":\"%s\",\"pickup\":\"%s\",\"pawn\":\"%s\"}"),
                    *ItemId.ToString(),
                    *GetNameSafe(this),
                    *GetNameSafe(InstigatorPawn)));
        }
    }
    return true;
}

void AAbyssItemPickupActor::ConfigureItem(FName NewItemId)
{
    if (!HasAuthority())
    {
        return;
    }

    ItemId = NewItemId;
    bConsumed = false;
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    ForceNetUpdate();
}

void AAbyssItemPickupActor::OnRep_Consumed()
{
    SetActorHiddenInGame(bConsumed);
    SetActorEnableCollision(!bConsumed);
}
