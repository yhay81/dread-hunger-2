#include "ActionSystem/FrostwakeFogAbility.h"

#include "ActionSystem/FrostwakeActionComponent.h"
#include "FrostwakeLog.h"
#include "GameFramework/Actor.h"

UFrostwakeFogAbility::UFrostwakeFogAbility()
{
	ActionId = FName(TEXT("Saboteur.Fog"));
	CooldownSeconds = 30.f;                          // abstracted placeholder (DH thrall: 120-300s), retuned with spell data
	Duration = 8.f;                                  // the obscured-vision window
	CostAttribute = EFrostwakeAttribute::Stamina;    // Stamina's first real consumer
	Cost = 25.f;
	// (ActionTags Ability.Saboteur.Fog left unset at CDO-construction time — native tag access is deferred to
	//  runtime, same rule the effects follow.)
}

void UFrostwakeFogAbility::Start(UFrostwakeActionComponent* Component)
{
	Super::Start(Component); // base: consume Stamina, mark running, start cooldown + duration timers

	// Observable in normal play (the vision VFX is a later content concern). The behavioural assertion of the
	// cost/cooldown/duration lifecycle lives in FrostwakeDevSmoke::RunAbility.
	const AActor* Owner = Component ? Component->GetOwner() : nullptr;
	UE_LOG(LogFrostwakeGameplay, Log, TEXT("saboteur_ability_started ability=%s instigator=%s"),
		*ActionId.ToString(), *GetNameSafe(Owner));
}
