#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FrostwakePlayerController.generated.h"

UCLASS()
class FROSTWAKE_API AFrostwakePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AFrostwakePlayerController();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    void TryPrimaryInteract();

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Lobby")
    void SetReadyForMatch(bool bReady);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Lobby")
    void RequestHostStartMatch();

    UFUNCTION(BlueprintCallable, Category = "Frostwake|SinglePlayer")
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
    class UFrostwakeMainMenuWidget* MainMenuWidget = nullptr;

    UPROPERTY()
    class UFrostwakeHudWidget* HudWidget = nullptr;
};
