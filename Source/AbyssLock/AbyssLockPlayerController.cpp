#include "AbyssLockPlayerController.h"
#include "AbyssInteractionComponent.h"
#include "AbyssLockCharacter.h"
#include "AbyssLockGameMode.h"
#include "AbyssLockLog.h"
#include "AbyssMainMenuWidget.h"
#include "AbyssLockPlayerState.h"
#include "AbyssTelemetrySubsystem.h"
#include "Engine/GameInstance.h"

AAbyssLockPlayerController::AAbyssLockPlayerController()
{
    bReplicates = true;
}

void AAbyssLockPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController() || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    MainMenuWidget = CreateWidget<UAbyssMainMenuWidget>(this, UAbyssMainMenuWidget::StaticClass());
    if (MainMenuWidget)
    {
        MainMenuWidget->AddToViewport(10);
        bShowMouseCursor = true;
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
        SetInputMode(InputMode);

        if (GetNetMode() != NM_Standalone)
        {
            MainMenuWidget->ShowLobbyScreen();
        }
    }
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

void AAbyssLockPlayerController::RequestHostStartMatch()
{
    if (HasAuthority())
    {
        ServerRequestHostStartMatch_Implementation();
        return;
    }

    ServerRequestHostStartMatch();
}

void AAbyssLockPlayerController::RequestSoloMatch()
{
    if (HasAuthority())
    {
        ServerRequestSoloMatch_Implementation();
        return;
    }

    ServerRequestSoloMatch();
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

void AAbyssLockPlayerController::ServerRequestHostStartMatch_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        if (AAbyssLockGameMode* AbyssGameMode = World->GetAuthGameMode<AAbyssLockGameMode>())
        {
            AbyssGameMode->TryStartMatchFromHost(this);
        }
    }
}

void AAbyssLockPlayerController::ServerRequestSoloMatch_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        if (AAbyssLockGameMode* AbyssGameMode = World->GetAuthGameMode<AAbyssLockGameMode>())
        {
            AbyssGameMode->TryStartSoloMatchFromMenu(this);
        }
    }
}
