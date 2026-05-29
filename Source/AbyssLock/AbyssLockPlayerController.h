#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AbyssLockPlayerController.generated.h"

UCLASS()
class ABYSSLOCK_API AAbyssLockPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AAbyssLockPlayerController();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    void TryPrimaryInteract();

    UFUNCTION(BlueprintCallable, Category = "Abyss|Lobby")
    void SetReadyForMatch(bool bReady);

    UFUNCTION(BlueprintCallable, Category = "Abyss|Lobby")
    void RequestHostStartMatch();

    UFUNCTION(BlueprintCallable, Category = "Abyss|SinglePlayer")
    void RequestSoloMatch();

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetReadyForMatch(bool bReady);

    UFUNCTION(Server, Reliable)
    void ServerRequestHostStartMatch();

    UFUNCTION(Server, Reliable)
    void ServerRequestSoloMatch();

private:
    UPROPERTY()
    class UAbyssMainMenuWidget* MainMenuWidget = nullptr;

    UPROPERTY()
    class UAbyssHudWidget* HudWidget = nullptr;
};
