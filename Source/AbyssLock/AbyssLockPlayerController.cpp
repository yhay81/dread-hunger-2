#include "AbyssLockPlayerController.h"
#include "AbyssInteractionComponent.h"
#include "AbyssLockCharacter.h"
#include "AbyssLockGameMode.h"
#include "AbyssLockLog.h"
#include "AbyssLockPlayerState.h"
#include "AbyssTelemetrySubsystem.h"
#include "Engine/GameInstance.h"

AAbyssLockPlayerController::AAbyssLockPlayerController()
{
    bReplicates = true;
}

void AAbyssLockPlayerController::TryPrimaryInteract()
{
    AAbyssLockCharacter* AbyssCharacter = Cast<AAbyssLockCharacter>(GetPawn());
    if (!AbyssCharacter)
    {
        return;
    }

    if (UAbyssInteractionComponent* InteractionComponent = AbyssCharacter->GetInteractionComponent())
    {
        InteractionComponent->TryInteract();
    }
}

void AAbyssLockPlayerController::SetReadyForMatch(bool bReady)
{
    if (HasAuthority())
    {
        ServerSetReadyForMatch_Implementation(bReady);
        return;
    }

    ServerSetReadyForMatch(bReady);
}

void AAbyssLockPlayerController::ServerSetReadyForMatch_Implementation(bool bReady)
{
    AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (!AbyssPlayerState)
    {
        return;
    }

    AbyssPlayerState->SetReadyForMatch(bReady);
    UE_LOG(LogAbyssGameplay, Log, TEXT("player_ready_changed player=%s ready=%s"), *GetNameSafe(AbyssPlayerState), bReady ? TEXT("true") : TEXT("false"));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_ready_changed"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"ready\":%s}"), *GetNameSafe(AbyssPlayerState), bReady ? TEXT("true") : TEXT("false")));
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (AAbyssLockGameMode* AbyssGameMode = World->GetAuthGameMode<AAbyssLockGameMode>())
        {
            AbyssGameMode->TryStartMatchFromReady();
        }
    }
}
