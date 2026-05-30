#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FrostwakePerkDefinition.generated.h"

/**
 * How a perk's magnitude combines with an attribute (plan §3.15: ADDITIVE / MULTIPLICATIVE + pity).
 * Stable boundary enum (kept local to the perk definition; not part of the tag taxonomy).
 */
UENUM(BlueprintType)
enum class EFrostwakePerkModifierOp : uint8
{
	/** newValue = base + Magnitude */
	Additive,
	/** newValue = base * Magnitude */
	Multiplicative,
};

/**
 * Data-driven perk / talisman modifier (plan §6: EPlayerRolePerk / EPlayerTalismanPerk
 * = enumerations of attribute modifiers). Registered under PrimaryAssetType "FrostwakePerk".
 *
 * A perk is, mechanically, an attribute modifier applied via an ActionEffect (§3.2 Tier 2).
 */
UCLASS(BlueprintType)
class FROSTWAKE_API UFrostwakePerkDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable content identifier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Perk")
	FName PerkId = NAME_None;

	/** Designer-facing label (placeholder pre-ship, §2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Perk")
	FString DisplayName;

	/** Perk taxonomy tags (Perk.Role.* / Perk.Talisman.*). Assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Perk")
	FGameplayTagContainer PerkTags;

	/** Tag of the attribute this perk modifies (e.g. an Attribute.* / Status.* tag). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Perk")
	FGameplayTag TargetAttributeTag;

	/** How Magnitude combines with the target attribute. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Perk")
	EFrostwakePerkModifierOp ModifierOp = EFrostwakePerkModifierOp::Additive;

	/** Modifier magnitude (additive delta or multiplicative factor). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Perk")
	float Magnitude = 0.f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("FrostwakePerk")), PerkId);
	}
};
