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

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    void TryPrimaryInteract();

    UFUNCTION(BlueprintCallable, Category = "Abyss|Lobby")
    void SetReadyForMatch(bool bReady);

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetReadyForMatch(bool bReady);
};
