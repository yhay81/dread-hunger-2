#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/FrostwakeActionEffect.h"
#include "Engine/TimerHandle.h"
#include "FrostwakeColdExposureEffect.generated.h"

class UFrostwakeActionComponent;

/**
 * Cold-exposure debuff (§3.17 / §3.22-23). The FIRST real consumer of the Action System (plan §9.5
 * step 5 SCAFFOLD → LIVE): from here on, combat / perk / effect modifiers go through Action / ActionEffect,
 * NOT raw methods.
 *
 * While applied (Survival applies it when Warmth bottoms out, removes it when Warmth recovers) it inflicts
 * periodic DT_Cold damage THROUGH the character's typed damage path — so hypothermia downs the player and
 * emits telemetry like any other damage source, with no raw attribute writes. A "richer" effect: it
 * overrides OnApplied/OnRemoved to run a periodic timer instead of the base's one-shot modifier.
 */
UCLASS()
class FROSTWAKE_API UFrostwakeColdExposureEffect : public UFrostwakeActionEffect
{
	GENERATED_BODY()

public:
	UFrostwakeColdExposureEffect();

	virtual void OnApplied(UFrostwakeActionComponent* Component) override;
	virtual void OnRemoved(UFrostwakeActionComponent* Component) override;

	/** Health lost per tick while exposed (inflicted as DT_Cold). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect")
	float HealthPerTick = 1.0f;

	/** Seconds between hypothermia ticks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect")
	float TickInterval = 1.0f;

private:
	void HypothermiaTick();

	// Weak: the component (and this effect) are owned by the character; cleared on removal.
	TWeakObjectPtr<UFrostwakeActionComponent> OwningComponent;

	FTimerHandle TickTimer;
};
