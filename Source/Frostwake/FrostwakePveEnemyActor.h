#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FrostwakePveEnemyActor.generated.h"

class AFrostwakeCharacter;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class FROSTWAKE_API AFrostwakePveEnemyActor : public AActor
{
    GENERATED_BODY()

public:
    AFrostwakePveEnemyActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|PvE")
    void ConfigureEnemy(float InDamageAmount);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|PvE")
    bool AttackCharacter(AFrostwakeCharacter* TargetCharacter);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|PvE")
    float GetDamageAmount() const { return DamageAmount; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|PvE")
    int32 GetAttackCount() const { return AttackCount; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|PvE")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|PvE")
    TObjectPtr<UStaticMeshComponent> MarkerMesh;

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|PvE", meta = (ClampMin = "0.0"))
    float DamageAmount;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|PvE")
    int32 AttackCount;
};
