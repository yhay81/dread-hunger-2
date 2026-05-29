#include "AbyssMainMenuWidget.h"

#include "AbyssLockGameState.h"
#include "AbyssUIText.h"
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
    TextBlock->SetFont(AbyssUIText::UiFont(FontSize));
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

TSharedRef<SWidget> UAbyssMainMenuWidget::RebuildWidget()
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

void UAbyssMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);
    BuildWidgetTree();
    ShowStartScreen();

    UE_LOG(LogTemp, Log, TEXT("frontend_menu_construct rootbox_children=%d"), RootBox ? RootBox->GetChildrenCount() : -1);
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

    GameStartButton = MakeButton(WidgetTree, AbyssUIText::Text(TEXT("Menu_Play")));
    SoloModeButton = MakeButton(WidgetTree, AbyssUIText::Text(TEXT("Menu_SoloPractice")));
    HostLobbyButton = MakeButton(WidgetTree, AbyssUIText::Text(TEXT("Menu_HostLobby")));
    JoinLobbyButton = MakeButton(WidgetTree, AbyssUIText::Text(TEXT("Menu_JoinLobby")));
    HostStartButton = MakeButton(WidgetTree, AbyssUIText::Text(TEXT("Menu_StartMatch")));
    ModeButton = MakeCycleButton(WidgetTree, ModeButtonLabel);
    DifficultyButton = MakeCycleButton(WidgetTree, DifficultyButtonLabel);
    ConfigConfirmButton = MakeCycleButton(WidgetTree, ConfigConfirmLabel);

    JoinAddressText = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    JoinAddressText->SetText(FText::FromString(TEXT("127.0.0.1")));
    JoinAddressText->SetHintText(AbyssUIText::Text(TEXT("Menu_AddressHint")));

    GameStartButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleGameStartClicked);
    SoloModeButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleSoloModeClicked);
    HostLobbyButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleHostLobbyClicked);
    JoinLobbyButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleJoinLobbyClicked);
    HostStartButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleHostStartClicked);
    ModeButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleModeCycleClicked);
    DifficultyButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleDifficultyCycleClicked);
    ConfigConfirmButton->OnClicked.AddDynamic(this, &UAbyssMainMenuWidget::HandleConfigConfirmClicked);

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
    ModeButton->SetVisibility(ESlateVisibility::Collapsed);
    DifficultyButton->SetVisibility(ESlateVisibility::Collapsed);
    ConfigConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UAbyssMainMenuWidget::ShowLobbyChoiceScreen()
{
    CurrentScreen = EAbyssFrontEndScreen::LobbyChoice;
    StatusText->SetText(AbyssUIText::Text(TEXT("Menu_LobbyChoicePrompt")));
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

void UAbyssMainMenuWidget::ShowSoloConfigScreen()
{
    CurrentScreen = EAbyssFrontEndScreen::SoloConfig;
    StatusText->SetText(AbyssUIText::Text(TEXT("Menu_ConfigPrompt")));
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
    ConfigConfirmLabel->SetText(AbyssUIText::Text(TEXT("Menu_StartSolo")));
    UpdateConfigButtonLabels();
}

void UAbyssMainMenuWidget::ShowHostConfigScreen()
{
    CurrentScreen = EAbyssFrontEndScreen::HostConfig;
    StatusText->SetText(AbyssUIText::Text(TEXT("Menu_ConfigPrompt")));
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
    ConfigConfirmLabel->SetText(AbyssUIText::Text(TEXT("Menu_HostLobby")));
    UpdateConfigButtonLabels();
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
    ModeButton->SetVisibility(ESlateVisibility::Collapsed);
    DifficultyButton->SetVisibility(ESlateVisibility::Collapsed);
    ConfigConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
    RefreshLobbyState();
}

void UAbyssMainMenuWidget::RefreshLobbyState()
{
    const int32 PlayerCount = GetConnectedPlayerCount();
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Current"), PlayerCount);
        Args.Add(TEXT("Required"), AbyssLobbyRequiredPlayers);
        PlayerCountText->SetText(FText::Format(AbyssUIText::Text(TEXT("Menu_FmtPlayerCount")), Args));
    }

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
        FFormatNamedArguments Args;
        Args.Add(TEXT("Required"), AbyssLobbyRequiredPlayers);
        StatusText->SetText(FText::Format(AbyssUIText::Text(TEXT("Menu_FmtLobbyReady")), Args));
    }
    else if (bHost)
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Required"), AbyssLobbyRequiredPlayers);
        StatusText->SetText(FText::Format(AbyssUIText::Text(TEXT("Menu_FmtLobbyWaiting")), Args));
    }
    else
    {
        StatusText->SetText(AbyssUIText::Text(TEXT("Menu_WaitingForHost")));
    }
}

void UAbyssMainMenuWidget::HandleGameStartClicked()
{
    ShowLobbyChoiceScreen();
}

void UAbyssMainMenuWidget::HandleHostLobbyClicked()
{
    // Choosing "Host" opens the host config screen (mode + difficulty); travel happens on confirm.
    ShowHostConfigScreen();
}

void UAbyssMainMenuWidget::HandleSoloModeClicked()
{
    // Choosing "Solo" opens the solo config screen (difficulty only); travel happens on confirm.
    ShowSoloConfigScreen();
}

void UAbyssMainMenuWidget::HandleConfigConfirmClicked()
{
    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        return;
    }

    if (CurrentScreen == EAbyssFrontEndScreen::SoloConfig)
    {
        // Solo standalone practice: InitGame reads "solo" + the chosen difficulty (Standard forced).
        PlayerController->ConsoleCommand(FString::Printf(TEXT("open /Game/Maps/L_IcebreakerWhitebox?solo%s"), *BuildConfigUrlOptions(false)));
        EnterGameInputAndClose();
    }
    else if (CurrentScreen == EAbyssFrontEndScreen::HostConfig)
    {
        // Host: open a listen lobby carrying the chosen mode + difficulty.
        PlayerController->ConsoleCommand(FString::Printf(TEXT("open /Game/Maps/L_IcebreakerWhitebox?listen%s"), *BuildConfigUrlOptions(true)));
        ShowLobbyScreen();
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

void UAbyssMainMenuWidget::HandleModeCycleClicked()
{
    SelectedMode = (SelectedMode == EAbyssMatchMode::Standard) ? EAbyssMatchMode::Madman : EAbyssMatchMode::Standard;
    UpdateConfigButtonLabels();
}

void UAbyssMainMenuWidget::HandleDifficultyCycleClicked()
{
    switch (SelectedDifficulty)
    {
    case EAbyssDifficulty::Easy:
        SelectedDifficulty = EAbyssDifficulty::Normal;
        break;
    case EAbyssDifficulty::Normal:
        SelectedDifficulty = EAbyssDifficulty::Hard;
        break;
    default:
        SelectedDifficulty = EAbyssDifficulty::Easy;
        break;
    }
    UpdateConfigButtonLabels();
}

void UAbyssMainMenuWidget::UpdateConfigButtonLabels()
{
    if (ModeButtonLabel)
    {
        const TCHAR* ModeKey = (SelectedMode == EAbyssMatchMode::Madman) ? TEXT("Mode_Madman") : TEXT("Mode_Standard");
        FFormatNamedArguments Args;
        Args.Add(TEXT("Value"), AbyssUIText::Text(ModeKey));
        ModeButtonLabel->SetText(FText::Format(AbyssUIText::Text(TEXT("Menu_FmtMode")), Args));
    }
    if (DifficultyButtonLabel)
    {
        const TCHAR* DifficultyKey = TEXT("Difficulty_Normal");
        if (SelectedDifficulty == EAbyssDifficulty::Easy)
        {
            DifficultyKey = TEXT("Difficulty_Easy");
        }
        else if (SelectedDifficulty == EAbyssDifficulty::Hard)
        {
            DifficultyKey = TEXT("Difficulty_Hard");
        }
        FFormatNamedArguments Args;
        Args.Add(TEXT("Value"), AbyssUIText::Text(DifficultyKey));
        DifficultyButtonLabel->SetText(FText::Format(AbyssUIText::Text(TEXT("Menu_FmtDifficulty")), Args));
    }
}

FString UAbyssMainMenuWidget::BuildConfigUrlOptions(bool bIncludeMode) const
{
    const TCHAR* Difficulty = TEXT("normal");
    if (SelectedDifficulty == EAbyssDifficulty::Easy)
    {
        Difficulty = TEXT("easy");
    }
    else if (SelectedDifficulty == EAbyssDifficulty::Hard)
    {
        Difficulty = TEXT("hard");
    }
    FString Options = FString::Printf(TEXT("?difficulty=%s"), Difficulty);
    if (bIncludeMode)
    {
        Options += FString::Printf(TEXT("?mode=%s"), (SelectedMode == EAbyssMatchMode::Madman) ? TEXT("madman") : TEXT("standard"));
    }
    return Options;
}
