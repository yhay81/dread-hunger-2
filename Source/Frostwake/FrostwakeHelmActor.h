#pragma once

#include "CoreMinimal.h"
#include "FrostwakeInteractableActor.h"
#include "FrostwakeHelmActor.generated.h"

// The ship's helm. Interacting toggles "manning" the helm; while it is manned AND the furnace is lit
// with fuel, the voyage (RouteProgress) advances. The ship never advances on its own.
UCLASS()
class FROSTWAKE_API AFrostwakeHelmActor : public AFrostwakeInteractableActor
{
    GENERATED_BODY()

public:
    AFrostwakeHelmActor();

    virtual bool Interact(APawn* InstigatorPawn) override;
};
