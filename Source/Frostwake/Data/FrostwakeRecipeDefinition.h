#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FrostwakeRecipeDefinition.generated.h"

/**
 * Data-driven crafting recipe (plan §6: recipes.txt CR_* + §3.19 families/rules/methods).
 * Registered under PrimaryAssetType "FrostwakeRecipe" (/Game/Data/Recipes).
 *
 * The 47-recipe content set is seeded from recipes.txt in Wave 1 (§2: material maps / times
 * are mechanics+numbers = allowed; no DH names/text copied).
 */
UCLASS(BlueprintType)
class FROSTWAKE_API UFrostwakeRecipeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable content identifier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Recipe")
	FName RecipeId = NAME_None;

	/** Designer-facing label (placeholder pre-ship, §2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Recipe")
	FString DisplayName;

	/** Ingredients: item ItemId -> required count. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Recipe")
	TMap<FName, int32> Materials;

	/** Item produced (its ItemId). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Recipe")
	FName OutputItemId = NAME_None;

	/** Quantity produced per craft. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Recipe")
	int32 OutputCount = 1;

	/** Seconds to craft. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Recipe")
	float CraftSeconds = 0.f;

	/** Crafting taxonomy tags (Crafting.Family.* / Crafting.Method.* / Crafting.Rule.*). Assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Recipe")
	FGameplayTagContainer RecipeTags;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("FrostwakeRecipe")), RecipeId);
	}
};
