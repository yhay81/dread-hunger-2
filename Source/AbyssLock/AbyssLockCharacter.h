#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbyssLockCharacter.generated.h"

class UAbyssInteractionComponent;
class UAbyssInventoryComponent;
class UCameraComponent;
class AAbyssItemPickupActor;

UCLASS()
class ABYSSLOCK_API AAbyssLockCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAbyssLockCharacter();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    UAbyssInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Inventory")
    UAbyssInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Interaction")
    void TryPrimaryInteract();

    UFUNCTION(BlueprintCallable, Category = "Abyss|Inventory")
    void TryDropItem();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Life")
    bool ApplyServerDamage(float DamageAmount, AActor* DamageSource);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Life")
    bool RescueFromDowned(APawn* RescuerPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Life")
    bool ContainPlayer(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Life")
    bool ReleaseFromContainment(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, Category = "Abyss|Life")
    float GetHealth() const { return Health; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Interaction")
    TObjectPtr<UAbyssInteractionComponent> InteractionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Inventory")
    TObjectPtr<UAbyssInventoryComponent> InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Camera")
    TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Life")
    float MaxHealth;

    UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Abyss|Life")
    float Health;

    UFUNCTION()
    void OnRep_Health();

    UFUNCTION(Server, Reliable)
    void ServerTryDropItem();

    AAbyssItemPickupActor* DropFirstInventoryItem();

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
};
