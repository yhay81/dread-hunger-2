#include "ActionSystem/FrostwakeColdResistPerkEffect.h"

#include "ActionSystem/FrostwakeActionComponent.h"
#include "FrostwakeGameplayTags.h"

UFrostwakeColdResistPerkEffect::UFrostwakeColdResistPerkEffect()
{
	EffectId = FName(TEXT("ColdResistPerk"));
	Duration = -1.f; // worn until removed (e.g. talisman unequipped)
	// (PerkTags Perk.Talisman.ColdResist + the resisted tag are descriptive; left unset to avoid touching a
	//  native gameplay tag at CDO-construction time — same rule as UFrostwakeColdExposureEffect. The resisted
	//  tag is resolved at runtime in OnApplied below.)
}

void UFrostwakeColdResistPerkEffect::OnApplied(UFrostwakeActionComponent* Component)
{
	Super::OnApplied(Component); // base built-in modifier (none here; this perk supplies a resistance)
	if (Component)
	{
		Component->AddDamageResistance(this, FrostwakeTags::Damage_Cold, ResistanceFraction);
	}
}

void UFrostwakeColdResistPerkEffect::OnRemoved(UFrostwakeActionComponent* Component)
{
	if (Component)
	{
		Component->RemoveDamageResistancesFrom(this);
	}
	Super::OnRemoved(Component);
}
