#include "AbyssMainMenuWidget.h"

#include "AbyssLockGameState.h"
#include "AbyssLockPlayerController.h"
#include "AbyssLockPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr int32 AbyssLobbyRequiredPlayers = 8;

UTextBlock* MakeText(UWidgetTree* WidgetTree, const FText& Text, float FontSize)
{
    UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TextBlock->SetText(Text);
    FSlateFontInfo Font = TextBlock->GetFont();
    Font.Size = FontSize;
    TextBlock->SetFont(Font);
    TextBlock->SetJustification(ETextJustify::Center);
    return TextBlock;
}

UButton* MakeButton(UWidgetTree* WidgetTree, const FText& Text)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    UTextBlock* Label = MakeText(WidgetTree, Text, 24.0f);
    Button->AddChild(Label);
    return Button;
}

void AddBoxChild(UVerticalBox* Box, UWidget* Child, float TopPadding = 12.0f)
{
    if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child))
    {
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
    }
}
}

void UAbyssMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BuildWidgetTree();
    ShowStartScreen();
}

void UAbyssMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (CurrentScreen == EAbyssFrontEndScreen::Lobby)
    {
        RefreshLobbyState();
    }
}

void UAbyssMainMenuWidget::BuildWidgetTree()
{
    if (!WidgetTree || RootBox)
    {
        return;
    }

    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    WidgetTree->RootWidget = RootOverlay;

    // Full-screen dark backdrop so the front-end reads as its own menu screen instead of
    // showing the 3D whitebox behind it. It is removed with the widget when a match starts.
    UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Background->SetBrushColor(FLinearColor(0.012f, 0.02f, 0.035f, 1.0f));
    if (UOverlaySlot* BackgroundSlot = RootOverlay->AddChildToOverlay(Background))
    {
        BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
        BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
    }

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UOverlaySlot* MenuSlot = RootOverlay->AddChildToOverlay(RootBox))
    {
        MenuSlot->SetHorizontalAlignment(HAlign_Center);
        MenuSlot->SetVerticalAlignment(VAlign_Fill);
    }

    TitleText = MakeText(WidgetTree, FText::FromString(TEXT("Frostwake")), 44.0f);
    StatusText = MakeText(WidgetTree, FText::GetEmpty(), 18.0f);
    PlayerCountText = MakeText(WidgetTree, FText::GetEmpty(), 22.0f);

    GameStartButton = MakeButton(WidgetTree, FText::FromString(TEXT("ゲーム開始")));
    SoloModeButton = MakeButton(WidgetTree, FText::FromString(TEXT("一人モード")));
    HostLobbyButton = MakeButton(WidgetTree, FText::FromString(TEXT("ロビーを開く")));
    JoinLobbyButton = MakeButton(WidgetTree, FText::FromString(TEXT("ロビーに入る")));
    HostStartButton = MakeButton(WidgetTree, FText::FromString(TEXT("開始")));

    JoinAddressText = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    JoinAddressText->SetText(FText::FromString(TEXT("127.0.0.1")));
    JoinAddressText->SetHintText(FText::FromString(TEXT("接続先アドレス")));

    GameStartButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleGameStartClicked);
    SoloModeButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleSoloModeClicked);
    HostLobbyButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleHostLobbyClicked);
    JoinLobbyButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleJoinLobbyClicked);
    HostStartButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleHostStartClicked);

    AddBoxChild(RootBox, TitleText, 160.0f);
    AddBoxChild(RootBox, StatusText, 24.0f);
    AddBoxChild(RootBox, PlayerCountText);
    AddBoxChild(RootBox, JoinAddressText);
    AddBoxChild(RootBox, GameStartButton);
    AddBoxChild(RootBox, SoloModeButton);
    AddBoxChild(RootBox, HostLobbyButton);
    AddBoxChild(RootBox, JoinLobbyButton);
    AddBoxChild(RootBox, HostStartButton);
}

void UAbyssMainMenuWidget::ShowStartScreen()
{
    CurrentScreen = EAbyssFrontEndScreen::Start;
    TitleText->SetText(FText::FromString(TEXT("Frostwake")));
    StatusText->SetText(FText::GetEmpty());
    PlayerCountText->SetVisibility(ESlateVisibility::Collapsed);
    JoinAddressText->SetVisibility(ESlateVisibility::Collapsed);
    GameStartButton->SetVisibility(ESlateVisibility::Visible);
    SoloModeButton->SetVisibility(ESlateVisibility::Collapsed);
    HostLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    HostStartButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UAbyssMainMenuWidget::ShowLobbyChoiceScreen()
{
    CurrentScreen = EAbyssFrontEndScreen::LobbyChoice;
    StatusText->SetText(FText::FromString(TEXT("ロビーを作成するか、既存のロビーに参加してください。")));
    PlayerCountText->SetVisibility(ESlateVisibility::Collapsed);
    JoinAddressText->SetVisibility(ESlateVisibility::Visible);
    GameStartButton->SetVisibility(ESlateVisibility::Collapsed);
    SoloModeButton->SetVisibility(ESlateVisibility::Visible);
    HostLobbyButton->SetVisibility(ESlateVisibility::Visible);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Visible);
    HostStartButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UAbyssMainMenuWidget::ShowLobbyScreen()
{
    CurrentScreen = EAbyssFrontEndScreen::Lobby;
    PlayerCountText->SetVisibility(ESlateVisibility::Visible);
    JoinAddressText->SetVisibility(ESlateVisibility::Collapsed);
    GameStartButton->SetVisibility(ESlateVisibility::Collapsed);
    SoloModeButton->SetVisibility(ESlateVisibility::Collapsed);
    HostLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    HostStartButton->SetVisibility(IsLocalPlayerHost() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    RefreshLobbyState();
}

void UAbyssMainMenuWidget::RefreshLobbyState()
{
    const int32 PlayerCount = GetConnectedPlayerCount();
    PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), PlayerCount, AbyssLobbyRequiredPlayers)));

    if (const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr)
    {
        if (AbyssGameState->GetMatchPhase() != EAbyssMatchPhase::WaitingForPlayers)
        {
            EnterGameInputAndClose();
            return;
        }
    }

    const bool bHost = IsLocalPlayerHost();
    const bool bReadyToStart = bHost && PlayerCount == AbyssLobbyRequiredPlayers;
    HostStartButton->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    HostStartButton->SetIsEnabled(bReadyToStart);

    if (bReadyToStart)
    {
        StatusText->SetText(FText::FromString(TEXT("8人揃いました。ホストが開始できます。")));
    }
    else if (bHost)
    {
        StatusText->SetText(FText::FromString(TEXT("8人揃うまで待機中です。")));
    }
    else
    {
        StatusText->SetText(FText::FromString(TEXT("ホストの開始を待っています。")));
    }
}

void UAbyssMainMenuWidget::HandleGameStartClicked()
{
    ShowLobbyChoiceScreen();
}

void UAbyssMainMenuWidget::HandleHostLobbyClicked()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        PlayerController->ConsoleCommand(TEXT("open /Game/Maps/L_IcebreakerWhitebox?listen"));
    }
    ShowLobbyScreen();
}

void UAbyssMainMenuWidget::HandleSoloModeClicked()
{
    if (AAbyssLockPlayerController* PlayerController = Cast<AAbyssLockPlayerController>(GetOwningPlayer()))
    {
        PlayerController->RequestSoloMatch();
        EnterGameInputAndClose();
    }
}

void UAbyssMainMenuWidget::HandleJoinLobbyClicked()
{
    const FString Address = JoinAddressText ? JoinAddressText->GetText().ToString().TrimStartAndEnd() : FString();
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        PlayerController->ConsoleCommand(FString::Printf(TEXT("open %s"), Address.IsEmpty() ? TEXT("127.0.0.1") : *Address));
    }
}

void UAbyssMainMenuWidget::HandleHostStartClicked()
{
    if (AAbyssLockPlayerController* PlayerController = Cast<AAbyssLockPlayerController>(GetOwningPlayer()))
    {
        PlayerController->RequestHostStartMatch();
    }
}

bool UAbyssMainMenuWidget::IsLocalPlayerHost() const
{
    const APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController || !PlayerController->HasAuthority())
    {
        return false;
    }

    const AGameStateBase* CurrentGameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    return CurrentGameState && CurrentGameState->PlayerArray.Num() > 0 && CurrentGameState->PlayerArray[0] == PlayerController->PlayerState;
}

int32 UAbyssMainMenuWidget::GetConnectedPlayerCount() const
{
    const AGameStateBase* CurrentGameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    return CurrentGameState ? CurrentGameState->PlayerArray.Num() : 0;
}

void UAbyssMainMenuWidget::EnterGameInputAndClose()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        PlayerController->bShowMouseCursor = false;
        PlayerController->SetInputMode(FInputModeGameOnly());
    }

    RemoveFromParent();
}
