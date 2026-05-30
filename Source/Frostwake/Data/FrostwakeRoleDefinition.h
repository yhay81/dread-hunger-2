#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FrostwakeTypes.h"
#include "FrostwakeRoleDefinition.generated.h"

/**
 * Data-driven player role (plan §6: 8 roles, EPlayerTeamRole + role perks).
 * Registered under PrimaryAssetType "FrostwakeRole" (/Game/Data/Roles).
 *
 * Role identity/perks are mechanics (allowed §2); names/theme are DH-near placeholders
 * originalized before public exposure.
 */
UCLASS(BlueprintType)
class FROSTWAKE_API UFrostwakeRoleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable content identifier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Role")
	FName RoleId = NAME_None;

	/** Designer-facing label (placeholder pre-ship, §2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Role")
	FString DisplayName;

	/** Which cooperative team this role belongs to (reuses the stable boundary enum). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Role")
	EFrostwakeTeam Team = EFrostwakeTeam::Unassigned;

	/** Role taxonomy tags (Role.*). Assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Role")
	FGameplayTagContainer RoleTags;

	/** Initial loadout: item ItemIds granted at match start. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Role")
	TArray<FName> StartingItemIds;

	/** Role perks: PerkIds (resolve to UFrostwakePerkDefinition) applied while in this role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Role")
	TArray<FName> PerkIds;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("FrostwakeRole")), RoleId);
	}
};
