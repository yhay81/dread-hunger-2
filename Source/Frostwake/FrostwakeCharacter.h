#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Character.h"
#include "FrostwakeCharacter.generated.h"

class UFrostwakeInteractionComponent;
class UFrostwakeInventoryComponent;
class UFrostwakeAttributeComponent;
class UCameraComponent;
class AFrostwakeItemPickupActor;

UCLASS()
class FROSTWAKE_API AFrostwakeCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFrostwakeCharacter();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    UFrostwakeInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    UFrostwakeInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Interaction")
    void TryPrimaryInteract();

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Inventory")
    void TryDropItem();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool ApplyServerDamage(float DamageAmount, AActor* DamageSource);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool RescueFromDowned(APawn* RescuerPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool ContainPlayer(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool ReleaseFromContainment(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Life")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Life")
    float GetMaxHealth() const { return MaxHealth; }

    // Survival gauges (original near-future expedition framing): food + warmth deplete over time and
    // drain health when empty. Functional parity with the genre's survival meters; styling is original.
    // "Satiation" = food remaining = MaxHunger - Hunger. The DH-semantic Hunger attribute (§3.15) RISES
    // as you get hungrier; these accessors present the satiation view so the HUD food bar still reads
    // high=fed without any UX change.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Survival")
    float GetSatiation() const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Survival")
    float GetMaxSatiation() const;

    // Warmth + Hunger now live in the Action System attribute component (their designated home,
    // plan §3.2/§3.15); these accessors read it so the HUD / heat sources keep working unchanged.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Survival")
    float GetWarmth() const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Survival")
    float GetMaxWarmth() const;

    // Server-authoritative: consume the item in SlotIndex if it is food, reducing Hunger.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Survival")
    bool EatRation(int32 SlotIndex);

    // Server-authoritative: add Warmth (clamped). Called by a heat-source interactable.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Survival")
    bool ApplyWarmth(float WarmthAmount, AActor* HeatSource);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Interaction")
    TObjectPtr<UFrostwakeInteractionComponent> InteractionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Inventory")
    TObjectPtr<UFrostwakeInventoryComponent> InventoryComponent;

    // Action System survival attributes (plan §3.2 Tier 2). Canonical home for Warmth (temperature-driven)
    // and Hunger (DH-semantic, rises while unfed); Health migrates here with the damage/death slice (§3.17).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Attributes")
    TObjectPtr<UFrostwakeAttributeComponent> AttributeComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Camera")
    TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Life")
    float MaxHealth;

    UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Frostwake|Life")
    float Health;

    UFUNCTION()
    void OnRep_Health();

    // Hunger (DH-semantic, §3.15) rises by this much per second; difficulty scales it. Starvation
    // damage applies once Hunger reaches its max.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Survival")
    float HungerIncreasePerSecond;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Survival")
    float StarvationDamagePerSecond;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Survival")
    float HypothermiaDamagePerSecond;

    // Hunger removed by eating one ration, and the set of item ids that count as food.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Survival")
    float RationHungerRestore;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Survival")
    TSet<FName> FoodItemIds;

    FTimerHandle SurvivalTimerHandle;

    // Server-side periodic survival update: food/warmth decay, then health drains when either is empty.
    void UpdateSurvival();

    // Difficulty multiplier applied to food/warmth decay (1.0 if no match config / not authoritative).
    float GetMatchDecayMultiplier() const;

    UFUNCTION(Server, Reliable)
    void ServerTryDropItem();

    // Use (e.g. eat) the locally-selected item. SelectedSlot is client-local, so the index is sent
    // to the server explicitly.
    void UseSelectedItem();

    UFUNCTION(Server, Reliable)
    void ServerUseSelectedItem(int32 SlotIndex);

    AFrostwakeItemPickupActor* DropFirstInventoryItem();

    void SelectNextItem();
    void SelectPrevItem();

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
};
