#include "AbyssHelmActor.h"

#include "AbyssLockGameState.h"
#include "AbyssLockLog.h"
#include "Engine/World.h"
#include "Internationalization/Text.h"

AAbyssHelmActor::AAbyssHelmActor()
{
    InteractionText = FText::FromString(TEXT("Take the Helm"));
}

bool AAbyssHelmActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !GetWorld())
    {
        return false;
    }

    AAbyssLockGameState* GameState = GetWorld()->GetGameState<AAbyssLockGameState>();
    if (!GameState)
    {
        return false;
    }

    const bool bNowManned = !GameState->IsHelmManned();
    GameState->SetHelmManned(bNowManned);
    UE_LOG(LogAbyssGameplay, Log, TEXT("helm_toggled manned=%d"), bNowManned ? 1 : 0);
    return true;
}
