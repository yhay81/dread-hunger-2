#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbyssHudWidget.generated.h"

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

    void BuildHud();
};
