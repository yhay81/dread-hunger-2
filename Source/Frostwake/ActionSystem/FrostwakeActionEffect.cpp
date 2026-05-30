#include "ActionSystem/FrostwakeActionEffect.h"

#include "ActionSystem/FrostwakeActionComponent.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "Engine/World.h"

void UFrostwakeActionEffect::OnApplied(UFrostwakeActionComponent* Component)
{
	if (!bModifiesAttribute || !Component)
	{
		return;
	}

	if (UFrostwakeAttributeComponent* Attributes = Component->GetAttributes())
	{
		Attributes->ModifyAttribute(Attribute, Magnitude);
	}
}

void UFrostwakeActionEffect::OnRemoved(UFrostwakeActionComponent* Component)
{
	if (!bModifiesAttribute || !Component)
	{
		return;
	}

	// Foundation reverse is the additive inverse; multiplicative/stacking semantics arrive in Phase 2.
	if (UFrostwakeAttributeComponent* Attributes = Component->GetAttributes())
	{
		Attributes->ModifyAttribute(Attribute, -Magnitude);
	}
}

UWorld* UFrostwakeActionEffect::GetWorld() const
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
