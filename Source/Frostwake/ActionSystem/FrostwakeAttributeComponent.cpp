#include "ActionSystem/FrostwakeAttributeComponent.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UFrostwakeAttributeComponent::UFrostwakeAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Start full; data-config maxes may be lowered by a role definition before play.
	Health = MaxHealth;
	ReservedHealth = MaxReservedHealth;
	Hunger = MaxHunger;
	Warmth = MaxWarmth;
	Stamina = MaxStamina;
}

void UFrostwakeAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	DOREPLIFETIME_WITH_PARAMS_FAST(UFrostwakeAttributeComponent, Health, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFrostwakeAttributeComponent, ReservedHealth, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFrostwakeAttributeComponent, Hunger, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFrostwakeAttributeComponent, Warmth, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFrostwakeAttributeComponent, Stamina, Params);
}

float* UFrostwakeAttributeComponent::AttributePtr(EFrostwakeAttribute Attribute)
{
	switch (Attribute)
	{
	case EFrostwakeAttribute::Health:         return &Health;
	case EFrostwakeAttribute::ReservedHealth: return &ReservedHealth;
	case EFrostwakeAttribute::Hunger:         return &Hunger;
	case EFrostwakeAttribute::Warmth:         return &Warmth;
	case EFrostwakeAttribute::Stamina:        return &Stamina;
	default:                                  return nullptr;
	}
}

const float* UFrostwakeAttributeComponent::AttributePtr(EFrostwakeAttribute Attribute) const
{
	return const_cast<UFrostwakeAttributeComponent*>(this)->AttributePtr(Attribute);
}

float UFrostwakeAttributeComponent::MaxFor(EFrostwakeAttribute Attribute) const
{
	switch (Attribute)
	{
	case EFrostwakeAttribute::Health:         return MaxHealth;
	case EFrostwakeAttribute::ReservedHealth: return MaxReservedHealth;
	case EFrostwakeAttribute::Hunger:         return MaxHunger;
	case EFrostwakeAttribute::Warmth:         return MaxWarmth;
	case EFrostwakeAttribute::Stamina:        return MaxStamina;
	default:                                  return 0.f;
	}
}

void UFrostwakeAttributeComponent::MarkAttributeDirty(EFrostwakeAttribute Attribute)
{
	switch (Attribute)
	{
	case EFrostwakeAttribute::Health:         MARK_PROPERTY_DIRTY_FROM_NAME(UFrostwakeAttributeComponent, Health, this); break;
	case EFrostwakeAttribute::ReservedHealth: MARK_PROPERTY_DIRTY_FROM_NAME(UFrostwakeAttributeComponent, ReservedHealth, this); break;
	case EFrostwakeAttribute::Hunger:         MARK_PROPERTY_DIRTY_FROM_NAME(UFrostwakeAttributeComponent, Hunger, this); break;
	case EFrostwakeAttribute::Warmth:         MARK_PROPERTY_DIRTY_FROM_NAME(UFrostwakeAttributeComponent, Warmth, this); break;
	case EFrostwakeAttribute::Stamina:        MARK_PROPERTY_DIRTY_FROM_NAME(UFrostwakeAttributeComponent, Stamina, this); break;
	default: break;
	}
}

float UFrostwakeAttributeComponent::GetAttribute(EFrostwakeAttribute Attribute) const
{
	const float* Ptr = AttributePtr(Attribute);
	return Ptr ? *Ptr : 0.f;
}

float UFrostwakeAttributeComponent::GetMaxAttribute(EFrostwakeAttribute Attribute) const
{
	return MaxFor(Attribute);
}

void UFrostwakeAttributeComponent::SetAttribute(EFrostwakeAttribute Attribute, float NewValue)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	float* Ptr = AttributePtr(Attribute);
	if (!Ptr)
	{
		return;
	}

	const float OldValue = *Ptr;
	const float Clamped = FMath::Clamp(NewValue, 0.f, MaxFor(Attribute));
	if (Clamped == OldValue)
	{
		return;
	}

	*Ptr = Clamped;
	MarkAttributeDirty(Attribute);
	// Server-side observers fire immediately; clients fire from the matching OnRep.
	OnAttributeChanged.Broadcast(this, Attribute, Clamped, OldValue);
}

float UFrostwakeAttributeComponent::ModifyAttribute(EFrostwakeAttribute Attribute, float Delta)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return GetAttribute(Attribute);
	}

	SetAttribute(Attribute, GetAttribute(Attribute) + Delta);
	return GetAttribute(Attribute);
}

void UFrostwakeAttributeComponent::OnRep_Health(float OldValue)
{
	OnAttributeChanged.Broadcast(this, EFrostwakeAttribute::Health, Health, OldValue);
}

void UFrostwakeAttributeComponent::OnRep_ReservedHealth(float OldValue)
{
	OnAttributeChanged.Broadcast(this, EFrostwakeAttribute::ReservedHealth, ReservedHealth, OldValue);
}

void UFrostwakeAttributeComponent::OnRep_Hunger(float OldValue)
{
	OnAttributeChanged.Broadcast(this, EFrostwakeAttribute::Hunger, Hunger, OldValue);
}

void UFrostwakeAttributeComponent::OnRep_Warmth(float OldValue)
{
	OnAttributeChanged.Broadcast(this, EFrostwakeAttribute::Warmth, Warmth, OldValue);
}

void UFrostwakeAttributeComponent::OnRep_Stamina(float OldValue)
{
	OnAttributeChanged.Broadcast(this, EFrostwakeAttribute::Stamina, Stamina, OldValue);
}
