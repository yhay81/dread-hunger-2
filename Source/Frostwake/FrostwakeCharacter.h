#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "FrostwakeCharacter.generated.h"

class UFrostwakeInteractionComponent;
class UFrostwakeInventoryComponent;
class UFrostwakeAttributeComponent;
class UFrostwakeActionComponent;
class UFrostwakeActionEffect;
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

    // Server-authoritative typed damage (§3.17): DamageType is a Damage.* tag; the amount is run through
    // AdjustDamage (data-driven DamageTypeDefinition modifiers) before it hits Health.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool ApplyServerDamage(float DamageAmount, FGameplayTag DamageType, AActor* DamageSource);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool RescueFromDowned(APawn* RescuerPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool ContainPlayer(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Life")
    bool ReleaseFromContainment(APawn* InstigatorPawn);

    // Health lives in the Action System attribute component (its designated home, plan §3.15); these
    // accessors read it so the HUD / damage callers keep working unchanged.
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Life")
    float GetHealth() const;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Life")
    float GetMaxHealth() const;

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

    // Action System survival attributes (plan §3.2 Tier 2). Canonical home for ALL vitals now —
    // Health, Warmth (temperature-driven), Hunger (DH-semantic, rises while unfed), ReservedHealth
    // (§3.17: DT_Poison drains it; revive draws Health back out of the reserve, so a poisoned reserve
    // restores less), and Stamina (consumed by abilities, e.g. UFrostwakeFogAbility). All five are now
    // driven; the character keeps no hand-written vitals floats.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Attributes")
    TObjectPtr<UFrostwakeAttributeComponent> AttributeComponent;

    // Action System host (plan §3.2 Tier 2): buffs/debuffs/abilities run through this, not raw methods.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Action")
    TObjectPtr<UFrostwakeActionComponent> ActionComponent;

    // The live cold-exposure effect while Warmth is bottomed out (server-only handle; nullptr otherwise).
    UPROPERTY(Transient)
    TObjectPtr<UFrostwakeActionEffect> ActiveColdEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Camera")
    TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

    // Hunger (DH-semantic, §3.15) rises by this much per second; difficulty scales it. Starvation
    // damage applies once Hunger reaches its max.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Survival")
    float HungerIncreasePerSecond;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Frostwake|Survival")
    float StarvationDamagePerSecond;
    // (Cold/hypothermia drain now lives in UFrostwakeColdExposureEffect, applied while Warmth is at 0.)

    FTimerHandle SurvivalTimerHandle;

    // Server-side periodic survival update: food/warmth decay, then health drains when either is empty.
    void UpdateSurvival();

    // Difficulty multiplier applied to food/warmth decay (1.0 if no match config / not authoritative).
    float GetMatchDecayMultiplier() const;

    // §3.17 AdjustDamage: apply the DamageType's data-driven DamageMultiplier to a base amount, and report
    // (out) whether the type drains ReservedHealth (DT_Poison). Per-perk resistances arrive with perks.
    float AdjustDamage(float BaseDamage, const FGameplayTag& DamageType, bool& bOutAffectsReservedHealth) const;

    UFUNCTION(Server, Reliable)
    void ServerTryDropItem();

    // Use (e.g. eat) the locally-selected item. SelectedSlot is client-local, so the index is sent
    // to the server explicitly.
    void UseSelectedItem();

    UFUNCTION(Server, Reliable)
    void ServerUseSelectedItem(int32 SlotIndex);

    // Forward the locally-cycled hotbar selection to the server so it can publish the held item (the
    // visible-to-all HeldItemId) — SelectedSlot is client-local, so the server must be told (review #2b).
    UFUNCTION(Server, Reliable)
    void ServerSetSelectedSlot(int32 SlotIndex);

    AFrostwakeItemPickupActor* DropFirstInventoryItem();

    void SelectNextItem();
    void SelectPrevItem();
    // Send the local hotbar selection to the server (so it can publish the visible-to-all held item).
    void PublishSelectedSlot();

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
};
