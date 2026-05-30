#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/FrostwakeAction.h"
#include "FrostwakeFogAbility.generated.h"

class UFrostwakeActionComponent;

/**
 * Saboteur "obscure vision" ability (cf. Ability.Saboteur.Fog) — the FIRST concrete UFrostwakeAction, the
 * Action half of the system going LIVE (review #1: previously AddAction/StartAction had zero callers). It
 * proves a planned ability is now expressible as DATA on the thickened base: a Stamina cost, a cooldown,
 * and a timed duration, with no bespoke lifecycle code.
 *
 * The vision-obscuring world effect itself belongs to a later vision/weather system; this slice establishes
 * and exercises the FW shape (cost / cooldown / duration / instigator). The placeholder numbers (abstracted,
 * not the DH thrall values) get retuned when the spell DATA type is seeded.
 */
UCLASS()
class FROSTWAKE_API UFrostwakeFogAbility : public UFrostwakeAction
{
	GENERATED_BODY()

public:
	UFrostwakeFogAbility();

	virtual void Start(UFrostwakeActionComponent* Component) override;
};
