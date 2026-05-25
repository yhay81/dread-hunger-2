#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssPveEnemyActor.generated.h"

class AAbyssLockCharacter;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ABYSSLOCK_API AAbyssPveEnemyActor : public AActor
{
    GENERATED_BODY()

public:
    AAbyssPveEnemyActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|PvE")
    void ConfigureEnemy(float InDamageAmount);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|PvE")
    bool AttackCharacter(AAbyssLockCharacter* TargetCharacter);

    UFUNCTION(BlueprintCallable, Category = "Abyss|PvE")
    float GetDamageAmount() const { return DamageAmount; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|PvE")
    int32 GetAttackCount() const { return AttackCount; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|PvE")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|PvE")
    TObjectPtr<UStaticMeshComponent> MarkerMesh;

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Abyss|PvE", meta = (ClampMin = "0.0"))
    float DamageAmount;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|PvE")
    int32 AttackCount;
};
