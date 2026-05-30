#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FrostwakeItemDefinition.generated.h"

/**
 * Coarse item family (plan §4 taxonomy: Item.{Weapon,Food,Fuel,Tool,Resource,Quest}).
 * Stable boundary enum kept local to the item definition; Gameplay Tags are the modern,
 * extensible axis (§3.1). Defined here (not in the shared FrostwakeTypes.h) to keep the
 * foundation track's edits within Data/.
 */
UENUM(BlueprintType)
enum class EFrostwakeItemCategory : uint8
{
	None     UMETA(DisplayName = "None"),
	Weapon   UMETA(DisplayName = "Weapon"),
	Food     UMETA(DisplayName = "Food"),
	Fuel     UMETA(DisplayName = "Fuel"),
	Tool     UMETA(DisplayName = "Tool"),
	Resource UMETA(DisplayName = "Resource"),
	Quest    UMETA(DisplayName = "Quest"),
};

/**
 * Data-driven definition for an inventory item (plan §3.2 "logic in C++, data in assets").
 *
 * Gameplay code resolves items by their stable FName ItemId via the AssetManager
 * PrimaryAssetType "FrostwakeItem" (registered in DefaultGame.ini, plan §9.1-1) or via
 * UFrostwakeDataSubsystem (the foundation ingestion path, plan §9.2).
 *
 * One-way-door §8 (LOCKED 2026-05-30): identity = a single BARE canonical FName ItemId
 * (e.g. "Ration", NOT "Item_Ration") — the PrimaryAssetType ("FrostwakeItem") is the namespace,
 * so no per-id prefix. Gameplay inventory holds this SAME FName, so resolving a held item is just
 * DataSubsystem.GetItemDefinition(InventoryItemId). ItemId is STABLE; placeholder DisplayName / art
 * are DH-near now and originalized before any public exposure (§2).
 *
 * Shared defaults live on this base; specialized items (e.g. weapons) derive and override.
 */
UCLASS(BlueprintType)
class FROSTWAKE_API UFrostwakeItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable content identifier (Name half of the PrimaryAssetId). Never renamed post-ship. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Item")
	FName ItemId = NAME_None;

	/** Designer-facing label. Placeholder string for the prototype; becomes localized FText pre-ship (§2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Item")
	FString DisplayName;

	/** Coarse legacy category. Kept as a stable boundary enum; Gameplay Tags are the modern axis (§3.1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Item")
	EFrostwakeItemCategory Category = EFrostwakeItemCategory::None;

	/** Gameplay tags (e.g. Item.Food.*, Item.Fuel.*). Taxonomy is owned by the tag-header track; values assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Item")
	FGameplayTagContainer ItemTags;

	/** Max units per inventory slot (item_stats.txt seeds real values in Wave 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Item")
	int32 MaxStack = 1;

	/** Carried weight per unit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Item")
	float Weight = 0.f;

	/** Food only (Category == Food): hunger removed when eaten (the DH-semantic Hunger attribute falls).
	 *  0 = not nourishing. Seeded from item_stats.txt food values in the items(55) content slice. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Item")
	float HungerRestore = 0.f;

	//~ Begin UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GetItemAssetType(), ItemId);
	}
	//~ End UPrimaryDataAsset interface

protected:
	/**
	 * PrimaryAssetType (Type half of the PrimaryAssetId) this definition registers under.
	 * Derived definitions override this to register under their own type/scan directory.
	 * Constructed at call time (not a static global) to avoid FName static-init ordering issues.
	 */
	virtual FPrimaryAssetType GetItemAssetType() const { return FPrimaryAssetType(TEXT("FrostwakeItem")); }
};
