#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FrostwakeDamageTypeDefinition.generated.h"

/**
 * Data-driven damage type (plan §6: DH_DamageType派生 DT_Cold/Drowning/Starvation/Poison/Piercing…
 * with flags, §3.17). Registered under PrimaryAssetType "FrostwakeDamageType".
 *
 * Used by AdjustDamage (type modifiers / resistances) before ModifyHealth in Phase 2.
 */
UCLASS(BlueprintType)
class FROSTWAKE_API UFrostwakeDamageTypeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable content identifier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|DamageType")
	FName DamageTypeId = NAME_None;

	/** Designer-facing label (placeholder pre-ship, §2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|DamageType")
	FString DisplayName;

	/** Damage taxonomy tags (Damage.* / Status.Damage.*). Assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|DamageType")
	FGameplayTagContainer DamageTypeTags;

	/** If true, this damage is reduced by the target's matching resistance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|DamageType")
	bool bAffectedByResistance = true;

	/** If true, this damage can deplete ReservedHealth (carcass) rather than living Health. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|DamageType")
	bool bAffectsReservedHealth = false;

	/** Global multiplier applied to all incoming damage of this type (balance knob). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|DamageType")
	float DamageMultiplier = 1.f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("FrostwakeDamageType")), DamageTypeId);
	}
};
