#pragma once

#include "CoreMinimal.h"
#include "Data/FrostwakeItemDefinition.h"
#include "FrostwakeWeaponDefinition.generated.h"

/**
 * A weapon is a specialized item (plan §9.1-1): it inherits every item field and adds
 * combat numbers. Registered under its own PrimaryAssetType "FrostwakeWeapon" so the
 * AssetManager can scan weapons independently (DefaultGame.ini, /Game/Data/Weapons).
 *
 * Damage numbers / weapon coefficients are seeded from item_stats.txt in Wave 1 (§2: numbers OK).
 */
UCLASS(BlueprintType)
class FROSTWAKE_API UFrostwakeWeaponDefinition : public UFrostwakeItemDefinition
{
	GENERATED_BODY()

public:
	/** Base damage applied on a successful hit, before damage-type modifiers / resistances (Phase 2 §6). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Weapon")
	float BaseDamage = 0.f;

	/** Damage-type this weapon deals; resolves to a UFrostwakeDamageTypeDefinition by id. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Weapon")
	FName DamageTypeId = NAME_None;

	/** Seconds between attacks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Weapon")
	float AttackInterval = 1.f;

	/** Effective range in cm (melee = short, ranged = long). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Weapon")
	float Range = 0.f;

protected:
	virtual FPrimaryAssetType GetItemAssetType() const override { return FPrimaryAssetType(TEXT("FrostwakeWeapon")); }
};
