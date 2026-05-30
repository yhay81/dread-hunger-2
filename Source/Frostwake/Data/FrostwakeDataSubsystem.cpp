#include "Data/FrostwakeDataSubsystem.h"

#include "Data/FrostwakeItemDefinition.h"
#include "Data/FrostwakeDamageTypeDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

DEFINE_LOG_CATEGORY(LogFrostwakeData);

namespace
{
// Synchronously load every PrimaryDataAsset of a type from the AssetManager (the baked .uasset under
// /Game/Data/<type>, scanned per DefaultGame.ini). Empty if nothing is baked yet.
template <typename TDef>
TArray<TDef*> LoadBakedPrimaryAssets(const TCHAR* TypeName)
{
	TArray<TDef*> Result;
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
	{
		return Result;
	}
	const FPrimaryAssetType Type(TypeName);
	TArray<FPrimaryAssetId> Ids;
	AssetManager->GetPrimaryAssetIdList(Type, Ids);
	if (Ids.Num() == 0)
	{
		return Result;
	}
	if (const TSharedPtr<FStreamableHandle> Handle = AssetManager->LoadPrimaryAssets(Ids))
	{
		Handle->WaitUntilComplete();
	}
	for (const FPrimaryAssetId& Id : Ids)
	{
		if (TDef* Def = AssetManager->GetPrimaryAssetObject<TDef>(Id))
		{
			Result.Add(Def);
		}
	}
	return Result;
}
}

void UFrostwakeDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadAllDefinitions();
}

void UFrostwakeDataSubsystem::ReloadAllDefinitions()
{
	LoadItemDefinitions();
	LoadDamageTypeDefinitions();
	// Wave 1 extends this further (weapons/recipes/roles/perks) using the same parse path.
}

const UFrostwakeDamageTypeDefinition* UFrostwakeDataSubsystem::GetDamageTypeDefinitionForTag(const FGameplayTag& DamageTag) const
{
	if (!DamageTag.IsValid())
	{
		return nullptr;
	}
	// DamageTypeId is the full tag name (e.g. "Damage.Cold"), so the lookup is O(1) and avoids any
	// FGameplayTagContainer JSON binding.
	const TObjectPtr<UFrostwakeDamageTypeDefinition>* Found = DamageTypeDefinitions.Find(DamageTag.GetTagName());
	return Found ? Found->Get() : nullptr;
}

UFrostwakeItemDefinition* UFrostwakeDataSubsystem::GetItemDefinition(FName ItemId) const
{
	const TObjectPtr<UFrostwakeItemDefinition>* Found = ItemDefinitions.Find(ItemId);
	return Found ? *Found : nullptr;
}

void UFrostwakeDataSubsystem::LoadItemDefinitions()
{
	ItemDefinitions.Reset();

	// Prefer baked PrimaryDataAssets via the AssetManager (the cook-safe "data is asset" path). Fall back
	// to the loose JSON source (dev convenience) only if nothing is baked yet (run `-run=BakeFrostwakeData`).
	for (UFrostwakeItemDefinition* Def : LoadBakedPrimaryAssets<UFrostwakeItemDefinition>(TEXT("FrostwakeItem")))
	{
		if (Def && !Def->ItemId.IsNone())
		{
			ItemDefinitions.Add(Def->ItemId, Def);
		}
	}
	if (ItemDefinitions.Num() > 0)
	{
		UE_LOG(LogFrostwakeData, Log, TEXT("[DataSubsystem] Loaded %d item definition(s) from AssetManager (baked)"), ItemDefinitions.Num());
		return;
	}

	const FString SourceDir = FPaths::ProjectContentDir() / TEXT("Data/Items/source");

	TArray<FString> JsonFileNames;
	IFileManager::Get().FindFiles(JsonFileNames, *(SourceDir / TEXT("*.json")), /*Files*/ true, /*Directories*/ false);

	if (JsonFileNames.Num() == 0)
	{
		UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] No item JSON found under %s"), *SourceDir);
		return;
	}

	for (const FString& FileName : JsonFileNames)
	{
		const FString FullPath = SourceDir / FileName;

		FString Raw;
		if (!FFileHelper::LoadFileToString(Raw, *FullPath))
		{
			UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Failed to read %s"), *FullPath);
			continue;
		}

		// Source format: a top-level JSON array of item objects.
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		if (!FJsonSerializer::Deserialize(Reader, JsonArray))
		{
			UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Failed to parse JSON array in %s"), *FullPath);
			continue;
		}

		for (const TSharedPtr<FJsonValue>& Value : JsonArray)
		{
			const TSharedPtr<FJsonObject>* ItemObj = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(ItemObj) || !ItemObj->IsValid())
			{
				continue;
			}

			UFrostwakeItemDefinition* Def = NewObject<UFrostwakeItemDefinition>(this);

			// Bind JSON fields to UPROPERTYs by name (the §9.2 method). UClass is a UStruct
			// describing the object's property layout, so the object pointer is the OutStruct.
			if (!FJsonObjectConverter::JsonObjectToUStruct(
					ItemObj->ToSharedRef(), UFrostwakeItemDefinition::StaticClass(), Def, /*CheckFlags*/ 0, /*SkipFlags*/ 0))
			{
				UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Field binding failed for an entry in %s"), *FullPath);
				continue;
			}

			if (Def->ItemId.IsNone())
			{
				UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Skipping item with no ItemId in %s"), *FullPath);
				continue;
			}

			ItemDefinitions.Add(Def->ItemId, Def);
		}
	}

	UE_LOG(LogFrostwakeData, Log, TEXT("[DataSubsystem] Loaded %d item definition(s) from %s"),
		ItemDefinitions.Num(), *SourceDir);
}

void UFrostwakeDataSubsystem::LoadDamageTypeDefinitions()
{
	DamageTypeDefinitions.Reset();

	// Prefer baked PrimaryDataAssets (cook-safe); fall back to loose JSON (dev) — see LoadItemDefinitions.
	for (UFrostwakeDamageTypeDefinition* Def : LoadBakedPrimaryAssets<UFrostwakeDamageTypeDefinition>(TEXT("FrostwakeDamageType")))
	{
		if (Def && !Def->DamageTypeId.IsNone())
		{
			DamageTypeDefinitions.Add(Def->DamageTypeId, Def);
		}
	}
	if (DamageTypeDefinitions.Num() > 0)
	{
		UE_LOG(LogFrostwakeData, Log, TEXT("[DataSubsystem] Loaded %d damage type definition(s) from AssetManager (baked)"), DamageTypeDefinitions.Num());
		return;
	}

	const FString SourceDir = FPaths::ProjectContentDir() / TEXT("Data/DamageTypes/source");

	TArray<FString> JsonFileNames;
	IFileManager::Get().FindFiles(JsonFileNames, *(SourceDir / TEXT("*.json")), /*Files*/ true, /*Directories*/ false);

	if (JsonFileNames.Num() == 0)
	{
		UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] No damage-type JSON found under %s"), *SourceDir);
		return;
	}

	for (const FString& FileName : JsonFileNames)
	{
		const FString FullPath = SourceDir / FileName;

		FString Raw;
		if (!FFileHelper::LoadFileToString(Raw, *FullPath))
		{
			UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Failed to read %s"), *FullPath);
			continue;
		}

		TArray<TSharedPtr<FJsonValue>> JsonArray;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		if (!FJsonSerializer::Deserialize(Reader, JsonArray))
		{
			UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Failed to parse JSON array in %s"), *FullPath);
			continue;
		}

		for (const TSharedPtr<FJsonValue>& Value : JsonArray)
		{
			const TSharedPtr<FJsonObject>* TypeObj = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(TypeObj) || !TypeObj->IsValid())
			{
				continue;
			}

			UFrostwakeDamageTypeDefinition* Def = NewObject<UFrostwakeDamageTypeDefinition>(this);
			if (!FJsonObjectConverter::JsonObjectToUStruct(
					TypeObj->ToSharedRef(), UFrostwakeDamageTypeDefinition::StaticClass(), Def, /*CheckFlags*/ 0, /*SkipFlags*/ 0))
			{
				UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Field binding failed for an entry in %s"), *FullPath);
				continue;
			}

			if (Def->DamageTypeId.IsNone())
			{
				UE_LOG(LogFrostwakeData, Warning, TEXT("[DataSubsystem] Skipping damage type with no DamageTypeId in %s"), *FullPath);
				continue;
			}

			DamageTypeDefinitions.Add(Def->DamageTypeId, Def);
		}
	}

	UE_LOG(LogFrostwakeData, Log, TEXT("[DataSubsystem] Loaded %d damage type definition(s) from %s"),
		DamageTypeDefinitions.Num(), *SourceDir);
}
