#include "AbyssHudWidget.h"

#include "AbyssInventoryComponent.h"
#include "AbyssLockCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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

    if (!HotbarText)
    {
        return;
    }

    const APawn* Pawn = GetOwningPlayer() ? GetOwningPlayer()->GetPawn() : nullptr;
    const AAbyssLockCharacter* Character = Cast<AAbyssLockCharacter>(Pawn);
    const UAbyssInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
    if (!Inventory)
    {
        HotbarText->SetText(FText::FromString(TEXT("Items: (none)")));
        return;
    }

    const TArray<FName> Items = Inventory->GetItems();
    if (Items.Num() == 0)
    {
        HotbarText->SetText(FText::FromString(TEXT("Items: (empty)")));
        return;
    }

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
}
