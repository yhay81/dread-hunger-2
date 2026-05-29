#include "FrostwakeMainMenuWidget.h"

#include "FrostwakeGameState.h"
#include "FrostwakeUIText.h"
#include "FrostwakePlayerController.h"
#include "FrostwakePlayerState.h"
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
constexpr int32 FrostwakeLobbyRequiredPlayers = 8;

UTextBlock* MakeText(UWidgetTree* WidgetTree, const FText& Text, float FontSize)
{
    UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TextBlock->SetText(Text);
    TextBlock->SetFont(FrostwakeUIText::UiFont(FontSize));
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

// A button whose label is kept (OutLabel) so it can be re-set as the value cycles.
UButton* MakeCycleButton(UWidgetTree* WidgetTree, UTextBlock*& OutLabel)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    OutLabel = MakeText(WidgetTree, FText::GetEmpty(), 20.0f);
    Button->AddChild(OutLabel);
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

TSharedRef<SWidget> UFrostwakeMainMenuWidget::RebuildWidget()
{
    // This is a code-only UUserWidget (no UMG asset), so it has no WidgetTree by default.
    // Create one and build the menu hierarchy before Slate is constructed, otherwise the
    // widget renders nothing (a black screen).
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    BuildWidgetTree();
    return Super::RebuildWidget();
}

void UFrostwakeMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);
    BuildWidgetTree();
    ShowStartScreen();

    UE_LOG(LogTemp, Log, TEXT("frontend_menu_construct rootbox_children=%d"), RootBox ? RootBox->GetChildrenCount() : -1);
}

void UFrostwakeMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (CurrentScreen == EFrostwakeFrontEndScreen::Lobby)
    {
        RefreshLobbyState();
    }
}

void UFrostwakeMainMenuWidget::BuildWidgetTree()
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

    GameStartButton = MakeButton(WidgetTree, FrostwakeUIText::Text(TEXT("Menu_Play")));
    SoloModeButton = MakeButton(WidgetTree, FrostwakeUIText::Text(TEXT("Menu_SoloPractice")));
    HostLobbyButton = MakeButton(WidgetTree, FrostwakeUIText::Text(TEXT("Menu_HostLobby")));
    JoinLobbyButton = MakeButton(WidgetTree, FrostwakeUIText::Text(TEXT("Menu_JoinLobby")));
    HostStartButton = MakeButton(WidgetTree, FrostwakeUIText::Text(TEXT("Menu_StartMatch")));
    ModeButton = MakeCycleButton(WidgetTree, ModeButtonLabel);
    DifficultyButton = MakeCycleButton(WidgetTree, DifficultyButtonLabel);
    ConfigConfirmButton = MakeCycleButton(WidgetTree, ConfigConfirmLabel);

    JoinAddressText = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    JoinAddressText->SetText(FText::FromString(TEXT("127.0.0.1")));
    JoinAddressText->SetHintText(FrostwakeUIText::Text(TEXT("Menu_AddressHint")));

    GameStartButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleGameStartClicked);
    SoloModeButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleSoloModeClicked);
    HostLobbyButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleHostLobbyClicked);
    JoinLobbyButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleJoinLobbyClicked);
    HostStartButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleHostStartClicked);
    ModeButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleModeCycleClicked);
    DifficultyButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleDifficultyCycleClicked);
    ConfigConfirmButton->OnClicked.AddDynamic(this, &UFrostwakeMainMenuWidget::HandleConfigConfirmClicked);

    AddBoxChild(RootBox, TitleText, 160.0f);
    AddBoxChild(RootBox, StatusText, 24.0f);
    AddBoxChild(RootBox, PlayerCountText);
    AddBoxChild(RootBox, JoinAddressText);
    // Match-config selectors + confirm: shown on the solo/host config screen (after Solo/Host is chosen).
    AddBoxChild(RootBox, ModeButton);
    AddBoxChild(RootBox, DifficultyButton);
    AddBoxChild(RootBox, ConfigConfirmButton);
    AddBoxChild(RootBox, GameStartButton);
    AddBoxChild(RootBox, SoloModeButton);
    AddBoxChild(RootBox, HostLobbyButton);
    AddBoxChild(RootBox, JoinLobbyButton);
    AddBoxChild(RootBox, HostStartButton);

    UpdateConfigButtonLabels();
}

void UFrostwakeMainMenuWidget::ShowStartScreen()
{
    CurrentScreen = EFrostwakeFrontEndScreen::Start;
    TitleText->SetText(FText::FromString(TEXT("Frostwake")));
    StatusText->SetText(FText::GetEmpty());
    PlayerCountText->SetVisibility(ESlateVisibility::Collapsed);
    JoinAddressText->SetVisibility(ESlateVisibility::Collapsed);
    GameStartButton->SetVisibility(ESlateVisibility::Visible);
    SoloModeButton->SetVisibility(ESlateVisibility::Collapsed);
    HostLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    HostStartButton->SetVisibility(ESlateVisibility::Collapsed);
    ModeButton->SetVisibility(ESlateVisibility::Collapsed);
    DifficultyButton->SetVisibility(ESlateVisibility::Collapsed);
    ConfigConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UFrostwakeMainMenuWidget::ShowLobbyChoiceScreen()
{
    CurrentScreen = EFrostwakeFrontEndScreen::LobbyChoice;
    StatusText->SetText(FrostwakeUIText::Text(TEXT("Menu_LobbyChoicePrompt")));
    PlayerCountText->SetVisibility(ESlateVisibility::Collapsed);
    JoinAddressText->SetVisibility(ESlateVisibility::Visible);
    GameStartButton->SetVisibility(ESlateVisibility::Collapsed);
    SoloModeButton->SetVisibility(ESlateVisibility::Visible);
    HostLobbyButton->SetVisibility(ESlateVisibility::Visible);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Visible);
    HostStartButton->SetVisibility(ESlateVisibility::Collapsed);
    // Config selectors appear AFTER choosing Solo / Host (on the config screen), not here.
    ModeButton->SetVisibility(ESlateVisibility::Collapsed);
    DifficultyButton->SetVisibility(ESlateVisibility::Collapsed);
    ConfigConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UFrostwakeMainMenuWidget::ShowSoloConfigScreen()
{
    CurrentScreen = EFrostwakeFrontEndScreen::SoloConfig;
    StatusText->SetText(FrostwakeUIText::Text(TEXT("Menu_ConfigPrompt")));
    PlayerCountText->SetVisibility(ESlateVisibility::Collapsed);
    JoinAddressText->SetVisibility(ESlateVisibility::Collapsed);
    GameStartButton->SetVisibility(ESlateVisibility::Collapsed);
    SoloModeButton->SetVisibility(ESlateVisibility::Collapsed);
    HostLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    HostStartButton->SetVisibility(ESlateVisibility::Collapsed);
    // Solo = always Standard, so only the difficulty selector + Start are shown (no mode selector).
    ModeButton->SetVisibility(ESlateVisibility::Collapsed);
    DifficultyButton->SetVisibility(ESlateVisibility::Visible);
    ConfigConfirmButton->SetVisibility(ESlateVisibility::Visible);
    ConfigConfirmLabel->SetText(FrostwakeUIText::Text(TEXT("Menu_StartSolo")));
    UpdateConfigButtonLabels();
}

void UFrostwakeMainMenuWidget::ShowHostConfigScreen()
{
    CurrentScreen = EFrostwakeFrontEndScreen::HostConfig;
    StatusText->SetText(FrostwakeUIText::Text(TEXT("Menu_ConfigPrompt")));
    PlayerCountText->SetVisibility(ESlateVisibility::Collapsed);
    JoinAddressText->SetVisibility(ESlateVisibility::Collapsed);
    GameStartButton->SetVisibility(ESlateVisibility::Collapsed);
    SoloModeButton->SetVisibility(ESlateVisibility::Collapsed);
    HostLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    HostStartButton->SetVisibility(ESlateVisibility::Collapsed);
    // Host = mode + difficulty, then confirm to open the listen lobby.
    ModeButton->SetVisibility(ESlateVisibility::Visible);
    DifficultyButton->SetVisibility(ESlateVisibility::Visible);
    ConfigConfirmButton->SetVisibility(ESlateVisibility::Visible);
    ConfigConfirmLabel->SetText(FrostwakeUIText::Text(TEXT("Menu_HostLobby")));
    UpdateConfigButtonLabels();
}

void UFrostwakeMainMenuWidget::ShowLobbyScreen()
{
    CurrentScreen = EFrostwakeFrontEndScreen::Lobby;
    PlayerCountText->SetVisibility(ESlateVisibility::Visible);
    JoinAddressText->SetVisibility(ESlateVisibility::Collapsed);
    GameStartButton->SetVisibility(ESlateVisibility::Collapsed);
    SoloModeButton->SetVisibility(ESlateVisibility::Collapsed);
    HostLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    JoinLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
    HostStartButton->SetVisibility(IsLocalPlayerHost() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ModeButton->SetVisibility(ESlateVisibility::Collapsed);
    DifficultyButton->SetVisibility(ESlateVisibility::Collapsed);
    ConfigConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
    RefreshLobbyState();
}

void UFrostwakeMainMenuWidget::RefreshLobbyState()
{
    const int32 PlayerCount = GetConnectedPlayerCount();
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Current"), PlayerCount);
        Args.Add(TEXT("Required"), FrostwakeLobbyRequiredPlayers);
        PlayerCountText->SetText(FText::Format(FrostwakeUIText::Text(TEXT("Menu_FmtPlayerCount")), Args));
    }

    if (const AFrostwakeGameState* FrostwakeGameState = GetWorld() ? GetWorld()->GetGameState<AFrostwakeGameState>() : nullptr)
    {
        if (FrostwakeGameState->GetMatchPhase() != EFrostwakeMatchPhase::WaitingForPlayers)
        {
            EnterGameInputAndClose();
            return;
        }
    }

    const bool bHost = IsLocalPlayerHost();
    const bool bReadyToStart = bHost && PlayerCount == FrostwakeLobbyRequiredPlayers;
    HostStartButton->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    HostStartButton->SetIsEnabled(bReadyToStart);

    if (bReadyToStart)
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Required"), FrostwakeLobbyRequiredPlayers);
        StatusText->SetText(FText::Format(FrostwakeUIText::Text(TEXT("Menu_FmtLobbyReady")), Args));
    }
    else if (bHost)
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Required"), FrostwakeLobbyRequiredPlayers);
        StatusText->SetText(FText::Format(FrostwakeUIText::Text(TEXT("Menu_FmtLobbyWaiting")), Args));
    }
    else
    {
        StatusText->SetText(FrostwakeUIText::Text(TEXT("Menu_WaitingForHost")));
    }
}

void UFrostwakeMainMenuWidget::HandleGameStartClicked()
{
    ShowLobbyChoiceScreen();
}

void UFrostwakeMainMenuWidget::HandleHostLobbyClicked()
{
    // Choosing "Host" opens the host config screen (mode + difficulty); travel happens on confirm.
    ShowHostConfigScreen();
}

void UFrostwakeMainMenuWidget::HandleSoloModeClicked()
{
    // Choosing "Solo" opens the solo config screen (difficulty only); travel happens on confirm.
    ShowSoloConfigScreen();
}

void UFrostwakeMainMenuWidget::HandleConfigConfirmClicked()
{
    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        return;
    }

    if (CurrentScreen == EFrostwakeFrontEndScreen::SoloConfig)
    {
        // Solo standalone practice: InitGame reads "solo" + the chosen difficulty (Standard forced).
        PlayerController->ConsoleCommand(FString::Printf(TEXT("open /Game/Maps/L_IcebreakerWhitebox?solo%s"), *BuildConfigUrlOptions(false)));
        EnterGameInputAndClose();
    }
    else if (CurrentScreen == EFrostwakeFrontEndScreen::HostConfig)
    {
        // Host: open a listen lobby carrying the chosen mode + difficulty.
        PlayerController->ConsoleCommand(FString::Printf(TEXT("open /Game/Maps/L_IcebreakerWhitebox?listen%s"), *BuildConfigUrlOptions(true)));
        ShowLobbyScreen();
    }
}

void UFrostwakeMainMenuWidget::HandleJoinLobbyClicked()
{
    const FString Address = JoinAddressText ? JoinAddressText->GetText().ToString().TrimStartAndEnd() : FString();
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        PlayerController->ConsoleCommand(FString::Printf(TEXT("open %s"), Address.IsEmpty() ? TEXT("127.0.0.1") : *Address));
    }
}

void UFrostwakeMainMenuWidget::HandleHostStartClicked()
{
    if (AFrostwakePlayerController* PlayerController = Cast<AFrostwakePlayerController>(GetOwningPlayer()))
    {
        PlayerController->RequestHostStartMatch();
    }
}

bool UFrostwakeMainMenuWidget::IsLocalPlayerHost() const
{
    const APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController || !PlayerController->HasAuthority())
    {
        return false;
    }

    const AGameStateBase* CurrentGameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    return CurrentGameState && CurrentGameState->PlayerArray.Num() > 0 && CurrentGameState->PlayerArray[0] == PlayerController->PlayerState;
}

int32 UFrostwakeMainMenuWidget::GetConnectedPlayerCount() const
{
    const AGameStateBase* CurrentGameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    return CurrentGameState ? CurrentGameState->PlayerArray.Num() : 0;
}

void UFrostwakeMainMenuWidget::EnterGameInputAndClose()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        PlayerController->bShowMouseCursor = false;
        PlayerController->SetInputMode(FInputModeGameOnly());
    }

    RemoveFromParent();
}

void UFrostwakeMainMenuWidget::HandleModeCycleClicked()
{
    SelectedMode = (SelectedMode == EFrostwakeMatchMode::Standard) ? EFrostwakeMatchMode::Madman : EFrostwakeMatchMode::Standard;
    UpdateConfigButtonLabels();
}

void UFrostwakeMainMenuWidget::HandleDifficultyCycleClicked()
{
    switch (SelectedDifficulty)
    {
    case EFrostwakeDifficulty::Easy:
        SelectedDifficulty = EFrostwakeDifficulty::Normal;
        break;
    case EFrostwakeDifficulty::Normal:
        SelectedDifficulty = EFrostwakeDifficulty::Hard;
        break;
    default:
        SelectedDifficulty = EFrostwakeDifficulty::Easy;
        break;
    }
    UpdateConfigButtonLabels();
}

void UFrostwakeMainMenuWidget::UpdateConfigButtonLabels()
{
    if (ModeButtonLabel)
    {
        const TCHAR* ModeKey = (SelectedMode == EFrostwakeMatchMode::Madman) ? TEXT("Mode_Madman") : TEXT("Mode_Standard");
        FFormatNamedArguments Args;
        Args.Add(TEXT("Value"), FrostwakeUIText::Text(ModeKey));
        ModeButtonLabel->SetText(FText::Format(FrostwakeUIText::Text(TEXT("Menu_FmtMode")), Args));
    }
    if (DifficultyButtonLabel)
    {
        const TCHAR* DifficultyKey = TEXT("Difficulty_Normal");
        if (SelectedDifficulty == EFrostwakeDifficulty::Easy)
        {
            DifficultyKey = TEXT("Difficulty_Easy");
        }
        else if (SelectedDifficulty == EFrostwakeDifficulty::Hard)
        {
            DifficultyKey = TEXT("Difficulty_Hard");
        }
        FFormatNamedArguments Args;
        Args.Add(TEXT("Value"), FrostwakeUIText::Text(DifficultyKey));
        DifficultyButtonLabel->SetText(FText::Format(FrostwakeUIText::Text(TEXT("Menu_FmtDifficulty")), Args));
    }
}

FString UFrostwakeMainMenuWidget::BuildConfigUrlOptions(bool bIncludeMode) const
{
    const TCHAR* Difficulty = TEXT("normal");
    if (SelectedDifficulty == EFrostwakeDifficulty::Easy)
    {
        Difficulty = TEXT("easy");
    }
    else if (SelectedDifficulty == EFrostwakeDifficulty::Hard)
    {
        Difficulty = TEXT("hard");
    }
    FString Options = FString::Printf(TEXT("?difficulty=%s"), Difficulty);
    if (bIncludeMode)
    {
        Options += FString::Printf(TEXT("?mode=%s"), (SelectedMode == EFrostwakeMatchMode::Madman) ? TEXT("madman") : TEXT("standard"));
    }
    return Options;
}
