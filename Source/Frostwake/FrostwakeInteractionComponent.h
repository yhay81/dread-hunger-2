#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrostwakeInteractionComponent.generated.h"

class AFrostwakeInteractableActor;

UCLASS(ClassGroup = (Frostwake), meta = (BlueprintSpawnableComponent))
class FROSTWAKE_API UFrostwakeInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFrostwakeInteractionComponent();

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    void TryInteract();

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    AFrostwakeInteractableActor* FindBestInteractable() const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Interaction")
    float InteractionDistance;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Interaction")
    float InteractionRadius;

    UFUNCTION(Server, Reliable)
    void ServerTryInteract(AFrostwakeInteractableActor* Target);

    bool IsTargetInRange(const AFrostwakeInteractableActor* Target) const;
};
