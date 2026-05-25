#pragma once

#include "CoreMinimal.h"
#include "AbyssInteractableActor.h"
#include "AbyssLifeActionActor.generated.h"

class AAbyssLockCharacter;

UENUM(BlueprintType)
enum class EAbyssLifeAction : uint8
{
    RescueDowned UMETA(DisplayName = "Rescue Downed"),
    ContainAlive UMETA(DisplayName = "Contain Alive"),
    ReleaseContained UMETA(DisplayName = "Release Contained")
};

UCLASS(Blueprintable)
class ABYSSLOCK_API AAbyssLifeActionActor : public AAbyssInteractableActor
{
    GENERATED_BODY()

public:
    AAbyssLifeActionActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Life")
    void ConfigureLifeAction(AAbyssLockCharacter* InTargetCharacter, EAbyssLifeAction InAction);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Life")
    TObjectPtr<AAbyssLockCharacter> TargetCharacter;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Life")
    EAbyssLifeAction Action;
};
