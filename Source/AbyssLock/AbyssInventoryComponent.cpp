#include "AbyssInventoryComponent.h"
#include "AbyssItemPickupActor.h"
#include "AbyssLockLog.h"
#include "AbyssTelemetrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UAbyssInventoryComponent::UAbyssInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    MaxSlots = 2;
    DropPickupClass = AAbyssItemPickupActor::StaticClass();
}

void UAbyssInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UAbyssInventoryComponent, Items, COND_OwnerOnly);
}

bool UAbyssInventoryComponent::TryAddItem(FName ItemId)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority())
    {
        return false;
    }

    if (ItemId.IsNone() || Items.Num() >= MaxSlots)
    {
        return false;
    }

    Items.Add(ItemId);
    OwnerActor->ForceNetUpdate();
    UE_LOG(LogAbyssGameplay, Log, TEXT("item_added owner=%s item=%s count=%d"), *GetNameSafe(GetOwner()), *ItemId.ToString(), Items.Num());
    return true;
}

bool UAbyssInventoryComponent::TryRemoveItem(FName ItemId)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority() || ItemId.IsNone())
    {
        return false;
    }

    const int32 RemovedCount = Items.RemoveSingle(ItemId);
    if (RemovedCount <= 0)
    {
        return false;
    }

    OwnerActor->ForceNetUpdate();
    UE_LOG(LogAbyssGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d"), *GetNameSafe(GetOwner()), *ItemId.ToString(), Items.Num());
    return true;
}

bool UAbyssInventoryComponent::TryRemoveFirstItem(FName& OutItemId)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority() || Items.Num() <= 0)
    {
        OutItemId = NAME_None;
        return false;
    }

    OutItemId = Items[0];
    Items.RemoveAt(0, 1, EAllowShrinking::No);
    OwnerActor->ForceNetUpdate();
    UE_LOG(LogAbyssGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d source=drop"), *GetNameSafe(GetOwner()), *OutItemId.ToString(), Items.Num());
    return true;
}

AAbyssItemPickupActor* UAbyssInventoryComponent::TryDropFirstItem(const FVector& DropLocation, const FRotator& DropRotation)
{
    AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();
    if (!OwnerActor || !OwnerActor->HasAuthority() || !World)
    {
        return nullptr;
    }

    FName DroppedItemId = NAME_None;
    if (!TryRemoveFirstItem(DroppedItemId))
    {
        return nullptr;
    }

    UClass* PickupClass = DropPickupClass ? DropPickupClass.Get() : AAbyssItemPickupActor::StaticClass();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = OwnerActor;
    SpawnParameters.Instigator = Cast<APawn>(OwnerActor);
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAbyssItemPickupActor* DroppedPickup = World->SpawnActor<AAbyssItemPickupActor>(PickupClass, DropLocation, DropRotation, SpawnParameters);
    if (!DroppedPickup)
    {
        Items.Insert(DroppedItemId, 0);
        OwnerActor->ForceNetUpdate();
        UE_LOG(LogAbyssGameplay, Warning, TEXT("item_drop_failed owner=%s item=%s reason=spawn_failed"), *GetNameSafe(OwnerActor), *DroppedItemId.ToString());
        return nullptr;
    }

    DroppedPickup->ConfigureItem(DroppedItemId);
    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("item_dropped owner=%s item=%s pickup=%s count=%d"),
        *GetNameSafe(OwnerActor),
        *DroppedItemId.ToString(),
        *GetNameSafe(DroppedPickup),
        Items.Num());

    if (UGameInstance* GameInstance = World->GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("item_dropped"),
                FString::Printf(
                    TEXT("{\"owner\":\"%s\",\"item\":\"%s\",\"pickup\":\"%s\",\"count\":%d}"),
                    *GetNameSafe(OwnerActor),
                    *DroppedItemId.ToString(),
                    *GetNameSafe(DroppedPickup),
                    Items.Num()));
        }
    }

    return DroppedPickup;
}

bool UAbyssInventoryComponent::HasItem(FName ItemId) const
{
    return Items.Contains(ItemId);
}

void UAbyssInventoryComponent::OnRep_Items()
{
}
