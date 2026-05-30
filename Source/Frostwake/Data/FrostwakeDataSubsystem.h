#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "FrostwakeDataSubsystem.generated.h"

class UFrostwakeItemDefinition;
class UFrostwakeDamageTypeDefinition;

DECLARE_LOG_CATEGORY_EXTERN(LogFrostwakeData, Log, All);

/**
 * Foundation data-ingestion subsystem (plan §9.2 — the ONE established pattern).
 *
 * Source of truth = plain-text JSON under Content/Data/<type>/source/*.json. At startup this
 * subsystem parses that text and binds it to typed UFrostwake*Definition instances via
 * FJsonObjectConverter — so "edit text -> see it in game" closes WITHOUT hand-authoring .uasset
 * files and WITHOUT an editor session. Everything here lives in the runtime module, so the loop
 * is verifiable by build_game alone (plan §5 gate).
 *
 * Wave 1 systems query definitions by their stable FName id (GetItemDefinition), never by asset path.
 *
 * The same JSON parse is intended to be reused by an editor "bake" sink later (commandlet ->
 * on-disk PrimaryDataAssets for AssetManager async-load/cook); the AssetManager scan rules are
 * pre-registered in DefaultGame.ini (plan §9.1-1) for that future sink and scan harmlessly empty
 * until then. Cooked-build note: loose JSON must be added to packaging's non-asset dirs, or use
 * the bake sink — see Content/Data/README.md.
 */
UCLASS()
class FROSTWAKE_API UFrostwakeDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End USubsystem interface

	/** Re-parse every JSON source and rebuild the registries. Safe to call at runtime to pick up text edits. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Data")
	void ReloadAllDefinitions();

	/** Resolve an item by its stable id. Returns nullptr if unknown. */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Data")
	UFrostwakeItemDefinition* GetItemDefinition(FName ItemId) const;

	/** Number of loaded item definitions (used by the connectivity smoke). */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Data")
	int32 GetNumItemDefinitions() const { return ItemDefinitions.Num(); }

	/** Resolve a damage type by a Damage.* gameplay tag (linear over the small set). nullptr if unknown. */
	const UFrostwakeDamageTypeDefinition* GetDamageTypeDefinitionForTag(const FGameplayTag& DamageTag) const;

	/** Number of loaded damage type definitions (connectivity smoke). */
	UFUNCTION(BlueprintCallable, Category="Frostwake|Data")
	int32 GetNumDamageTypeDefinitions() const { return DamageTypeDefinitions.Num(); }

private:
	/** Parse Content/Data/Items/source/*.json into ItemDefinitions. */
	void LoadItemDefinitions();

	/** Parse Content/Data/DamageTypes/source/*.json into DamageTypeDefinitions. */
	void LoadDamageTypeDefinitions();

	/** ItemId -> definition. Owns the instances (UPROPERTY keeps them referenced). */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UFrostwakeItemDefinition>> ItemDefinitions;

	/** DamageTypeId -> definition. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UFrostwakeDamageTypeDefinition>> DamageTypeDefinitions;
};
