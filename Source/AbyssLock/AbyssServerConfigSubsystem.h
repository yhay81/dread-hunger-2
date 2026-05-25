#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AbyssServerConfigSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FAbyssServerConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    FString ServerName = TEXT("Frostwake Test Server");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    FString Region = TEXT("jp");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    int32 MaxPlayers = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    FString Map = TEXT("L_IcebreakerWhitebox");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    FString Ruleset = TEXT("standard");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    FString AdminTokenEnv = TEXT("FROSTWAKE_ADMIN_TOKEN");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    FString BanlistPath = TEXT("Saved/Config/banlist.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    FString LogPath = TEXT("Saved/Logs/server.jsonl");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    bool bAdvertise = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss|Server")
    bool bAutoShutdownAfterMatch = false;
};

UCLASS()
class ABYSSLOCK_API UAbyssServerConfigSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Server")
    FAbyssServerConfig GetServerConfig() const { return Config; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Server")
    bool WasLoadedFromFile() const { return bLoadedFromFile; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Server")
    FString GetLoadedConfigPath() const { return LoadedConfigPath; }

private:
    bool LoadFromJsonFile(const FString& ConfigPath);
    FString ResolveConfigPath() const;

    UPROPERTY()
    FAbyssServerConfig Config;

    UPROPERTY()
    FString LoadedConfigPath;

    UPROPERTY()
    bool bLoadedFromFile = false;
};
