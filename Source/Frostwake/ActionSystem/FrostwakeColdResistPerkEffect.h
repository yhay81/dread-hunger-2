#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/FrostwakeActionEffect.h"
#include "FrostwakeColdResistPerkEffect.generated.h"

class UFrostwakeActionComponent;

/**
 * Cold-resistance perk (plan §3.20 / §6 "roles + perks"; cf. Perk.Talisman.ColdResist). The FIRST use of
 * the Action System for its design purpose — a perk — proving the §8 rule end to end: a modifier (here a
 * damage resistance) is supplied by an ActionEffect, never a raw method.
 *
 * While worn it registers an additive resistance fraction against Damage.Cold on the owning
 * UFrostwakeActionComponent; AFrostwakeCharacter::AdjustDamage multiplies incoming cold damage by the
 * resulting (1 - fraction). On removal the resistance is dropped. The fraction follows the DH curve
 * convention (×0.01: 0.5 = a 50% reduction); when the perk DATA type is seeded (backlog), this magnitude +
 * the resisted tag move into a PerkDefinition and this stays the runtime carrier.
 */
UCLASS()
class FROSTWAKE_API UFrostwakeColdResistPerkEffect : public UFrostwakeActionEffect
{
	GENERATED_BODY()

public:
	UFrostwakeColdResistPerkEffect();

	virtual void OnApplied(UFrostwakeActionComponent* Component) override;
	virtual void OnRemoved(UFrostwakeActionComponent* Component) override;

	/** Fraction of cold damage removed while worn (0..1; 0.5 = 50% resist, DH curves ×0.01). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Perk")
	float ResistanceFraction = 0.5f;
};
