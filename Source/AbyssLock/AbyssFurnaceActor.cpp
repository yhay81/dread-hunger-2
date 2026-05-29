#include "AbyssFurnaceActor.h"

#include "AbyssInventoryComponent.h"
#include "AbyssLockGameState.h"
#include "AbyssLockLog.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Internationalization/Text.h"

AAbyssFurnaceActor::AAbyssFurnaceActor()
{
    InteractionText = FText::FromString(TEXT("Load Fuel & Ignite"));
}

bool AAbyssFurnaceActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !GetWorld() || !InstigatorPawn)
    {
        return false;
    }

    AAbyssLockGameState* GameState = GetWorld()->GetGameState<AAbyssLockGameState>();
    if (!GameState)
    {
        return false;
    }

    // Loading requires a fuel item in the player's inventory.
    UAbyssInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UAbyssInventoryComponent>();
    if (!Inventory || !Inventory->HasItem(FuelItemId))
    {
        UE_LOG(LogAbyssGameplay, Log, TEXT("furnace_load_failed reason=no_fuel item=%s"), *FuelItemId.ToString());
        return false;
    }
    Inventory->TryRemoveItem(FuelItemId);

    FAbyssShipSystemStatus FuelStatus;
    GameState->GetShipSystemStatus(EAbyssShipSystem::Fuel, FuelStatus);
    const float NewFuel = FMath::Clamp(FuelStatus.Condition + FuelPerLoad, 0.0f, 1.0f);
    GameState->SetShipSystemStatus(EAbyssShipSystem::Fuel, NewFuel, FuelStatus.bOffline, FuelStatus.bSabotaged);
    GameState->SetFurnaceLit(true);

    UE_LOG(LogAbyssGameplay, Log, TEXT("furnace_loaded fuel=%.2f lit=1"), NewFuel);
    return true;
}
