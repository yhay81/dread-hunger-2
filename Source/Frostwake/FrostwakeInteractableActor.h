#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FrostwakeInteractableActor.generated.h"

class USceneComponent;
class USphereComponent;

UCLASS(Abstract, Blueprintable)
class FROSTWAKE_API AFrostwakeInteractableActor : public AActor
{
    GENERATED_BODY()

public:
    AFrostwakeInteractableActor();

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    virtual bool CanInteract(const APawn* InstigatorPawn) const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    virtual bool Interact(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    FText GetInteractionText() const { return InteractionText; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Interaction")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Interaction")
    TObjectPtr<USphereComponent> InteractionBounds;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Interaction")
    FText InteractionText;
};
