#pragma once

#include "CoreMinimal.h"
#include "FrostwakeInteractableActor.h"
#include "FrostwakeItemPickupActor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class FROSTWAKE_API AFrostwakeItemPickupActor : public AFrostwakeInteractableActor
{
    GENERATED_BODY()

public:
    AFrostwakeItemPickupActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Item")
    void ConfigureItem(FName NewItemId);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Item")
    FName GetItemId() const { return ItemId; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Item")
    bool IsConsumed() const { return bConsumed; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Item")
    UStaticMeshComponent* DisplayMesh;

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Item")
    FName ItemId;

    UPROPERTY(ReplicatedUsing = OnRep_Consumed, BlueprintReadOnly, Category = "Frostwake|Item")
    bool bConsumed;

    UFUNCTION()
    void OnRep_Consumed();
};
