#include "FrostwakeFurnaceActor.h"

#include "FrostwakeInventoryComponent.h"
#include "FrostwakeGameState.h"
#include "FrostwakeLog.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Internationalization/Text.h"

AFrostwakeFurnaceActor::AFrostwakeFurnaceActor()
{
    InteractionText = FText::FromString(TEXT("Load Fuel & Ignite"));
}

bool AFrostwakeFurnaceActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !GetWorld() || !InstigatorPawn)
    {
        return false;
    }

    AFrostwakeGameState* GameState = GetWorld()->GetGameState<AFrostwakeGameState>();
    if (!GameState)
    {
        return false;
    }

    // Loading requires a fuel item in the player's inventory.
    UFrostwakeInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UFrostwakeInventoryComponent>();
    if (!Inventory || !Inventory->HasItem(FuelItemId))
    {
        UE_LOG(LogFrostwakeGameplay, Log, TEXT("furnace_load_failed reason=no_fuel item=%s"), *FuelItemId.ToString());
        return false;
    }
    Inventory->TryRemoveItem(FuelItemId);

    FFrostwakeShipSystemStatus FuelStatus;
    GameState->GetShipSystemStatus(EFrostwakeShipSystem::Fuel, FuelStatus);
    const float NewFuel = FMath::Clamp(FuelStatus.Condition + FuelPerLoad, 0.0f, 1.0f);
    GameState->SetShipSystemStatus(EFrostwakeShipSystem::Fuel, NewFuel, FuelStatus.bOffline, FuelStatus.bSabotaged);
    GameState->SetFurnaceLit(true);

    UE_LOG(LogFrostwakeGameplay, Log, TEXT("furnace_loaded fuel=%.2f lit=1"), NewFuel);
    return true;
}
