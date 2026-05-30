#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FrostwakeTemperatureSubsystem.generated.h"

class UFrostwakeHeatSourceComponent;

/**
 * Shared temperature aggregator (plan §3.22-23, §9.1 step 6). Heat sources self-register; any system
 * samples the temperature at a world location:
 *     CurrentTemperature = GlobalTemperature + Σ active source.ContributionAt(location)
 * Survival reads it to drive Warmth (warmth Δ/sec = temperature: positive near a fire warms you,
 * negative in the cold cools you). The Ship boiler and environmental heat sources contribute via
 * UFrostwakeHeatSourceComponent. Weather will drive GlobalTemperature later (TOD + blizzard curves,
 * plan §3.22-23); for now it is a tunable ambient baseline.
 */
UCLASS()
class FROSTWAKE_API UFrostwakeTemperatureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterHeatSource(UFrostwakeHeatSourceComponent* HeatSource);
	void UnregisterHeatSource(UFrostwakeHeatSourceComponent* HeatSource);

	/** GlobalTemperature + Σ heat-source contributions at Location (Warmth pts/sec). */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Temperature")
	float ComputeTemperatureAt(const FVector& Location) const;

	/** Ambient baseline (Warmth pts/sec) with no heat sources. Negative = the world cools you. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Temperature")
	float GetGlobalTemperature() const { return GlobalTemperature; }

	/** Weather / host config sets the ambient baseline (plan §3.22-23). */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Temperature")
	void SetGlobalTemperature(float NewGlobalTemperature) { GlobalTemperature = NewGlobalTemperature; }

	/** Convenience accessor from any world context object. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Temperature", meta=(WorldContext="WorldContext"))
	static UFrostwakeTemperatureSubsystem* Get(const UObject* WorldContext);

private:
	// Ambient cold baseline on the 0..100 Warmth scale (pts/sec). DH reference (numbers/structure only,
	// IP §2): GlobalTemperature = TOD(night −0.015) + Blizzard(→ −1.0) on a normalized 0..1 warmth; seeded
	// here to preserve the prior survival decay feel (~0.25 pts/sec) until Weather drives it (plan §3.22-23).
	float GlobalTemperature = -0.25f;

	// Weak so a destroyed heat-source actor cannot dangle; cleaned up on EndPlay and lazily while summing.
	TArray<TWeakObjectPtr<UFrostwakeHeatSourceComponent>> HeatSources;
};
