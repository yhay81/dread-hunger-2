#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrostwakeInventoryComponent.generated.h"

class AFrostwakeItemPickupActor;

/**
 * One occupied inventory slot: an item id plus how many units are stacked in it (review #2 — the container
 * must express ItemDefinition.MaxStack, e.g. 5 rations in one slot, before the items(55) content lands).
 */
USTRUCT(BlueprintType)
struct FFrostwakeInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Inventory")
	int32 Count = 0;
};

UCLASS(ClassGroup = (Frostwake), meta = (BlueprintSpawnableComponent))
class FROSTWAKE_API UFrostwakeInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFrostwakeInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Add one unit of ItemId. Stacks into an existing slot of the same item up to its MaxStack
    // (ItemDefinition data); otherwise opens a new slot if one is free. Returns false if there is no room.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    bool TryAddItem(FName ItemId);

    // Remove one unit of ItemId (decrementing a stack; the slot frees when it hits zero).
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Inventory")
    bool TryRemoveItem(FName ItemId);

    bool TryRemoveFirstItem(FName& OutItemId);

    // Server-authoritative remove of one unit from a specific slot (used by "use selected item"). Returns the
    // removed id via OutItemId. SelectedSlot is local/non-replicated, so callers re-clamp via
    // GetSelectedSlot() afterward; the server's own SelectedSlot is irrelevant.
    bool TryRemoveItemAt(int32 SlotIndex, FName& OutItemId);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    FName GetItemIdAt(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Inventory")
    AFrostwakeItemPickupActor* TryDropFirstItem(const FVector& DropLocation, const FRotator& DropRotation);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    bool HasItem(FName ItemId) const;

    // One ItemId per occupied slot (NOT expanded by count) — the slot view used by the HUD and by
    // index-based lookups (IndexOfByKey(ItemId) finds the slot).
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    TArray<FName> GetItems() const;

    // The raw slot entries (id + count). C++ callers that need stack sizes (HUD badges, crafting) use this.
    const TArray<FFrostwakeInventoryEntry>& GetEntries() const { return Items; }

    // Total units across all slots (a stack of 5 counts as 5).
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    int32 GetItemCount() const;

    // Total units of a specific item across all its stacks.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    int32 GetItemCountOf(FName ItemId) const;

    // Number of occupied slots (a stack of 5 counts as 1).
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    int32 GetSlotCount() const { return Items.Num(); }

    // Selected/active slot (local UI choice) — scroll wheel cycles it; the HUD shows it.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    void CycleSelectedSlot(int32 Delta);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    int32 GetSelectedSlot() const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    FName GetSelectedItemId() const;

protected:
    // Per-slot stacks. NOTE (review #2 follow-up): still COND_OwnerOnly — the full backpack is private.
    // Making the *held/equipped* item visible to other players (DH social read: "who carries the Nitro")
    // is the paired next slice and needs a selection->server path + a separate replicated held-item field.
    UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Frostwake|Inventory")
    TArray<FFrostwakeInventoryEntry> Items;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Inventory")
    int32 MaxSlots;

    // Local (non-replicated) selected slot for the hotbar; cycled by the scroll wheel.
    int32 SelectedSlot = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Inventory")
    TSubclassOf<AFrostwakeItemPickupActor> DropPickupClass;

    UFUNCTION()
    void OnRep_Items();

private:
    // MaxStack for an item from its ItemDefinition (data); 1 (no stacking) if unknown.
    int32 GetMaxStackFor(FName ItemId) const;
};
