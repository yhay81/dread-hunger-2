#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FrostwakeMenuGameMode.generated.h"

// Lightweight front-end GameMode for the main menu map (L_MainMenu). It spawns the
// menu-showing player controller with no pawn and no match logic. The gameplay map
// (L_IcebreakerWhitebox) keeps using AFrostwakeGameMode.
UCLASS()
class FROSTWAKE_API AFrostwakeMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AFrostwakeMenuGameMode();
};
