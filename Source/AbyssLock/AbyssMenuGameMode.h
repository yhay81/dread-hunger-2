#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AbyssMenuGameMode.generated.h"

// Lightweight front-end GameMode for the main menu map (L_MainMenu). It spawns the
// menu-showing player controller with no pawn and no match logic. The gameplay map
// (L_IcebreakerWhitebox) keeps using AAbyssLockGameMode.
UCLASS()
class ABYSSLOCK_API AAbyssMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAbyssMenuGameMode();
};
