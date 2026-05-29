#include "FrostwakeLifeActionActor.h"
#include "FrostwakeCharacter.h"
#include "FrostwakeGameState.h"
#include "FrostwakeLog.h"
#include "FrostwakePlayerState.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

AFrostwakeLifeActionActor::AFrostwakeLifeActionActor()
{
    bReplicates = true;
    Action = EFrostwakeLifeAction::RescueDowned;
    InteractionText = FText::FromString(TEXT("Assist"));
}

void AFrostwakeLifeActionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFrostwakeLifeActionActor, TargetCharacter);
    DOREPLIFETIME(AFrostwakeLifeActionActor, Action);
}

void AFrostwakeLifeActionActor::ConfigureLifeAction(AFrostwakeCharacter* InTargetCharacter, EFrostwakeLifeAction InAction)
{
    if (!HasAuthority())
    {
        return;
    }

    TargetCharacter = InTargetCharacter;
    Action = InAction;
}

bool AFrostwakeLifeActionActor::CanInteract(const APawn* InstigatorPawn) const
{
    if (!Super::CanInteract(InstigatorPawn) || !TargetCharacter || InstigatorPawn == TargetCharacter)
    {
        return false;
    }

    const AFrostwakeGameState* FrostwakeGameState = GetWorld() ? GetWorld()->GetGameState<AFrostwakeGameState>() : nullptr;
    if (!FrostwakeGameState || !FrostwakeGameState->IsMatchActive())
    {
        return false;
    }

    const AFrostwakePlayerState* TargetPlayerState = TargetCharacter->GetPlayerState<AFrostwakePlayerState>();
    if (!TargetPlayerState)
    {
        return false;
    }

    switch (Action)
    {
        case EFrostwakeLifeAction::RescueDowned:
            return TargetPlayerState->GetLifeState() == EFrostwakeLifeState::Downed;
        case EFrostwakeLifeAction::ContainAlive:
            return TargetPlayerState->GetLifeState() == EFrostwakeLifeState::Alive;
        case EFrostwakeLifeAction::ReleaseContained:
            return TargetPlayerState->GetLifeState() == EFrostwakeLifeState::Contained;
        default:
            return false;
    }
}

bool AFrostwakeLifeActionActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !CanInteract(InstigatorPawn))
    {
        return false;
    }

    bool bSucceeded = false;
    switch (Action)
    {
        case EFrostwakeLifeAction::RescueDowned:
            bSucceeded = TargetCharacter->RescueFromDowned(InstigatorPawn);
            break;
        case EFrostwakeLifeAction::ContainAlive:
            bSucceeded = TargetCharacter->ContainPlayer(InstigatorPawn);
            break;
        case EFrostwakeLifeAction::ReleaseContained:
            bSucceeded = TargetCharacter->ReleaseFromContainment(InstigatorPawn);
            break;
        default:
            bSucceeded = false;
            break;
    }

    UE_LOG(
        LogFrostwakeGameplay,
        Log,
        TEXT("life_action_interacted action=%d target=%s instigator=%s result=%s"),
        static_cast<int32>(Action),
        *GetNameSafe(TargetCharacter),
        *GetNameSafe(InstigatorPawn),
        bSucceeded ? TEXT("success") : TEXT("failed"));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("life_action_interacted"),
                FString::Printf(
                    TEXT("{\"action\":%d,\"target\":\"%s\",\"instigator\":\"%s\",\"result\":\"%s\"}"),
                    static_cast<int32>(Action),
                    *GetNameSafe(TargetCharacter),
                    *GetNameSafe(InstigatorPawn),
                    bSucceeded ? TEXT("success") : TEXT("failed")));
        }
    }

    return bSucceeded;
}
