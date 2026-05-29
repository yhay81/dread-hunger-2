#include "AbyssHudWidget.h"

#include "AbyssInventoryComponent.h"
#include "AbyssLockCharacter.h"
#include "AbyssLockGameState.h"
#include "AbyssLockPlayerState.h"
#include "AbyssLockTypes.h"
#include "AbyssUIText.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
UTextBlock* MakeLine(UWidgetTree* WidgetTree, const FText& Text, float FontSize)
{
    UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TextBlock->SetText(Text);
    TextBlock->SetFont(AbyssUIText::UiFont(FontSize));
    return TextBlock;
}

// A labeled horizontal gauge: [fixed-width label] [fixed-size progress bar]. Original styling.
UHorizontalBox* MakeGauge(UWidgetTree* WidgetTree, const FText& LabelText, const FLinearColor& FillColor,
                          UTextBlock*& OutLabel, UProgressBar*& OutBar)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    OutLabel = MakeLine(WidgetTree, LabelText, 14.0f);
    USizeBox* LabelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelSize->SetWidthOverride(112.0f);
    LabelSize->AddChild(OutLabel);
    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelSize))
    {
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    }

    OutBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
    OutBar->SetPercent(1.0f);
    OutBar->SetFillColorAndOpacity(FillColor);
    USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    BarSize->SetWidthOverride(200.0f);
    BarSize->SetHeightOverride(14.0f);
    BarSize->AddChild(OutBar);
    if (UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(BarSize))
    {
        BarSlot->SetVerticalAlignment(VAlign_Center);
    }

    return Row;
}

void UpdateGauge(UProgressBar* Bar, UTextBlock* Label, const FText& Name, float Value, float MaxValue)
{
    if (!Bar || !Label)
    {
        return;
    }
    const float Percent = (MaxValue > 0.0f) ? FMath::Clamp(Value / MaxValue, 0.0f, 1.0f) : 0.0f;
    Bar->SetPercent(Percent);
    FFormatNamedArguments Args;
    Args.Add(TEXT("Label"), Name);
    Args.Add(TEXT("Percent"), FMath::RoundToInt(Percent * 100.0f));
    Label->SetText(FText::Format(AbyssUIText::Text(TEXT("Hud_FmtGauge")), Args));
}

// Map a server match-end reason code to a localized, human-readable result string key.
const TCHAR* ReasonKeyFor(const FString& Reason)
{
    if (Reason == TEXT("final_approach_complete"))
    {
        return TEXT("Hud_ReasonFinalApproach");
    }
    if (Reason == TEXT("timer_expired"))
    {
        return TEXT("Hud_ReasonTimerExpired");
    }
    if (Reason == TEXT("crew_incapacitated"))
    {
        return TEXT("Hud_ReasonCrewIncapacitated");
    }
    if (Reason == TEXT("fatal_ship_state"))
    {
        return TEXT("Hud_ReasonFatalShip");
    }
    if (Reason == TEXT("crew_threshold"))
    {
        return TEXT("Hud_ReasonCrewThreshold");
    }
    return TEXT("Hud_ResultEnded");
}
}

TSharedRef<SWidget> UAbyssHudWidget::RebuildWidget()
{
    // Code-only UUserWidget has no WidgetTree by default; create + build before Slate is made,
    // otherwise it renders nothing (same fix as AbyssMainMenuWidget).
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    BuildHud();
    return Super::RebuildWidget();
}

void UAbyssHudWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BuildHud();
    UE_LOG(LogTemp, Log, TEXT("frostwake_hud_construct children=%d"), RootBox ? RootBox->GetChildrenCount() : -1);
}

void UAbyssHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const APawn* Pawn = GetOwningPlayer() ? GetOwningPlayer()->GetPawn() : nullptr;
    const AAbyssLockCharacter* Character = Cast<AAbyssLockCharacter>(Pawn);

    // Hotbar: held items with the selected slot highlighted.
    if (HotbarText)
    {
        const UAbyssInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
        if (!Inventory)
        {
            HotbarText->SetText(AbyssUIText::Text(TEXT("Hud_ItemsNone")));
        }
        else
        {
            const TArray<FName> Items = Inventory->GetItems();
            if (Items.Num() == 0)
            {
                HotbarText->SetText(AbyssUIText::Text(TEXT("Hud_ItemsEmpty")));
            }
            else
            {
                const int32 Selected = Inventory->GetSelectedSlot();
                // Item names are gameplay IDs (not yet localized); only the "Items:" label is.
                FString Tokens;
                for (int32 Index = 0; Index < Items.Num(); ++Index)
                {
                    Tokens += (Index == Selected)
                        ? FString::Printf(TEXT("  [%s]"), *Items[Index].ToString())
                        : FString::Printf(TEXT("   %s "), *Items[Index].ToString());
                }
                FFormatNamedArguments Args;
                Args.Add(TEXT("Items"), FText::FromString(Tokens));
                HotbarText->SetText(FText::Format(AbyssUIText::Text(TEXT("Hud_FmtItems")), Args));
            }
        }
    }

    // Vitals gauges from the local character.
    if (Character)
    {
        UpdateGauge(HealthBar, HealthLabel, AbyssUIText::Text(TEXT("Hud_VitalHealth")), Character->GetHealth(), Character->GetMaxHealth());
        UpdateGauge(FoodBar, FoodLabel, AbyssUIText::Text(TEXT("Hud_VitalFood")), Character->GetSatiation(), Character->GetMaxSatiation());
        UpdateGauge(WarmthBar, WarmthLabel, AbyssUIText::Text(TEXT("Hud_VitalWarmth")), Character->GetWarmth(), Character->GetMaxWarmth());
    }

    // Route-to-goal bar from the game state (RouteProgress is 0..1; >=1 flips to Final Approach).
    if (RouteBar && RouteLabel)
    {
        const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
        const float RouteProgress = AbyssGameState ? AbyssGameState->GetRouteProgress() : 0.0f;
        RouteBar->SetPercent(FMath::Clamp(RouteProgress, 0.0f, 1.0f));
        const bool bFinalApproach =
            AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::FinalApproach;
        FFormatNamedArguments Args;
        Args.Add(TEXT("Percent"), FMath::RoundToInt(FMath::Clamp(RouteProgress, 0.0f, 1.0f) * 100.0f));
        Args.Add(TEXT("Suffix"), bFinalApproach ? AbyssUIText::Text(TEXT("Hud_RouteFinalApproachSuffix")) : FText::GetEmpty());
        RouteLabel->SetText(FText::Format(AbyssUIText::Text(TEXT("Hud_FmtRouteToGoal")), Args));
    }

    // Role line (owner-only secret team) — the Madman alone sees their true role; to others they read as Crew.
    const APlayerController* OwningPC = GetOwningPlayer();
    const AAbyssLockPlayerState* MyPlayerState = OwningPC ? OwningPC->GetPlayerState<AAbyssLockPlayerState>() : nullptr;
    const EAbyssTeam MyTeam = MyPlayerState ? MyPlayerState->GetSecretTeamForOwner() : EAbyssTeam::Unassigned;
    if (RoleText)
    {
        const TCHAR* RoleKey = TEXT("Hud_RoleCrew");
        if (MyTeam == EAbyssTeam::Saboteur)
        {
            RoleKey = TEXT("Hud_RoleSaboteur");
        }
        else if (MyTeam == EAbyssTeam::Madman)
        {
            RoleKey = TEXT("Hud_RoleMadman");
        }
        RoleText->SetText(AbyssUIText::Text(RoleKey));
    }

    // End-of-match result panel: Victory/Defeat for THIS player (Madman wins iff the Saboteurs win) + reason.
    if (ResultBox)
    {
        const AAbyssLockGameState* ResultGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
        if (ResultGameState && ResultGameState->GetMatchPhase() == EAbyssMatchPhase::MatchEnded)
        {
            const EAbyssTeam Winner = ResultGameState->GetWinningTeam();
            const bool bSaboteurAligned = (MyTeam == EAbyssTeam::Saboteur || MyTeam == EAbyssTeam::Madman);
            const bool bWon = (Winner == EAbyssTeam::Crew && MyTeam == EAbyssTeam::Crew)
                || (Winner == EAbyssTeam::Saboteur && bSaboteurAligned);
            if (ResultTitle)
            {
                ResultTitle->SetText(AbyssUIText::Text(bWon ? TEXT("Hud_ResultVictory") : TEXT("Hud_ResultDefeat")));
                ResultTitle->SetColorAndOpacity(bWon
                    ? FSlateColor(FLinearColor(0.40f, 0.85f, 0.46f))
                    : FSlateColor(FLinearColor(0.88f, 0.30f, 0.30f)));
            }
            if (ResultReason)
            {
                ResultReason->SetText(AbyssUIText::Text(ReasonKeyFor(ResultGameState->GetMatchEndReason())));
            }
            ResultBox->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            ResultBox->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UAbyssHudWidget::BuildHud()
{
    if (!WidgetTree || RootBox)
    {
        return;
    }

    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    WidgetTree->RootWidget = RootOverlay;

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UOverlaySlot* HudSlot = RootOverlay->AddChildToOverlay(RootBox))
    {
        HudSlot->SetHorizontalAlignment(HAlign_Left);
        HudSlot->SetVerticalAlignment(VAlign_Top);
        HudSlot->SetPadding(FMargin(32.0f, 28.0f, 0.0f, 0.0f));
    }

    // UI strings come from the shared Frostwake UI string table (English source); JA / zh-Hans are
    // translations against the same keys. Legible CJK glyphs still need the GP-09 font step.
    RootBox->AddChildToVerticalBox(MakeLine(WidgetTree, AbyssUIText::Text(TEXT("Hud_TitlePractice")), 22.0f));
    // Role line is updated each tick from the owner-only SecretTeam (Crew / Saboteur / Madman).
    RoleText = MakeLine(WidgetTree, AbyssUIText::Text(TEXT("Hud_RoleCrew")), 18.0f);
    RootBox->AddChildToVerticalBox(RoleText);
    RootBox->AddChildToVerticalBox(MakeLine(WidgetTree, AbyssUIText::Text(TEXT("Hud_Objective")), 18.0f));
    RootBox->AddChildToVerticalBox(MakeLine(WidgetTree, AbyssUIText::Text(TEXT("Hud_Controls")), 16.0f));

    HotbarText = MakeLine(WidgetTree, AbyssUIText::Text(TEXT("Hud_ItemsEmpty")), 18.0f);
    RootBox->AddChildToVerticalBox(HotbarText);

    // --- Vitals panel (bottom-left): health / food / warmth gauges. Original styling. ---
    UVerticalBox* VitalsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UOverlaySlot* VitalsSlot = RootOverlay->AddChildToOverlay(VitalsBox))
    {
        VitalsSlot->SetHorizontalAlignment(HAlign_Left);
        VitalsSlot->SetVerticalAlignment(VAlign_Bottom);
        VitalsSlot->SetPadding(FMargin(32.0f, 0.0f, 0.0f, 32.0f));
    }
    VitalsBox->AddChildToVerticalBox(MakeLine(WidgetTree, AbyssUIText::Text(TEXT("Hud_Vitals")), 16.0f));
    VitalsBox->AddChildToVerticalBox(
        MakeGauge(WidgetTree, AbyssUIText::Text(TEXT("Hud_VitalHealth")), FLinearColor(0.86f, 0.24f, 0.24f), HealthLabel, HealthBar));
    VitalsBox->AddChildToVerticalBox(
        MakeGauge(WidgetTree, AbyssUIText::Text(TEXT("Hud_VitalFood")), FLinearColor(0.92f, 0.64f, 0.20f), FoodLabel, FoodBar));
    VitalsBox->AddChildToVerticalBox(
        MakeGauge(WidgetTree, AbyssUIText::Text(TEXT("Hud_VitalWarmth")), FLinearColor(0.36f, 0.74f, 0.92f), WarmthLabel, WarmthBar));

    // --- Route-to-goal bar (top-center): how far the ship is toward its destination. ---
    UVerticalBox* RouteBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UOverlaySlot* RouteSlot = RootOverlay->AddChildToOverlay(RouteBox))
    {
        RouteSlot->SetHorizontalAlignment(HAlign_Center);
        RouteSlot->SetVerticalAlignment(VAlign_Top);
        RouteSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
    }
    FFormatNamedArguments RouteArgs;
    RouteArgs.Add(TEXT("Percent"), 0);
    RouteArgs.Add(TEXT("Suffix"), FText::GetEmpty());
    RouteLabel = MakeLine(WidgetTree, FText::Format(AbyssUIText::Text(TEXT("Hud_FmtRouteToGoal")), RouteArgs), 16.0f);
    if (UVerticalBoxSlot* RouteLabelSlot = RouteBox->AddChildToVerticalBox(RouteLabel))
    {
        RouteLabelSlot->SetHorizontalAlignment(HAlign_Center);
    }
    RouteBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
    RouteBar->SetPercent(0.0f);
    RouteBar->SetFillColorAndOpacity(FLinearColor(0.40f, 0.80f, 0.46f));
    USizeBox* RouteSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    RouteSize->SetWidthOverride(360.0f);
    RouteSize->SetHeightOverride(16.0f);
    RouteSize->AddChild(RouteBar);
    if (UVerticalBoxSlot* RouteBarSlot = RouteBox->AddChildToVerticalBox(RouteSize))
    {
        RouteBarSlot->SetHorizontalAlignment(HAlign_Center);
        RouteBarSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
    }

    // --- Match result panel (center): hidden until the match ends, then shows Victory/Defeat + reason. ---
    ResultBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UOverlaySlot* ResultSlot = RootOverlay->AddChildToOverlay(ResultBox))
    {
        ResultSlot->SetHorizontalAlignment(HAlign_Center);
        ResultSlot->SetVerticalAlignment(VAlign_Center);
    }
    ResultTitle = MakeLine(WidgetTree, AbyssUIText::Text(TEXT("Hud_ResultEnded")), 44.0f);
    if (UVerticalBoxSlot* ResultTitleSlot = ResultBox->AddChildToVerticalBox(ResultTitle))
    {
        ResultTitleSlot->SetHorizontalAlignment(HAlign_Center);
    }
    ResultReason = MakeLine(WidgetTree, FText::GetEmpty(), 20.0f);
    if (UVerticalBoxSlot* ResultReasonSlot = ResultBox->AddChildToVerticalBox(ResultReason))
    {
        ResultReasonSlot->SetHorizontalAlignment(HAlign_Center);
        ResultReasonSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
    }
    ResultBox->SetVisibility(ESlateVisibility::Collapsed);
}
