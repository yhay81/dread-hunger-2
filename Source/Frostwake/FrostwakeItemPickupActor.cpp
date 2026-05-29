#include "FrostwakeItemPickupActor.h"
#include "FrostwakeInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "FrostwakeLog.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

AFrostwakeItemPickupActor::AFrostwakeItemPickupActor()
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

void AFrostwakeItemPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFrostwakeItemPickupActor, ItemId);
    DOREPLIFETIME(AFrostwakeItemPickupActor, bConsumed);
}

bool AFrostwakeItemPickupActor::CanInteract(const APawn* InstigatorPawn) const
{
    return Super::CanInteract(InstigatorPawn) && !bConsumed && !ItemId.IsNone();
}

bool AFrostwakeItemPickupActor::Interact(APawn* InstigatorPawn)
{
    if (!CanInteract(InstigatorPawn) || !HasAuthority())
    {
        return false;
    }

    UFrostwakeInventoryComponent* InventoryComponent = InstigatorPawn->FindComponentByClass<UFrostwakeInventoryComponent>();
    if (!InventoryComponent || !InventoryComponent->TryAddItem(ItemId))
    {
        return false;
    }

    bConsumed = true;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_pickup item=%s pickup=%s pawn=%s"), *ItemId.ToString(), *GetNameSafe(this), *GetNameSafe(InstigatorPawn));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
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

void AFrostwakeItemPickupActor::ConfigureItem(FName NewItemId)
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

void AFrostwakeItemPickupActor::OnRep_Consumed()
{
    SetActorHiddenInGame(bConsumed);
    SetActorEnableCollision(!bConsumed);
}
