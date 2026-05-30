#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrostwakeHeatSourceComponent.generated.h"

/**
 * Marks the owning actor as a heat source for the shared temperature model (plan §3.22-23, §9.1 step 6).
 * A boiler / stove / brazier / fire radiates warmth that falls off with distance; the
 * UFrostwakeTemperatureSubsystem sums every active source at a sample point:
 *     CurrentTemperature = GlobalTemperature + Σ source.ContributionAt(location)
 * The component self-registers with that subsystem on BeginPlay and unregisters on EndPlay, so the
 * Ship (boiler) and Survival (warmth decay) share one model without referencing each other.
 *
 * Units: contribution is in Warmth-points/second on Frostwake's 0..100 Warmth scale. DH reference
 * (numbers/structure only, IP §2): a boiler's MaximumHeatOutput = 0.02 on a normalized 0..1 warmth;
 * re-tuned here to our scale. Original expression.
 */
UCLASS(ClassGroup=(Frostwake), meta=(BlueprintSpawnableComponent))
class FROSTWAKE_API UFrostwakeHeatSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFrostwakeHeatSourceComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Peak warmth contribution (pts/sec) at the source location, before distance falloff. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Frostwake|Heat")
	float HeatOutput = 2.0f;

	/** Beyond this distance (cm) the source contributes nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Frostwake|Heat")
	float HeatRadius = 700.0f;

	/** Falloff shape against the normalized distance: 1 = linear, 2 = quadratic (sharper near the source). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Frostwake|Heat")
	float FalloffExponent = 2.0f;

	/** Whether the source is currently emitting (e.g. a lit vs unlit boiler). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Frostwake|Heat")
	bool bHeatActive = true;

	UFUNCTION(BlueprintCallable, Category="Frostwake|Heat")
	void SetHeatActive(bool bActive) { bHeatActive = bActive; }

	/** Warmth contribution (pts/sec) at SampleLocation; 0 if inactive or out of range. */
	float ContributionAt(const FVector& SampleLocation) const;
};
