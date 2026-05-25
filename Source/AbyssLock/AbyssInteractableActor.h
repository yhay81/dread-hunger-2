#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractableActor.generated.h"

class USceneComponent;
class USphereComponent;

UCLASS(Abstract, Blueprintable)
class ABYSSLOCK_API AAbyssInteractableActor : public AActor
{
    GENERATED_BODY()

public:
    AAbyssInteractableActor();

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    virtual bool CanInteract(const APawn* InstigatorPawn) const;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    virtual bool Interact(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    FText GetInteractionText() const { return InteractionText; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Interaction")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Interaction")
    TObjectPtr<USphereComponent> InteractionBounds;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Interaction")
    FText InteractionText;
};
