#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrostwakeInventoryComponent.generated.h"

class AFrostwakeItemPickupActor;

UCLASS(ClassGroup = (Frostwake), meta = (BlueprintSpawnableComponent))
class FROSTWAKE_API UFrostwakeInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFrostwakeInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    bool TryAddItem(FName ItemId);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Inventory")
    bool TryRemoveItem(FName ItemId);

    bool TryRemoveFirstItem(FName& OutItemId);

    // Server-authoritative remove of a specific slot (used by "use selected item"). Returns the
    // removed id via OutItemId. SelectedSlot is local/non-replicated, so callers re-clamp via
    // GetSelectedSlot() afterward; the server's own SelectedSlot is irrelevant.
    bool TryRemoveItemAt(int32 SlotIndex, FName& OutItemId);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    FName GetItemIdAt(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Inventory")
    AFrostwakeItemPickupActor* TryDropFirstItem(const FVector& DropLocation, const FRotator& DropRotation);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    bool HasItem(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    TArray<FName> GetItems() const { return Items; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    int32 GetItemCount() const { return Items.Num(); }

    // Selected/active slot (local UI choice) — scroll wheel cycles it; the HUD shows it.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    void CycleSelectedSlot(int32 Delta);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    int32 GetSelectedSlot() const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    FName GetSelectedItemId() const;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Frostwake|Inventory")
    TArray<FName> Items;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Inventory")
    int32 MaxSlots;

    // Local (non-replicated) selected slot for the hotbar; cycled by the scroll wheel.
    int32 SelectedSlot = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Inventory")
    TSubclassOf<AFrostwakeItemPickupActor> DropPickupClass;

    UFUNCTION()
    void OnRep_Items();
};
