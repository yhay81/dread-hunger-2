#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"
#include "FrostwakeDataBakeCommandlet.generated.h"

/**
 * Bakes the JSON data sources (Content/Data/<type>/source/*.json) into on-disk PrimaryDataAsset .uasset
 * files under /Game/Data/<type>/, so the AssetManager (and cooked/shipping builds) resolve them — closing
 * the "loose JSON isn't packaged" gap (plan §9.5 step 3/4). The JSON stays the editable source; this is
 * the bake sink the DataSubsystem comment promised. Run:
 *   UnrealEditor-Cmd <proj> -run=BakeFrostwakeData
 */
UCLASS()
class FROSTWAKEEDITOR_API UBakeFrostwakeDataCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBakeFrostwakeDataCommandlet();

	virtual int32 Main(const FString& Params) override;
};
