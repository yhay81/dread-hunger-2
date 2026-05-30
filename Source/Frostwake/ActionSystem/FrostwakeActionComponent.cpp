#include "ActionSystem/FrostwakeActionComponent.h"

#include "ActionSystem/FrostwakeAction.h"
#include "ActionSystem/FrostwakeActionEffect.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UFrostwakeActionComponent::UFrostwakeActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFrostwakeActionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		CachedAttributes = Owner->FindComponentByClass<UFrostwakeAttributeComponent>();
	}
}

UFrostwakeAttributeComponent* UFrostwakeActionComponent::GetAttributes() const
{
	if (CachedAttributes)
	{
		return CachedAttributes;
	}
	return GetOwner() ? GetOwner()->FindComponentByClass<UFrostwakeAttributeComponent>() : nullptr;
}

UFrostwakeActionEffect* UFrostwakeActionComponent::ApplyEffect(TSubclassOf<UFrostwakeActionEffect> EffectClass)
{
	if (GetOwnerRole() != ROLE_Authority || !*EffectClass)
	{
		return nullptr;
	}

	UFrostwakeActionEffect* Effect = NewObject<UFrostwakeActionEffect>(this, EffectClass);
	if (!Effect)
	{
		return nullptr;
	}

	Effect->OnApplied(this);

	if (Effect->Duration == 0.f)
	{
		// Instant one-shot: the applied change is permanent; nothing to track or reverse.
		return Effect;
	}

	ActiveEffects.Add(Effect);

	if (Effect->Duration > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle Handle;
			FTimerDelegate Del = FTimerDelegate::CreateUObject(this, &UFrostwakeActionComponent::FinishEffect, Effect);
			World->GetTimerManager().SetTimer(Handle, Del, Effect->Duration, /*bLoop*/ false);
		}
	}

	return Effect;
}

void UFrostwakeActionComponent::RemoveEffect(UFrostwakeActionEffect* Effect)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	FinishEffect(Effect);
}

void UFrostwakeActionComponent::FinishEffect(UFrostwakeActionEffect* Effect)
{
	if (!Effect || !ActiveEffects.Contains(Effect))
	{
		return;
	}
	Effect->OnRemoved(this);
	ActiveEffects.Remove(Effect);
}

void UFrostwakeActionComponent::AddDamageResistance(UFrostwakeActionEffect* Source, FGameplayTag DamageType, float Fraction)
{
	if (GetOwnerRole() != ROLE_Authority || !DamageType.IsValid() || Fraction <= 0.f)
	{
		return;
	}
	ActiveResistances.Add(FFrostwakeActiveResistance{ DamageType, Fraction, Source });
}

void UFrostwakeActionComponent::RemoveDamageResistancesFrom(UFrostwakeActionEffect* Source)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	ActiveResistances.RemoveAll([Source](const FFrostwakeActiveResistance& Entry)
	{
		return Entry.Source.Get() == Source;
	});
}

float UFrostwakeActionComponent::GetDamageResistanceMultiplier(FGameplayTag DamageType) const
{
	if (!DamageType.IsValid())
	{
		return 1.f;
	}
	float TotalFraction = 0.f;
	for (const FFrostwakeActiveResistance& Entry : ActiveResistances)
	{
		if (Entry.DamageType == DamageType)
		{
			TotalFraction += Entry.Fraction;
		}
	}
	return 1.f - FMath::Clamp(TotalFraction, 0.f, 1.f);
}

UFrostwakeAction* UFrostwakeActionComponent::AddAction(TSubclassOf<UFrostwakeAction> ActionClass)
{
	if (GetOwnerRole() != ROLE_Authority || !*ActionClass)
	{
		return nullptr;
	}

	UFrostwakeAction* Action = NewObject<UFrostwakeAction>(this, ActionClass);
	if (Action)
	{
		Actions.Add(Action);
	}
	return Action;
}

bool UFrostwakeActionComponent::StartAction(UFrostwakeAction* Action)
{
	if (GetOwnerRole() != ROLE_Authority || !Action || !Actions.Contains(Action))
	{
		return false;
	}
	if (!Action->CanStart(this))
	{
		return false;
	}
	Action->Start(this);
	return true;
}

void UFrostwakeActionComponent::StopAction(UFrostwakeAction* Action)
{
	if (GetOwnerRole() != ROLE_Authority || !Action)
	{
		return;
	}
	Action->Stop(this);
}
