#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
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

    virtual void BeginPlay() override;
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

    UFUNCTION(BlueprintCallable, Category = "Abyss|Life")
    float GetMaxHealth() const { return MaxHealth; }

    // Survival gauges (original near-future expedition framing): food + warmth deplete over time and
    // drain health when empty. Functional parity with the genre's survival meters; styling is original.
    UFUNCTION(BlueprintCallable, Category = "Abyss|Survival")
    float GetSatiation() const { return Satiation; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Survival")
    float GetMaxSatiation() const { return MaxSatiation; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Survival")
    float GetWarmth() const { return Warmth; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Survival")
    float GetMaxWarmth() const { return MaxWarmth; }

    // Server-authoritative: consume the item in SlotIndex if it is food, restoring Satiation.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Survival")
    bool EatRation(int32 SlotIndex);

    // Server-authoritative: add Warmth (clamped). Called by a heat-source interactable.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Survival")
    bool ApplyWarmth(float WarmthAmount, AActor* HeatSource);

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    float MaxSatiation;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Survival")
    float Satiation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    float MaxWarmth;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Survival")
    float Warmth;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    float SatiationDecayPerSecond;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    float WarmthDecayPerSecond;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    float StarvationDamagePerSecond;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    float HypothermiaDamagePerSecond;

    // Satiation restored by eating one ration, and the set of item ids that count as food.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    float RationSatiationRestore;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss|Survival")
    TSet<FName> FoodItemIds;

    FTimerHandle SurvivalTimerHandle;

    // Server-side periodic survival update: food/warmth decay, then health drains when either is empty.
    void UpdateSurvival();

    UFUNCTION(Server, Reliable)
    void ServerTryDropItem();

    // Use (e.g. eat) the locally-selected item. SelectedSlot is client-local, so the index is sent
    // to the server explicitly.
    void UseSelectedItem();

    UFUNCTION(Server, Reliable)
    void ServerUseSelectedItem(int32 SlotIndex);

    AAbyssItemPickupActor* DropFirstInventoryItem();

    void SelectNextItem();
    void SelectPrevItem();

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
};
