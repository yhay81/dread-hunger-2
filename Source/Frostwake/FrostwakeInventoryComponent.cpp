#include "FrostwakeInventoryComponent.h"
#include "FrostwakeItemPickupActor.h"
#include "FrostwakeLog.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UFrostwakeInventoryComponent::UFrostwakeInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    MaxSlots = 4;
    DropPickupClass = AFrostwakeItemPickupActor::StaticClass();
}

void UFrostwakeInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UFrostwakeInventoryComponent, Items, COND_OwnerOnly);
}

bool UFrostwakeInventoryComponent::TryAddItem(FName ItemId)
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
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_added owner=%s item=%s count=%d"), *GetNameSafe(GetOwner()), *ItemId.ToString(), Items.Num());
    return true;
}

bool UFrostwakeInventoryComponent::TryRemoveItem(FName ItemId)
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
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d"), *GetNameSafe(GetOwner()), *ItemId.ToString(), Items.Num());
    return true;
}

bool UFrostwakeInventoryComponent::TryRemoveFirstItem(FName& OutItemId)
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
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d source=drop"), *GetNameSafe(GetOwner()), *OutItemId.ToString(), Items.Num());
    return true;
}

bool UFrostwakeInventoryComponent::TryRemoveItemAt(int32 SlotIndex, FName& OutItemId)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority() || !Items.IsValidIndex(SlotIndex))
    {
        OutItemId = NAME_None;
        return false;
    }

    OutItemId = Items[SlotIndex];
    Items.RemoveAt(SlotIndex, 1, EAllowShrinking::No);
    SelectedSlot = (Items.Num() > 0) ? FMath::Min(SelectedSlot, Items.Num() - 1) : 0;
    OwnerActor->ForceNetUpdate();
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d source=use"), *GetNameSafe(GetOwner()), *OutItemId.ToString(), Items.Num());
    return true;
}

FName UFrostwakeInventoryComponent::GetItemIdAt(int32 SlotIndex) const
{
    return Items.IsValidIndex(SlotIndex) ? Items[SlotIndex] : NAME_None;
}

AFrostwakeItemPickupActor* UFrostwakeInventoryComponent::TryDropFirstItem(const FVector& DropLocation, const FRotator& DropRotation)
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

    UClass* PickupClass = DropPickupClass ? DropPickupClass.Get() : AFrostwakeItemPickupActor::StaticClass();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = OwnerActor;
    SpawnParameters.Instigator = Cast<APawn>(OwnerActor);
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AFrostwakeItemPickupActor* DroppedPickup = World->SpawnActor<AFrostwakeItemPickupActor>(PickupClass, DropLocation, DropRotation, SpawnParameters);
    if (!DroppedPickup)
    {
        Items.Insert(DroppedItemId, 0);
        OwnerActor->ForceNetUpdate();
        UE_LOG(LogFrostwakeGameplay, Warning, TEXT("item_drop_failed owner=%s item=%s reason=spawn_failed"), *GetNameSafe(OwnerActor), *DroppedItemId.ToString());
        return nullptr;
    }

    DroppedPickup->ConfigureItem(DroppedItemId);
    UE_LOG(
        LogFrostwakeGameplay,
        Log,
        TEXT("item_dropped owner=%s item=%s pickup=%s count=%d"),
        *GetNameSafe(OwnerActor),
        *DroppedItemId.ToString(),
        *GetNameSafe(DroppedPickup),
        Items.Num());

    if (UGameInstance* GameInstance = World->GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
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

bool UFrostwakeInventoryComponent::HasItem(FName ItemId) const
{
    return Items.Contains(ItemId);
}

void UFrostwakeInventoryComponent::OnRep_Items()
{
}

void UFrostwakeInventoryComponent::CycleSelectedSlot(int32 Delta)
{
    if (Items.Num() == 0)
    {
        SelectedSlot = 0;
        return;
    }

    const int32 Count = Items.Num();
    SelectedSlot = ((SelectedSlot + Delta) % Count + Count) % Count;
}

int32 UFrostwakeInventoryComponent::GetSelectedSlot() const
{
    if (Items.Num() == 0)
    {
        return 0;
    }
    return FMath::Clamp(SelectedSlot, 0, Items.Num() - 1);
}

FName UFrostwakeInventoryComponent::GetSelectedItemId() const
{
    if (Items.Num() == 0)
    {
        return NAME_None;
    }
    return Items[GetSelectedSlot()];
}
