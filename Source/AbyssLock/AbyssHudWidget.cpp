#include "AbyssHudWidget.h"

#include "AbyssInventoryComponent.h"
#include "AbyssLockCharacter.h"
#include "AbyssLockGameState.h"
#include "AbyssLockTypes.h"
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
    FSlateFontInfo Font = TextBlock->GetFont();
    Font.Size = FontSize;
    TextBlock->SetFont(Font);
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

void UpdateGauge(UProgressBar* Bar, UTextBlock* Label, const TCHAR* Name, float Value, float MaxValue)
{
    if (!Bar || !Label)
    {
        return;
    }
    const float Percent = (MaxValue > 0.0f) ? FMath::Clamp(Value / MaxValue, 0.0f, 1.0f) : 0.0f;
    Bar->SetPercent(Percent);
    Label->SetText(FText::FromString(
        FString::Printf(TEXT("%s  %d%%"), Name, FMath::RoundToInt(Percent * 100.0f))));
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
            HotbarText->SetText(FText::FromString(TEXT("Items: (none)")));
        }
        else
        {
            const TArray<FName> Items = Inventory->GetItems();
            if (Items.Num() == 0)
            {
                HotbarText->SetText(FText::FromString(TEXT("Items: (empty)")));
            }
            else
            {
                const int32 Selected = Inventory->GetSelectedSlot();
                FString Line = TEXT("Items:");
                for (int32 Index = 0; Index < Items.Num(); ++Index)
                {
                    Line += (Index == Selected)
                        ? FString::Printf(TEXT("  [%s]"), *Items[Index].ToString())
                        : FString::Printf(TEXT("   %s "), *Items[Index].ToString());
                }
                HotbarText->SetText(FText::FromString(Line));
            }
        }
    }

    // Vitals gauges from the local character.
    if (Character)
    {
        UpdateGauge(HealthBar, HealthLabel, TEXT("Health"), Character->GetHealth(), Character->GetMaxHealth());
        UpdateGauge(FoodBar, FoodLabel, TEXT("Food"), Character->GetSatiation(), Character->GetMaxSatiation());
        UpdateGauge(WarmthBar, WarmthLabel, TEXT("Warmth"), Character->GetWarmth(), Character->GetMaxWarmth());
    }

    // Route-to-goal bar from the game state (RouteProgress is 0..1; >=1 flips to Final Approach).
    if (RouteBar && RouteLabel)
    {
        const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
        const float RouteProgress = AbyssGameState ? AbyssGameState->GetRouteProgress() : 0.0f;
        RouteBar->SetPercent(FMath::Clamp(RouteProgress, 0.0f, 1.0f));
        const bool bFinalApproach =
            AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::FinalApproach;
        RouteLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Route to Goal  %d%%%s"),
            FMath::RoundToInt(FMath::Clamp(RouteProgress, 0.0f, 1.0f) * 100.0f),
            bFinalApproach ? TEXT("  - Final Approach!") : TEXT(""))));
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

    // English placeholder copy for the debug slice; JP localization + font is GP-09.
    RootBox->AddChildToVerticalBox(MakeLine(WidgetTree, FText::FromString(TEXT("Frostwake - Practice")), 22.0f));
    RootBox->AddChildToVerticalBox(MakeLine(WidgetTree, FText::FromString(TEXT("Role: Crew")), 18.0f));
    RootBox->AddChildToVerticalBox(MakeLine(WidgetTree, FText::FromString(TEXT("Objective: advance the route, keep ship systems online")), 18.0f));
    RootBox->AddChildToVerticalBox(MakeLine(WidgetTree, FText::FromString(TEXT("[E] Interact   [Scroll] Select item   [Q] Drop")), 16.0f));

    HotbarText = MakeLine(WidgetTree, FText::FromString(TEXT("Items: (empty)")), 18.0f);
    RootBox->AddChildToVerticalBox(HotbarText);

    // --- Vitals panel (bottom-left): health / food / warmth gauges. Original styling. ---
    UVerticalBox* VitalsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UOverlaySlot* VitalsSlot = RootOverlay->AddChildToOverlay(VitalsBox))
    {
        VitalsSlot->SetHorizontalAlignment(HAlign_Left);
        VitalsSlot->SetVerticalAlignment(VAlign_Bottom);
        VitalsSlot->SetPadding(FMargin(32.0f, 0.0f, 0.0f, 32.0f));
    }
    VitalsBox->AddChildToVerticalBox(MakeLine(WidgetTree, FText::FromString(TEXT("Vitals")), 16.0f));
    VitalsBox->AddChildToVerticalBox(
        MakeGauge(WidgetTree, FText::FromString(TEXT("Health")), FLinearColor(0.86f, 0.24f, 0.24f), HealthLabel, HealthBar));
    VitalsBox->AddChildToVerticalBox(
        MakeGauge(WidgetTree, FText::FromString(TEXT("Food")), FLinearColor(0.92f, 0.64f, 0.20f), FoodLabel, FoodBar));
    VitalsBox->AddChildToVerticalBox(
        MakeGauge(WidgetTree, FText::FromString(TEXT("Warmth")), FLinearColor(0.36f, 0.74f, 0.92f), WarmthLabel, WarmthBar));

    // --- Route-to-goal bar (top-center): how far the ship is toward its destination. ---
    UVerticalBox* RouteBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UOverlaySlot* RouteSlot = RootOverlay->AddChildToOverlay(RouteBox))
    {
        RouteSlot->SetHorizontalAlignment(HAlign_Center);
        RouteSlot->SetVerticalAlignment(VAlign_Top);
        RouteSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
    }
    RouteLabel = MakeLine(WidgetTree, FText::FromString(TEXT("Route to Goal  0%")), 16.0f);
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
}
