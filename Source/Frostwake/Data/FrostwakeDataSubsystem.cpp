#include "Data/FrostwakeDataSubsystem.h"

#include "Data/FrostwakeItemDefinition.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

DEFINE_LOG_CATEGORY(LogFrostwakeData);

void UFrostwakeDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadAllDefinitions();
}

void UFrostwakeDataSubsystem::ReloadAllDefinitions()
{
	LoadItemDefinitions();
	// Wave 1 extends this with weapons/recipes/roles/perks/damage-types using the same parse path.
}

UFrostwakeItemDefinition* UFrostwakeDataSubsystem::GetItemDefinition(FName ItemId) const
{
	const TObjectPtr<UFrostwakeItemDefinition>* Found = ItemDefinitions.Find(ItemId);
	return Found ? *Found : nullptr;
}

void UFrostwakeDataSubsystem::LoadItemDefinitions()
{
	ItemDefinitions.Reset();

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
