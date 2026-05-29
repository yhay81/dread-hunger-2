#include "AbyssMenuGameMode.h"

#include "AbyssLockPlayerController.h"

AAbyssMenuGameMode::AAbyssMenuGameMode()
{
    PlayerControllerClass = AAbyssLockPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    bStartPlayersAsSpectators = true;
}
