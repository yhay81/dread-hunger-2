#include "ActionSystem/FrostwakeAction.h"

#include "Engine/World.h"

bool UFrostwakeAction::CanStart(UFrostwakeActionComponent* /*Component*/) const
{
	return !bRunning;
}

void UFrostwakeAction::Start(UFrostwakeActionComponent* /*Component*/)
{
	bRunning = true;
}

void UFrostwakeAction::Stop(UFrostwakeActionComponent* /*Component*/)
{
	bRunning = false;
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
