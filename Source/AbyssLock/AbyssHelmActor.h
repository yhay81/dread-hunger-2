#pragma once

#include "CoreMinimal.h"
#include "AbyssInteractableActor.h"
#include "AbyssHelmActor.generated.h"

// The ship's helm. Interacting toggles "manning" the helm; while it is manned AND the furnace is lit
// with fuel, the voyage (RouteProgress) advances. The ship never advances on its own.
UCLASS()
class ABYSSLOCK_API AAbyssHelmActor : public AAbyssInteractableActor
{
    GENERATED_BODY()

public:
    AAbyssHelmActor();

    virtual bool Interact(APawn* InstigatorPawn) override;
};
