#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "FrostwakeActionEffect.generated.h"

class UFrostwakeActionComponent;

/**
 * Base buff / debuff (plan §3.2 Tier 2: cold/hunger/poison, role & talisman perks, saboteur debuffs).
 * Server-authoritative; applied to and removed from a UFrostwakeActionComponent.
 *
 * The base carries an OPTIONAL built-in attribute modifier so the most common case (a perk that
 * adds/scales an attribute, plan §3.15) needs no subclass — that is enough to prove grant/remove/
 * attribute-change end to end. Richer effects override OnApplied/OnRemoved.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, Abstract)
class FROSTWAKE_API UFrostwakeActionEffect : public UObject
{
	GENERATED_BODY()

public:
	/** Stable id of this effect instance class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect")
	FName EffectId = NAME_None;

	/** Tags describing the effect (Status.*). Assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect")
	FGameplayTagContainer EffectTags;

	/** Lifetime in seconds: 0 = instant (applied then immediately removed), <0 = until removed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect")
	float Duration = -1.f;

	/** If true, OnApplied adds Magnitude to the target attribute and OnRemoved reverses it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect")
	bool bModifiesAttribute = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect", meta=(EditCondition="bModifiesAttribute"))
	EFrostwakeAttribute Attribute = EFrostwakeAttribute::Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Effect", meta=(EditCondition="bModifiesAttribute"))
	float Magnitude = 0.f;

	/** Called by the component when granted (server). Default applies the built-in modifier. */
	virtual void OnApplied(UFrostwakeActionComponent* Component);

	/** Called by the component when removed/expired (server). Default reverses the built-in modifier. */
	virtual void OnRemoved(UFrostwakeActionComponent* Component);

	//~ Route UObject world lookup through the outer component so timers / GetWorld() work.
	virtual UWorld* GetWorld() const override;
};
