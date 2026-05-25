#include "AbyssLifeActionActor.h"
#include "AbyssLockCharacter.h"
#include "AbyssLockGameState.h"
#include "AbyssLockLog.h"
#include "AbyssLockPlayerState.h"
#include "AbyssTelemetrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

AAbyssLifeActionActor::AAbyssLifeActionActor()
{
    bReplicates = true;
    Action = EAbyssLifeAction::RescueDowned;
    InteractionText = FText::FromString(TEXT("Assist"));
}

void AAbyssLifeActionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssLifeActionActor, TargetCharacter);
    DOREPLIFETIME(AAbyssLifeActionActor, Action);
}

void AAbyssLifeActionActor::ConfigureLifeAction(AAbyssLockCharacter* InTargetCharacter, EAbyssLifeAction InAction)
{
    if (!HasAuthority())
    {
        return;
    }

    TargetCharacter = InTargetCharacter;
    Action = InAction;
}

bool AAbyssLifeActionActor::CanInteract(const APawn* InstigatorPawn) const
{
    if (!Super::CanInteract(InstigatorPawn) || !TargetCharacter || InstigatorPawn == TargetCharacter)
    {
        return false;
    }

    const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
    if (!AbyssGameState || !AbyssGameState->IsMatchActive())
    {
        return false;
    }

    const AAbyssLockPlayerState* TargetPlayerState = TargetCharacter->GetPlayerState<AAbyssLockPlayerState>();
    if (!TargetPlayerState)
    {
        return false;
    }

    switch (Action)
    {
        case EAbyssLifeAction::RescueDowned:
            return TargetPlayerState->GetLifeState() == EAbyssLifeState::Downed;
        case EAbyssLifeAction::ContainAlive:
            return TargetPlayerState->GetLifeState() == EAbyssLifeState::Alive;
        case EAbyssLifeAction::ReleaseContained:
            return TargetPlayerState->GetLifeState() == EAbyssLifeState::Contained;
        default:
            return false;
    }
}

bool AAbyssLifeActionActor::Interact(APawn* InstigatorPawn)
{
    if (!HasAuthority() || !CanInteract(InstigatorPawn))
    {
        return false;
    }

    bool bSucceeded = false;
    switch (Action)
    {
        case EAbyssLifeAction::RescueDowned:
            bSucceeded = TargetCharacter->RescueFromDowned(InstigatorPawn);
            break;
        case EAbyssLifeAction::ContainAlive:
            bSucceeded = TargetCharacter->ContainPlayer(InstigatorPawn);
            break;
        case EAbyssLifeAction::ReleaseContained:
            bSucceeded = TargetCharacter->ReleaseFromContainment(InstigatorPawn);
            break;
        default:
            bSucceeded = false;
            break;
    }

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("life_action_interacted action=%d target=%s instigator=%s result=%s"),
        static_cast<int32>(Action),
        *GetNameSafe(TargetCharacter),
        *GetNameSafe(InstigatorPawn),
        bSucceeded ? TEXT("success") : TEXT("failed"));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
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
