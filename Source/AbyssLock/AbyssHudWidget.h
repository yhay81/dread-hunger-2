#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbyssHudWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UVerticalBox;

// Minimal in-match HUD so the solo/practice round reads as a game (role + objective + interact
// hint) instead of a silent greybox. Code-only UUserWidget; localization + JP font are GP-09.
UCLASS()
class ABYSSLOCK_API UAbyssHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UPROPERTY()
    UVerticalBox* RootBox = nullptr;

    UPROPERTY()
    UTextBlock* HotbarText = nullptr;

    // Survival gauges (health / food / warmth) + route-to-goal bar. Original styling.
    UPROPERTY()
    UProgressBar* HealthBar = nullptr;
    UPROPERTY()
    UTextBlock* HealthLabel = nullptr;

    UPROPERTY()
    UProgressBar* FoodBar = nullptr;
    UPROPERTY()
    UTextBlock* FoodLabel = nullptr;

    UPROPERTY()
    UProgressBar* WarmthBar = nullptr;
    UPROPERTY()
    UTextBlock* WarmthLabel = nullptr;

    UPROPERTY()
    UProgressBar* RouteBar = nullptr;
    UPROPERTY()
    UTextBlock* RouteLabel = nullptr;

    void BuildHud();
};
