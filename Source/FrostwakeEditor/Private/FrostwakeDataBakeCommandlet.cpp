#include "FrostwakeDataBakeCommandlet.h"

#include "Data/FrostwakeItemDefinition.h"
#include "Data/FrostwakeDamageTypeDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogFrostwakeDataBake, Log, All);

namespace
{
// Asset object names can't contain '.', so "Damage.Cold" -> "Damage_Cold". The id FIELD keeps the dot;
// only the on-disk asset name is sanitized (the AssetManager indexes by the PrimaryAssetId = id field).
FString SanitizeAssetName(const FString& Id)
{
	FString Name = Id;
	Name.ReplaceInline(TEXT("."), TEXT("_"));
	return Name;
}

template <typename TDef>
int32 BakeJsonType(const TCHAR* SourceSubdir, const TCHAR* PackageDir, const TCHAR* IdFieldName)
{
	int32 BakedCount = 0;
	const FString SourceDir = FPaths::ProjectContentDir() / SourceSubdir;

	TArray<FString> JsonFileNames;
	IFileManager::Get().FindFiles(JsonFileNames, *(SourceDir / TEXT("*.json")), /*Files*/ true, /*Directories*/ false);

	for (const FString& FileName : JsonFileNames)
	{
		FString Raw;
		if (!FFileHelper::LoadFileToString(Raw, *(SourceDir / FileName)))
		{
			UE_LOG(LogFrostwakeDataBake, Warning, TEXT("Could not read %s"), *(SourceDir / FileName));
			continue;
		}

		TArray<TSharedPtr<FJsonValue>> JsonArray;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		if (!FJsonSerializer::Deserialize(Reader, JsonArray))
		{
			UE_LOG(LogFrostwakeDataBake, Warning, TEXT("Could not parse JSON array in %s"), *FileName);
			continue;
		}

		for (const TSharedPtr<FJsonValue>& Value : JsonArray)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Obj) || !Obj->IsValid())
			{
				continue;
			}

			FString IdStr;
			if (!(*Obj)->TryGetStringField(IdFieldName, IdStr) || IdStr.IsEmpty())
			{
				UE_LOG(LogFrostwakeDataBake, Warning, TEXT("Entry missing %s in %s"), IdFieldName, *FileName);
				continue;
			}

			const FString AssetName = TEXT("DA_") + SanitizeAssetName(IdStr);
			const FString PackagePath = FString(PackageDir) / AssetName;

			UPackage* Package = CreatePackage(*PackagePath);
			if (!Package)
			{
				continue;
			}

			TDef* Def = NewObject<TDef>(Package, FName(*AssetName), RF_Public | RF_Standalone);
			if (!Def || !FJsonObjectConverter::JsonObjectToUStruct(
					(*Obj).ToSharedRef(), TDef::StaticClass(), Def, /*CheckFlags*/ 0, /*SkipFlags*/ 0))
			{
				UE_LOG(LogFrostwakeDataBake, Warning, TEXT("Field binding failed for %s"), *IdStr);
				continue;
			}

			FAssetRegistryModule::AssetCreated(Def);
			Package->MarkPackageDirty();

			const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			if (UPackage::SavePackage(Package, Def, *Filename, SaveArgs))
			{
				++BakedCount;
			}
			else
			{
				UE_LOG(LogFrostwakeDataBake, Warning, TEXT("SavePackage failed for %s"), *Filename);
			}
		}
	}

	UE_LOG(LogFrostwakeDataBake, Display, TEXT("Baked %d asset(s) into %s"), BakedCount, PackageDir);
	return BakedCount;
}
}

UBakeFrostwakeDataCommandlet::UBakeFrostwakeDataCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UBakeFrostwakeDataCommandlet::Main(const FString& Params)
{
	const int32 Items = BakeJsonType<UFrostwakeItemDefinition>(TEXT("Data/Items/source"), TEXT("/Game/Data/Items"), TEXT("ItemId"));
	const int32 DamageTypes = BakeJsonType<UFrostwakeDamageTypeDefinition>(TEXT("Data/DamageTypes/source"), TEXT("/Game/Data/DamageTypes"), TEXT("DamageTypeId"));

	UE_LOG(LogFrostwakeDataBake, Display, TEXT("frostwake_data_bake_complete items=%d damageTypes=%d"), Items, DamageTypes);
	return 0;
}
