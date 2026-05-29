#pragma once

#include "CoreMinimal.h"
#include "AbyssInteractableActor.h"
#include "AbyssHeatSourceActor.generated.h"

class UStaticMeshComponent;

// A warm spot on the ship (stove / brazier). Interacting with it restores the player's Warmth meter.
// Original generic framing; functional counterpart to the genre's heat sources.
UCLASS(Blueprintable)
class ABYSSLOCK_API AAbyssHeatSourceActor : public AAbyssInteractableActor
{
    GENERATED_BODY()

public:
    AAbyssHeatSourceActor();

    virtual bool Interact(APawn* InstigatorPawn) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Heat")
    UStaticMeshComponent* DisplayMesh;

    // Warmth restored per interaction (clamped to the character's MaxWarmth).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Heat")
    float WarmthPerUse;
};
