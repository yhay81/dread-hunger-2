#include "FrostwakePlayerController.h"
#include "FrostwakeInteractionComponent.h"
#include "FrostwakeCharacter.h"
#include "FrostwakeGameMode.h"
#include "FrostwakeLog.h"
#include "FrostwakeHudWidget.h"
#include "FrostwakeMainMenuWidget.h"
#include "FrostwakePlayerState.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AFrostwakePlayerController::AFrostwakePlayerController()
{
    bReplicates = true;
}

void AFrostwakePlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController() || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    const FString MapName = GetWorld() ? GetWorld()->GetMapName() : FString();
    const bool bOnMenuMap = MapName.Contains(TEXT("L_MainMenu"));

    // Reaching a gameplay map in standalone is the solo path: the GameMode auto-starts the
    // practice match, so show no menu and hand control straight to the player.
    if (!bOnMenuMap && GetNetMode() == NM_Standalone)
    {
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());

        // Solo/practice: show a minimal in-match HUD so the round reads as a game.
        HudWidget = CreateWidget<UFrostwakeHudWidget>(this, UFrostwakeHudWidget::StaticClass());
        if (HudWidget)
        {
            HudWidget->AddToViewport(0);
        }
        return;
    }

    MainMenuWidget = CreateWidget<UFrostwakeMainMenuWidget>(this, UFrostwakeMainMenuWidget::StaticClass());
    if (MainMenuWidget)
    {
        MainMenuWidget->AddToViewport(10);
        bShowMouseCursor = true;
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
        SetInputMode(InputMode);

        // Joined a hosted lobby (listen/client) rather than the menu map: go to the lobby screen.
        if (!bOnMenuMap)
        {
            MainMenuWidget->ShowLobbyScreen();
        }
    }
}

void AFrostwakePlayerController::TryPrimaryInteract()
{
    AFrostwakeCharacter* FrostwakeCharacter = Cast<AFrostwakeCharacter>(GetPawn());
    if (!FrostwakeCharacter)
    {
        return;
    }

    if (UFrostwakeInteractionComponent* InteractionComponent = FrostwakeCharacter->GetInteractionComponent())
    {
        InteractionComponent->TryInteract();
    }
}

void AFrostwakePlayerController::SetReadyForMatch(bool bReady)
{
    if (HasAuthority())
    {
        ServerSetReadyForMatch_Implementation(bReady);
        return;
    }

    ServerSetReadyForMatch(bReady);
}

void AFrostwakePlayerController::RequestHostStartMatch()
{
    if (HasAuthority())
    {
        ServerRequestHostStartMatch_Implementation();
        return;
    }

    ServerRequestHostStartMatch();
}

void AFrostwakePlayerController::RequestSoloMatch()
{
    if (HasAuthority())
    {
        ServerRequestSoloMatch_Implementation();
        return;
    }

    ServerRequestSoloMatch();
}

void AFrostwakePlayerController::ServerSetReadyForMatch_Implementation(bool bReady)
{
    AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (!FrostwakePlayerState)
    {
        return;
    }

    FrostwakePlayerState->SetReadyForMatch(bReady);
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_ready_changed player=%s ready=%s"), *GetNameSafe(FrostwakePlayerState), bReady ? TEXT("true") : TEXT("false"));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_ready_changed"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"ready\":%s}"), *GetNameSafe(FrostwakePlayerState), bReady ? TEXT("true") : TEXT("false")));
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (AFrostwakeGameMode* FrostwakeGameMode = World->GetAuthGameMode<AFrostwakeGameMode>())
        {
            FrostwakeGameMode->TryStartMatchFromReady();
        }
    }
}

void AFrostwakePlayerController::ServerRequestHostStartMatch_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        if (AFrostwakeGameMode* FrostwakeGameMode = World->GetAuthGameMode<AFrostwakeGameMode>())
        {
            FrostwakeGameMode->TryStartMatchFromHost(this);
        }
    }
}

void AFrostwakePlayerController::ServerRequestSoloMatch_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        if (AFrostwakeGameMode* FrostwakeGameMode = World->GetAuthGameMode<AFrostwakeGameMode>())
        {
            FrostwakeGameMode->TryStartSoloMatchFromMenu(this);
        }
    }
}
