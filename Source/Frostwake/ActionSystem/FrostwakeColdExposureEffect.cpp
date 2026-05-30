#include "ActionSystem/FrostwakeColdExposureEffect.h"

#include "ActionSystem/FrostwakeActionComponent.h"
#include "FrostwakeCharacter.h"
#include "FrostwakeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UFrostwakeColdExposureEffect::UFrostwakeColdExposureEffect()
{
	EffectId = FName(TEXT("ColdExposure"));
	Duration = -1.f; // until removed (Warmth recovers)
	// (EffectTags Status.Cold is descriptive only and unused here; left unset to avoid touching a native
	//  gameplay tag at CDO-construction time.)
}

void UFrostwakeColdExposureEffect::OnApplied(UFrostwakeActionComponent* Component)
{
	Super::OnApplied(Component); // base built-in modifier (none here; this effect ticks instead)
	OwningComponent = Component;

	// Bite immediately so the effect is observable the moment it lands, then keep ticking.
	HypothermiaTick();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TickTimer, this, &UFrostwakeColdExposureEffect::HypothermiaTick, TickInterval, /*bLoop*/ true);
	}
}

void UFrostwakeColdExposureEffect::OnRemoved(UFrostwakeActionComponent* Component)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimer);
	}
	OwningComponent = nullptr;
	Super::OnRemoved(Component);
}

void UFrostwakeColdExposureEffect::HypothermiaTick()
{
	UFrostwakeActionComponent* Component = OwningComponent.Get();
	if (!Component)
	{
		return;
	}

	// Route the drain through the typed damage path (DT_Cold) so it downs the player + emits telemetry
	// like any other damage — no raw attribute writes.
	if (AFrostwakeCharacter* Character = Cast<AFrostwakeCharacter>(Component->GetOwner()))
	{
		Character->ApplyServerDamage(HealthPerTick, FrostwakeTags::Damage_Cold, Character);
	}
}
