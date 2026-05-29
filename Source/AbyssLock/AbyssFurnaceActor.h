#pragma once

#include "CoreMinimal.h"
#include "AbyssInteractableActor.h"
#include "AbyssFurnaceActor.generated.h"

// The ship's furnace. Interacting loads one fuel item (consumed from the player's inventory) into the
// Fuel ship-system and ignites it. While lit and fueled, the furnace burns down over time (GameMode
// voyage tick); the ship only advances when a player is also manning the helm.
UCLASS()
class ABYSSLOCK_API AAbyssFurnaceActor : public AAbyssInteractableActor
{
    GENERATED_BODY()

public:
    AAbyssFurnaceActor();

    virtual bool Interact(APawn* InstigatorPawn) override;

protected:
    // Fuel added to the Fuel ship-system condition (0..1) per loaded item.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Furnace")
    float FuelPerLoad = 0.5f;

    // Inventory item id that counts as furnace fuel.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Furnace")
    FName FuelItemId = FName(TEXT("FuelDrum"));
};
