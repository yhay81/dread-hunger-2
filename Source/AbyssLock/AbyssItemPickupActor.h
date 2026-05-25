#pragma once

#include "CoreMinimal.h"
#include "AbyssInteractableActor.h"
#include "AbyssItemPickupActor.generated.h"

UCLASS(Blueprintable)
class ABYSSLOCK_API AAbyssItemPickupActor : public AAbyssInteractableActor
{
    GENERATED_BODY()

public:
    AAbyssItemPickupActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Item")
    void ConfigureItem(FName NewItemId);

    UFUNCTION(BlueprintCallable, Category = "Abyss|Item")
    FName GetItemId() const { return ItemId; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Item")
    bool IsConsumed() const { return bConsumed; }

protected:
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Item")
    FName ItemId;

    UPROPERTY(ReplicatedUsing = OnRep_Consumed, BlueprintReadOnly, Category = "Abyss|Item")
    bool bConsumed;

    UFUNCTION()
    void OnRep_Consumed();
};
