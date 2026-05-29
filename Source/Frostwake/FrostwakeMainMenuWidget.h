#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FrostwakeTypes.h"
#include "FrostwakeMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;

UENUM(BlueprintType)
enum class EFrostwakeFrontEndScreen : uint8
{
    Start,
    LobbyChoice,
    SoloConfig,
    HostConfig,
    Lobby
};

UCLASS()
class FROSTWAKE_API UFrostwakeMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void ShowLobbyScreen();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

    UFUNCTION()
    void HandleGameStartClicked();

    UFUNCTION()
    void HandleHostLobbyClicked();

    UFUNCTION()
    void HandleSoloModeClicked();

    UFUNCTION()
    void HandleJoinLobbyClicked();

    UFUNCTION()
    void HandleHostStartClicked();

    UFUNCTION()
    void HandleModeCycleClicked();

    UFUNCTION()
    void HandleDifficultyCycleClicked();

    UFUNCTION()
    void HandleConfigConfirmClicked();

private:
    UPROPERTY()
    UVerticalBox* RootBox = nullptr;

    UPROPERTY()
    UTextBlock* TitleText = nullptr;

    UPROPERTY()
    UTextBlock* StatusText = nullptr;

    UPROPERTY()
    UTextBlock* PlayerCountText = nullptr;

    UPROPERTY()
    UEditableTextBox* JoinAddressText = nullptr;

    UPROPERTY()
    UButton* GameStartButton = nullptr;

    UPROPERTY()
    UButton* HostLobbyButton = nullptr;

    UPROPERTY()
    UButton* SoloModeButton = nullptr;

    UPROPERTY()
    UButton* JoinLobbyButton = nullptr;

    UPROPERTY()
    UButton* HostStartButton = nullptr;

    // Host match-config selectors (cycle buttons) shown on the lobby-choice screen.
    UPROPERTY()
    UButton* ModeButton = nullptr;
    UPROPERTY()
    UTextBlock* ModeButtonLabel = nullptr;
    UPROPERTY()
    UButton* DifficultyButton = nullptr;
    UPROPERTY()
    UTextBlock* DifficultyButtonLabel = nullptr;

    // Confirm button shown on the solo/host config screen (label switches: "Start Voyage" / "Host Lobby").
    UPROPERTY()
    UButton* ConfigConfirmButton = nullptr;
    UPROPERTY()
    UTextBlock* ConfigConfirmLabel = nullptr;

    EFrostwakeFrontEndScreen CurrentScreen = EFrostwakeFrontEndScreen::Start;

    // Host's chosen match configuration, carried into the travel URL (?mode=.. ?difficulty=..).
    EFrostwakeMatchMode SelectedMode = EFrostwakeMatchMode::Standard;
    EFrostwakeDifficulty SelectedDifficulty = EFrostwakeDifficulty::Normal;

    void BuildWidgetTree();
    void ShowStartScreen();
    void ShowLobbyChoiceScreen();
    void ShowSoloConfigScreen();
    void ShowHostConfigScreen();
    void RefreshLobbyState();
    bool IsLocalPlayerHost() const;
    int32 GetConnectedPlayerCount() const;
    void EnterGameInputAndClose();
    void UpdateConfigButtonLabels();
    FString BuildConfigUrlOptions(bool bIncludeMode) const;
};
