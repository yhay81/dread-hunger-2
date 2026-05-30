#include "ActionSystem/FrostwakeAction.h"

#include "ActionSystem/FrostwakeActionComponent.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

bool UFrostwakeAction::CanStart(UFrostwakeActionComponent* Component) const
{
	if (bRunning || IsOnCooldown())
	{
		return false;
	}
	if (Cost > 0.f && Component)
	{
		const UFrostwakeAttributeComponent* Attributes = Component->GetAttributes();
		if (Attributes && Attributes->GetAttribute(CostAttribute) < Cost)
		{
			return false;
		}
	}
	return true;
}

void UFrostwakeAction::Start(UFrostwakeActionComponent* Component)
{
	bRunning = true;
	OwningComponent = Component;

	if (Component)
	{
		Instigator = Component->GetOwner();
		if (Cost > 0.f)
		{
			if (UFrostwakeAttributeComponent* Attributes = Component->GetAttributes())
			{
				Attributes->ModifyAttribute(CostAttribute, -Cost);
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (CooldownSeconds > 0.f)
		{
			CooldownEndWorldSeconds = World->GetTimeSeconds() + CooldownSeconds;
		}
		if (Duration > 0.f)
		{
			World->GetTimerManager().SetTimer(
				DurationTimer, this, &UFrostwakeAction::HandleDurationExpired, Duration, /*bLoop*/ false);
		}
	}
}

void UFrostwakeAction::Stop(UFrostwakeActionComponent* /*Component*/)
{
	bRunning = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimer);
	}
	// Cooldown is intentionally NOT cleared here: it runs from activation, so a finished action stays on
	// cooldown until CooldownSeconds elapse.
}

bool UFrostwakeAction::IsOnCooldown() const
{
	if (CooldownSeconds <= 0.f)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < CooldownEndWorldSeconds;
}

void UFrostwakeAction::HandleDurationExpired()
{
	Stop(OwningComponent.Get());
}

UWorld* UFrostwakeAction::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	if (const UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}
