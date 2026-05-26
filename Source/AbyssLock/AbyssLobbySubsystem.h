#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AbyssLobbySubsystem.generated.h"

UENUM(BlueprintType)
enum class EAbyssLobbyRejectReason : uint8
{
    None UMETA(DisplayName = "None"),
    SteamUnavailable UMETA(DisplayName = "Steam Unavailable"),
    InvalidMetadata UMETA(DisplayName = "Invalid Metadata"),
    BuildMismatch UMETA(DisplayName = "Build Mismatch"),
    MapMismatch UMETA(DisplayName = "Map Mismatch"),
    LobbyFull UMETA(DisplayName = "Lobby Full"),
    LobbyLocked UMETA(DisplayName = "Lobby Locked"),
    AlreadyInMatch UMETA(DisplayName = "Already In Match"),
    EndpointUnavailable UMETA(DisplayName = "Endpoint Unavailable"),
    TravelFailed UMETA(DisplayName = "Travel Failed")
};

USTRUCT(BlueprintType)
struct FAbyssLobbyMetadata
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    int32 SchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString LobbyName = TEXT("Weekend Casual Lobby");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString LobbyType = TEXT("casual");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString BuildId = TEXT("AbyssLock-Win64-Development-local");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString MapId = TEXT("L_IcebreakerWhitebox");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString Ruleset = TEXT("standard");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    int32 MaxPlayers = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    int32 CurrentPlayers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    int32 MinimumCompletedMatches = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString JoinState = TEXT("open");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString ConnectionMode = TEXT("dedicated");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    bool bOfficial = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    bool bPassworded = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString EndpointToken;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString Region;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Lobby")
    FString ServerName;
};

USTRUCT(BlueprintType)
struct FAbyssLobbyJoinDecision
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Lobby")
    bool bAccepted = false;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Lobby")
    bool bTravelAllowed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Lobby")
    EAbyssLobbyRejectReason RejectReason = EAbyssLobbyRejectReason::None;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Lobby")
    FString RejectReasonCode = TEXT("None");

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Lobby")
    FString Message;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Lobby")
    bool bEndpointAvailable = false;
};

UCLASS()
class ABYSSLOCK_API UAbyssLobbySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Abyss|Lobby")
    FAbyssLobbyJoinDecision EvaluateJoinMetadata(
        const FAbyssLobbyMetadata& Metadata,
        const FString& ExpectedBuildId,
        const FString& ExpectedMapId) const;

    UFUNCTION(BlueprintPure, Category = "Abyss|Lobby")
    bool IsSteamLobbyRuntimeAvailable() const;

    UFUNCTION(BlueprintPure, Category = "Abyss|Lobby")
    static TMap<FString, FString> ToKeyValueMetadata(const FAbyssLobbyMetadata& Metadata);

    UFUNCTION(BlueprintPure, Category = "Abyss|Lobby")
    static FAbyssLobbyMetadata FromKeyValueMetadata(const TMap<FString, FString>& Values);

    UFUNCTION(BlueprintPure, Category = "Abyss|Lobby")
    static FString RejectReasonToString(EAbyssLobbyRejectReason Reason);
};
