#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrostwakeServerConfigSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFrostwakeServerConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    FString ServerName = TEXT("Frostwake Test Server");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    FString Region = TEXT("jp");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    int32 MaxPlayers = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    FString Map = TEXT("L_IcebreakerWhitebox");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    FString Ruleset = TEXT("standard");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    FString AdminTokenEnv = TEXT("FROSTWAKE_ADMIN_TOKEN");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    FString BanlistPath = TEXT("Saved/Config/banlist.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    FString LogPath = TEXT("Saved/Logs/server.jsonl");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    bool bAdvertise = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Server")
    bool bAutoShutdownAfterMatch = false;
};

UCLASS()
class FROSTWAKE_API UFrostwakeServerConfigSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Server")
    FFrostwakeServerConfig GetServerConfig() const { return Config; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Server")
    bool WasLoadedFromFile() const { return bLoadedFromFile; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Server")
    FString GetLoadedConfigPath() const { return LoadedConfigPath; }

private:
    bool LoadFromJsonFile(const FString& ConfigPath);
    FString ResolveConfigPath() const;

    UPROPERTY()
    FFrostwakeServerConfig Config;

    UPROPERTY()
    FString LoadedConfigPath;

    UPROPERTY()
    bool bLoadedFromFile = false;
};
