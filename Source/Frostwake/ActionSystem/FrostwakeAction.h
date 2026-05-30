#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Engine/TimerHandle.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "FrostwakeAction.generated.h"

class UFrostwakeActionComponent;

/**
 * Base activatable ability (plan §3.2 Tier 2: interact / sabotage / use-item / role / saboteur abilities).
 * Server-authoritative; owned by a UFrostwakeActionComponent. Subclasses implement extra behaviour; the
 * base provides id/tags + the lifecycle the planned abilities NEED so they can be expressed as data, not
 * re-derived per subclass (review #1 / §8 one-way door — fix the FW shape before abilities load on it):
 *   - cooldown   : CanStart blocks until CooldownSeconds elapse from activation (DH spells: 120-300s).
 *   - cost       : Start consumes Cost from a resource attribute (default Stamina); CanStart gates on it.
 *   - duration   : if Duration > 0 the action auto-Stops after that long (timed abilities/buffs).
 *   - instigator : the activating actor, captured on Start (targeting/attribution).
 * Client replication of running-state is a deliberate later step (Actions aren't replicated objects yet;
 * their attribute/world EFFECTS replicate today) — tracked as the remaining ability-FW sub-item.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, Abstract)
class FROSTWAKE_API UFrostwakeAction : public UObject
{
	GENERATED_BODY()

public:
	/** Stable id of this action. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	FName ActionId = NAME_None;

	/** Tags describing/gating the action (Ability.*). Assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	FGameplayTagContainer ActionTags;

	/** Seconds before the action may be re-activated, measured from activation. 0 = no cooldown. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	float CooldownSeconds = 0.f;

	/** If > 0 the action auto-Stops this many seconds after Start (timed ability). 0 = manual stop only. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	float Duration = 0.f;

	/** Resource attribute consumed on Start (and gated by CanStart). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	EFrostwakeAttribute CostAttribute = EFrostwakeAttribute::Stamina;

	/** Amount of CostAttribute consumed on activation. 0 = free. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	float Cost = 0.f;

	/** Whether the action may start now: not already running, off cooldown, and enough resource. */
	virtual bool CanStart(UFrostwakeActionComponent* Component) const;

	/** Begin the action (server). Consumes cost, marks running, starts cooldown + duration timers. */
	virtual void Start(UFrostwakeActionComponent* Component);

	/** End the action (server). Clears running (cooldown keeps running from activation). */
	virtual void Stop(UFrostwakeActionComponent* Component);

	UFUNCTION(BlueprintCallable, Category="Frostwake|Action")
	bool IsRunning() const { return bRunning; }

	/** True while the post-activation cooldown is still elapsing. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Action")
	bool IsOnCooldown() const;

	//~ Route UObject world lookup through the outer component.
	virtual UWorld* GetWorld() const override;

protected:
	bool bRunning = false;

	/** World-seconds at which the cooldown ends (0 = never on cooldown). */
	float CooldownEndWorldSeconds = 0.f;

	FTimerHandle DurationTimer;

	// Weak: the component owns this action; cleared implicitly when the action is GC'd.
	TWeakObjectPtr<UFrostwakeActionComponent> OwningComponent;
	TWeakObjectPtr<AActor> Instigator;

private:
	/** Duration-timer callback: auto-stop a timed action. */
	void HandleDurationExpired();
};
