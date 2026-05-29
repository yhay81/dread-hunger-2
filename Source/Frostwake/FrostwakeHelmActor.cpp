#include "FrostwakeHelmActor.h"

#include "FrostwakeGameState.h"
#include "FrostwakeLog.h"
#include "Engine/World.h"
#include "Internationalization/Text.h"

AFrostwakeHelmActor::AFrostwakeHelmActor()
{
    InteractionText = FText::FromString(TEXT("Take the Helm"));
}

bool AFrostwakeHelmActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !GetWorld())
    {
        return false;
    }

    AFrostwakeGameState* GameState = GetWorld()->GetGameState<AFrostwakeGameState>();
    if (!GameState)
    {
        return false;
    }

    const bool bNowManned = !GameState->IsHelmManned();
    GameState->SetHelmManned(bNowManned);
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("helm_toggled manned=%d"), bNowManned ? 1 : 0);
    return true;
}
