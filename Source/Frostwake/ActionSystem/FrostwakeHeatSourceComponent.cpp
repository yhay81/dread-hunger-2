#include "ActionSystem/FrostwakeHeatSourceComponent.h"

#include "ActionSystem/FrostwakeTemperatureSubsystem.h"

UFrostwakeHeatSourceComponent::UFrostwakeHeatSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFrostwakeHeatSourceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UFrostwakeTemperatureSubsystem* Subsystem = UFrostwakeTemperatureSubsystem::Get(this))
	{
		Subsystem->RegisterHeatSource(this);
	}
}

void UFrostwakeHeatSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFrostwakeTemperatureSubsystem* Subsystem = UFrostwakeTemperatureSubsystem::Get(this))
	{
		Subsystem->UnregisterHeatSource(this);
	}

	Super::EndPlay(EndPlayReason);
}

float UFrostwakeHeatSourceComponent::ContributionAt(const FVector& SampleLocation) const
{
	if (!bHeatActive || HeatOutput <= 0.0f || HeatRadius <= 0.0f)
	{
		return 0.0f;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.0f;
	}

	const float Distance = FVector::Dist(Owner->GetActorLocation(), SampleLocation);
	if (Distance >= HeatRadius)
	{
		return 0.0f;
	}

	// 1 at the source, 0 at the radius; raised to the falloff exponent so heat concentrates near the source.
	const float Normalized = 1.0f - (Distance / HeatRadius);
	return HeatOutput * FMath::Pow(Normalized, FMath::Max(0.0f, FalloffExponent));
}
