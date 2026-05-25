#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbyssInventoryComponent.generated.h"

class AAbyssItemPickupActor;

UCLASS(ClassGroup = (Abyss), meta = (BlueprintSpawnableComponent))
class ABYSSLOCK_API UAbyssInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAbyssInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Inventory")
    bool TryAddItem(FName ItemId);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Inventory")
    bool TryRemoveItem(FName ItemId);

    bool TryRemoveFirstItem(FName& OutItemId);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Inventory")
    AAbyssItemPickupActor* TryDropFirstItem(const FVector& DropLocation, const FRotator& DropRotation);

    UFUNCTION(BlueprintCallable, Category = "Abyss|Inventory")
    bool HasItem(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Inventory")
    TArray<FName> GetItems() const { return Items; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Inventory")
    int32 GetItemCount() const { return Items.Num(); }

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Abyss|Inventory")
    TArray<FName> Items;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Inventory")
    int32 MaxSlots;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Inventory")
    TSubclassOf<AAbyssItemPickupActor> DropPickupClass;

    UFUNCTION()
    void OnRep_Items();
};
