#include "FrostwakeMenuGameMode.h"

#include "FrostwakePlayerController.h"

AFrostwakeMenuGameMode::AFrostwakeMenuGameMode()
{
    PlayerControllerClass = AFrostwakePlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    bStartPlayersAsSpectators = true;
}
