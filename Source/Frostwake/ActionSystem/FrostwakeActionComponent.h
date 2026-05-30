#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FrostwakeActionComponent.generated.h"

class UFrostwakeAttributeComponent;
class UFrostwakeAction;
class UFrostwakeActionEffect;

/**
 * Host for an actor's actions and active effects (plan §3.2 Tier 2 Action System core, Looman-style).
 * Server-authoritative grant/remove; attribute changes flow through the sibling
 * UFrostwakeAttributeComponent (which replicates), so clients observe results without the effect/action
 * objects themselves being replicated yet (full effect replication is a Phase 2 concern).
 *
 * This is the minimal slice the dispatch asks for: apply an effect -> attribute changes; remove it ->
 * the change reverses; add/start/stop an action.
 */
UCLASS(ClassGroup=(Frostwake), meta=(BlueprintSpawnableComponent))
class FROSTWAKE_API UFrostwakeActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFrostwakeActionComponent();

	/** The sibling attribute component on the owning actor (cached at BeginPlay, found on demand otherwise). */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Action")
	UFrostwakeAttributeComponent* GetAttributes() const;

	/** Server: instantiate and apply an effect. Returns the live instance (or nullptr). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Frostwake|Action")
	UFrostwakeActionEffect* ApplyEffect(TSubclassOf<UFrostwakeActionEffect> EffectClass);

	/** Server: remove a previously applied effect (reverses it). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Frostwake|Action")
	void RemoveEffect(UFrostwakeActionEffect* Effect);

	/** Server: instantiate and register an action (does not start it). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Frostwake|Action")
	UFrostwakeAction* AddAction(TSubclassOf<UFrostwakeAction> ActionClass);

	/** Server: start a registered action if CanStart. Returns whether it started. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Frostwake|Action")
	bool StartAction(UFrostwakeAction* Action);

	/** Server: stop a running action. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Frostwake|Action")
	void StopAction(UFrostwakeAction* Action);

	UFUNCTION(BlueprintCallable, Category="Frostwake|Action")
	int32 GetActiveEffectCount() const { return ActiveEffects.Num(); }

	// ── Damage resistance (plan §3.17 resistance hook / §3.20 perks) ──────────────────────────────
	// Perks supply resistance through ActionEffects (never raw methods, §8): a worn perk effect calls
	// AddDamageResistance on apply and RemoveDamageResistancesFrom on removal. AFrostwakeCharacter::
	// AdjustDamage queries GetDamageResistanceMultiplier so a resisted Damage.* tag lands for less.

	/** Server: register a resistance fraction (0..1) against a Damage.* tag, attributed to Source. */
	void AddDamageResistance(UFrostwakeActionEffect* Source, FGameplayTag DamageType, float Fraction);

	/** Server: drop every resistance contributed by Source (called from the effect's OnRemoved). */
	void RemoveDamageResistancesFrom(UFrostwakeActionEffect* Source);

	/** Net incoming-damage multiplier for a damage type: clamp(1 - Σ matching fractions, 0, 1). 1 = none.
	 *  Combination is additive for this slice; multiplicative/pity stacking (§3.20) is a Phase 2 refinement. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Action")
	float GetDamageResistanceMultiplier(FGameplayTag DamageType) const;

protected:
	virtual void BeginPlay() override;

private:
	void FinishEffect(UFrostwakeActionEffect* Effect);

	// One active resistance contribution. Plain (non-UPROPERTY) struct: the contributing effect is kept
	// alive by ActiveEffects, so a weak ptr suffices to match removals.
	struct FFrostwakeActiveResistance
	{
		FGameplayTag DamageType;
		float Fraction = 0.f;
		TWeakObjectPtr<UFrostwakeActionEffect> Source;
	};

	UPROPERTY(Transient)
	TObjectPtr<UFrostwakeAttributeComponent> CachedAttributes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UFrostwakeActionEffect>> ActiveEffects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UFrostwakeAction>> Actions;

	TArray<FFrostwakeActiveResistance> ActiveResistances;
};
