#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbyssMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;

UENUM(BlueprintType)
enum class EAbyssFrontEndScreen : uint8
{
    Start,
    LobbyChoice,
    Lobby
};

UCLASS()
class ABYSSLOCK_API UAbyssMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void ShowLobbyScreen();

protected:
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

    EAbyssFrontEndScreen CurrentScreen = EAbyssFrontEndScreen::Start;

    void BuildWidgetTree();
    void ShowStartScreen();
    void ShowLobbyChoiceScreen();
    void RefreshLobbyState();
    bool IsLocalPlayerHost() const;
    int32 GetConnectedPlayerCount() const;
    void EnterGameInputAndClose();
};
