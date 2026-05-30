#pragma once

#include "CoreMinimal.h"
#include "FrostwakeInteractableActor.h"
#include "FrostwakeHeatSourceActor.generated.h"

class UStaticMeshComponent;
class UFrostwakeHeatSourceComponent;

// A warm spot on the ship (stove / brazier). It passively radiates warmth into the shared temperature
// model (plan §3.22-23) while nearby, and interacting with it gives an extra Warmth boost.
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

    // Passive radiator feeding the shared temperature aggregation (plan §3.22-23, §9.1 step 6).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Heat")
    UFrostwakeHeatSourceComponent* HeatSourceComponent;

    // Warmth restored per interaction (clamped to the player's max warmth by the attribute component).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Heat")
    float WarmthPerUse;
};
