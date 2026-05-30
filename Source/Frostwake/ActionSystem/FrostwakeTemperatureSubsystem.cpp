#include "ActionSystem/FrostwakeTemperatureSubsystem.h"

#include "ActionSystem/FrostwakeHeatSourceComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void UFrostwakeTemperatureSubsystem::RegisterHeatSource(UFrostwakeHeatSourceComponent* HeatSource)
{
	if (HeatSource)
	{
		HeatSources.AddUnique(HeatSource);
	}
}

void UFrostwakeTemperatureSubsystem::UnregisterHeatSource(UFrostwakeHeatSourceComponent* HeatSource)
{
	HeatSources.RemoveAll([HeatSource](const TWeakObjectPtr<UFrostwakeHeatSourceComponent>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == HeatSource;
	});
}

float UFrostwakeTemperatureSubsystem::ComputeTemperatureAt(const FVector& Location) const
{
	float Temperature = GlobalTemperature;
	for (const TWeakObjectPtr<UFrostwakeHeatSourceComponent>& Entry : HeatSources)
	{
		if (const UFrostwakeHeatSourceComponent* HeatSource = Entry.Get())
		{
			Temperature += HeatSource->ContributionAt(Location);
		}
	}
	return Temperature;
}

UFrostwakeTemperatureSubsystem* UFrostwakeTemperatureSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine || !WorldContext)
	{
		return nullptr;
	}
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull))
	{
		return World->GetSubsystem<UFrostwakeTemperatureSubsystem>();
	}
	return nullptr;
}
