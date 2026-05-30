#include "FrostwakeInventoryComponent.h"
#include "FrostwakeItemPickupActor.h"
#include "FrostwakeLog.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "Data/FrostwakeDataSubsystem.h"
#include "Data/FrostwakeItemDefinition.h"
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

    // The backpack stays private to its owner; the held item is public so others can see what you carry.
    DOREPLIFETIME_CONDITION(UFrostwakeInventoryComponent, Items, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UFrostwakeInventoryComponent, HeldItemId, COND_None);
}

void UFrostwakeInventoryComponent::SetServerSelectedSlot(int32 SlotIndex)
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }
    SelectedSlot = (Items.Num() > 0) ? FMath::Clamp(SlotIndex, 0, Items.Num() - 1) : 0;
    RefreshHeldItem();
}

void UFrostwakeInventoryComponent::RefreshHeldItem()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }
    const FName NewHeld = Items.IsValidIndex(SelectedSlot) ? Items[SelectedSlot].ItemId : NAME_None;
    if (NewHeld != HeldItemId)
    {
        HeldItemId = NewHeld;
        OwnerActor->ForceNetUpdate();
    }
}

void UFrostwakeInventoryComponent::OnRep_HeldItemId()
{
    // Clients observe HeldItemId via this OnRep (and GetHeldItemId); a held-item nameplate/display reads it.
}

int32 UFrostwakeInventoryComponent::GetMaxStackFor(FName ItemId) const
{
    if (const UWorld* World = GetWorld())
    {
        if (const UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (const UFrostwakeDataSubsystem* DataSubsystem = GameInstance->GetSubsystem<UFrostwakeDataSubsystem>())
            {
                if (const UFrostwakeItemDefinition* Definition = DataSubsystem->GetItemDefinition(ItemId))
                {
                    return FMath::Max(1, Definition->MaxStack);
                }
            }
        }
    }
    return 1; // unknown item / no data subsystem: behave as non-stacking
}

bool UFrostwakeInventoryComponent::TryAddItem(FName ItemId)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority() || ItemId.IsNone())
    {
        return false;
    }

    const int32 MaxStack = GetMaxStackFor(ItemId);

    // Fill an existing stack of the same item first (data-driven MaxStack), then open a new slot.
    for (FFrostwakeInventoryEntry& Entry : Items)
    {
        if (Entry.ItemId == ItemId && Entry.Count < MaxStack)
        {
            ++Entry.Count;
            OwnerActor->ForceNetUpdate();
            UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_added owner=%s item=%s count=%d stack=%d"), *GetNameSafe(OwnerActor), *ItemId.ToString(), GetItemCount(), Entry.Count);
            RefreshHeldItem();
            return true;
        }
    }

    if (Items.Num() >= MaxSlots)
    {
        return false;
    }

    Items.Add(FFrostwakeInventoryEntry{ ItemId, 1 });
    OwnerActor->ForceNetUpdate();
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_added owner=%s item=%s count=%d stack=%d"), *GetNameSafe(OwnerActor), *ItemId.ToString(), GetItemCount(), 1);
    RefreshHeldItem();
    return true;
}

bool UFrostwakeInventoryComponent::TryRemoveItem(FName ItemId)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority() || ItemId.IsNone())
    {
        return false;
    }

    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        if (Items[Index].ItemId == ItemId)
        {
            if (--Items[Index].Count <= 0)
            {
                Items.RemoveAt(Index, 1, EAllowShrinking::No);
            }
            OwnerActor->ForceNetUpdate();
            UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d"), *GetNameSafe(OwnerActor), *ItemId.ToString(), GetItemCount());
            RefreshHeldItem();
            return true;
        }
    }
    return false;
}

bool UFrostwakeInventoryComponent::TryRemoveFirstItem(FName& OutItemId)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority() || Items.Num() <= 0)
    {
        OutItemId = NAME_None;
        return false;
    }

    OutItemId = Items[0].ItemId;
    if (--Items[0].Count <= 0)
    {
        Items.RemoveAt(0, 1, EAllowShrinking::No);
    }
    OwnerActor->ForceNetUpdate();
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d source=drop"), *GetNameSafe(OwnerActor), *OutItemId.ToString(), GetItemCount());
    RefreshHeldItem();
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

    OutItemId = Items[SlotIndex].ItemId;
    if (--Items[SlotIndex].Count <= 0)
    {
        Items.RemoveAt(SlotIndex, 1, EAllowShrinking::No);
    }
    SelectedSlot = (Items.Num() > 0) ? FMath::Min(SelectedSlot, Items.Num() - 1) : 0;
    OwnerActor->ForceNetUpdate();
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("item_removed owner=%s item=%s count=%d source=use"), *GetNameSafe(OwnerActor), *OutItemId.ToString(), GetItemCount());
    RefreshHeldItem();
    return true;
}

FName UFrostwakeInventoryComponent::GetItemIdAt(int32 SlotIndex) const
{
    return Items.IsValidIndex(SlotIndex) ? Items[SlotIndex].ItemId : NAME_None;
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
        // Rollback the removed unit (stacking-aware: back into a stack or a fresh slot).
        TryAddItem(DroppedItemId);
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
        GetItemCount());

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
                    GetItemCount()));
        }
    }

    return DroppedPickup;
}

bool UFrostwakeInventoryComponent::HasItem(FName ItemId) const
{
    for (const FFrostwakeInventoryEntry& Entry : Items)
    {
        if (Entry.ItemId == ItemId && Entry.Count > 0)
        {
            return true;
        }
    }
    return false;
}

TArray<FName> UFrostwakeInventoryComponent::GetItems() const
{
    TArray<FName> Result;
    Result.Reserve(Items.Num());
    for (const FFrostwakeInventoryEntry& Entry : Items)
    {
        Result.Add(Entry.ItemId);
    }
    return Result;
}

int32 UFrostwakeInventoryComponent::GetItemCount() const
{
    int32 Total = 0;
    for (const FFrostwakeInventoryEntry& Entry : Items)
    {
        Total += Entry.Count;
    }
    return Total;
}

int32 UFrostwakeInventoryComponent::GetItemCountOf(FName ItemId) const
{
    int32 Total = 0;
    for (const FFrostwakeInventoryEntry& Entry : Items)
    {
        if (Entry.ItemId == ItemId)
        {
            Total += Entry.Count;
        }
    }
    return Total;
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
    return Items[GetSelectedSlot()].ItemId;
}
