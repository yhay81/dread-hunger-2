#pragma once

#include "CoreMinimal.h"
#include "FrostwakeInteractableActor.h"
#include "FrostwakeLifeActionActor.generated.h"

class AFrostwakeCharacter;

UENUM(BlueprintType)
enum class EFrostwakeLifeAction : uint8
{
    RescueDowned UMETA(DisplayName = "Rescue Downed"),
    ContainAlive UMETA(DisplayName = "Contain Alive"),
    ReleaseContained UMETA(DisplayName = "Release Contained")
};

UCLASS(Blueprintable)
class FROSTWAKE_API AFrostwakeLifeActionActor : public AFrostwakeInteractableActor
{
    GENERATED_BODY()

public:
    AFrostwakeLifeActionActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    void ConfigureLifeAction(AFrostwakeCharacter* InTargetCharacter, EFrostwakeLifeAction InAction);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Life")
    TObjectPtr<AFrostwakeCharacter> TargetCharacter;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Life")
    EFrostwakeLifeAction Action;
};
