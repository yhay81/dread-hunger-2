#pragma once

#include "CoreMinimal.h"
#include "FrostwakeInteractableActor.h"
#include "FrostwakeHeatSourceActor.generated.h"

class UStaticMeshComponent;

// A warm spot on the ship (stove / brazier). Interacting with it restores the player's Warmth meter.
// Original generic framing; functional counterpart to the genre's heat sources.
UCLASS(Blueprintable)
class FROSTWAKE_API AFrostwakeHeatSourceActor : public AFrostwakeInteractableActor
{
    GENERATED_BODY()

public:
    AFrostwakeHeatSourceActor();

    virtual bool Interact(APawn* InstigatorPawn) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Heat")
    UStaticMeshComponent* DisplayMesh;

    // Warmth restored per interaction (clamped to the character's MaxWarmth).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Heat")
    float WarmthPerUse;
};
