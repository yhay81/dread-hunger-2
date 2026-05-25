#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbyssInteractionComponent.generated.h"

class AAbyssInteractableActor;

UCLASS(ClassGroup = (Abyss), meta = (BlueprintSpawnableComponent))
class ABYSSLOCK_API UAbyssInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAbyssInteractionComponent();

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    void TryInteract();

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    AAbyssInteractableActor* FindBestInteractable() const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Interaction")
    float InteractionDistance;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Interaction")
    float InteractionRadius;

    UFUNCTION(Server, Reliable)
    void ServerTryInteract(AAbyssInteractableActor* Target);

    bool IsTargetInRange(const AAbyssInteractableActor* Target) const;
};
