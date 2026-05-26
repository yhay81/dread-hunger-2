#include "AbyssServerConfigSubsystem.h"
#include "AbyssLockLog.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
void ReadStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, FString& OutValue)
{
    FString ParsedValue;
    if (Object.IsValid() && Object->TryGetStringField(FieldName, ParsedValue))
    {
        OutValue = ParsedValue;
    }
}

void ReadBoolField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, bool& OutValue)
{
    bool ParsedValue = false;
    if (Object.IsValid() && Object->TryGetBoolField(FieldName, ParsedValue))
    {
        OutValue = ParsedValue;
    }
}
} // namespace

void UAbyssServerConfigSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const FString ConfigPath = ResolveConfigPath();
    if (!LoadFromJsonFile(ConfigPath))
    {
        LoadedConfigPath = ConfigPath;
        UE_LOG(
            LogAbyssSession,
            Log,
            TEXT("Using default Frostwake server config. path=%s server=%s map=%s maxPlayers=%d"),
            *LoadedConfigPath,
            *Config.ServerName,
            *Config.Map,
            Config.MaxPlayers);
    }
}

FString UAbyssServerConfigSubsystem::ResolveConfigPath() const
{
    FString ConfigPath;
    if (FParse::Value(FCommandLine::Get(), TEXT("ServerConfig="), ConfigPath) && !ConfigPath.IsEmpty())
    {
        return FPaths::ConvertRelativePathToFull(ConfigPath);
    }

    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"), TEXT("ServerConfig.json"));
}

bool UAbyssServerConfigSubsystem::LoadFromJsonFile(const FString& ConfigPath)
{
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *ConfigPath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogAbyssSession, Warning, TEXT("Failed to parse server config JSON path=%s"), *ConfigPath);
        return false;
    }

    ReadStringField(RootObject, TEXT("serverName"), Config.ServerName);
    ReadStringField(RootObject, TEXT("region"), Config.Region);
    ReadStringField(RootObject, TEXT("map"), Config.Map);
    ReadStringField(RootObject, TEXT("ruleset"), Config.Ruleset);
    ReadStringField(RootObject, TEXT("adminTokenEnv"), Config.AdminTokenEnv);
    ReadStringField(RootObject, TEXT("banlistPath"), Config.BanlistPath);
    ReadStringField(RootObject, TEXT("logPath"), Config.LogPath);
    ReadBoolField(RootObject, TEXT("advertise"), Config.bAdvertise);
    ReadBoolField(RootObject, TEXT("autoShutdownAfterMatch"), Config.bAutoShutdownAfterMatch);

    double ParsedMaxPlayers = Config.MaxPlayers;
    if (RootObject->TryGetNumberField(TEXT("maxPlayers"), ParsedMaxPlayers))
    {
        Config.MaxPlayers = 8;
    }

    LoadedConfigPath = ConfigPath;
    bLoadedFromFile = true;

    UE_LOG(
        LogAbyssSession,
        Log,
        TEXT("Loaded Frostwake server config path=%s server=%s region=%s map=%s maxPlayers=%d advertise=%s"),
        *LoadedConfigPath,
        *Config.ServerName,
        *Config.Region,
        *Config.Map,
        Config.MaxPlayers,
        Config.bAdvertise ? TEXT("true") : TEXT("false"));

    return true;
}
