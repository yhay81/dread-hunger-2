#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrostwakeLobbySubsystem.generated.h"

UENUM(BlueprintType)
enum class EFrostwakeLobbyRejectReason : uint8
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
struct FFrostwakeLobbyMetadata
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    int32 SchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString LobbyName = TEXT("Weekend Casual Lobby");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString LobbyType = TEXT("casual");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString BuildId = TEXT("Frostwake-Win64-Development-local");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString MapId = TEXT("L_IcebreakerWhitebox");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString Ruleset = TEXT("standard");

    // Match mode advertised to joiners ("standard" / "madman"). Mirrors EFrostwakeMatchMode; rendezvous-only, informational.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString Mode = TEXT("standard");

    // Difficulty preset advertised to joiners ("easy" / "normal" / "hard"). Mirrors EFrostwakeDifficulty.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString Difficulty = TEXT("normal");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    int32 MaxPlayers = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    int32 CurrentPlayers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    int32 MinimumCompletedMatches = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString JoinState = TEXT("open");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString ConnectionMode = TEXT("dedicated");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    bool bOfficial = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    bool bPassworded = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString EndpointToken;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString Region;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Lobby")
    FString ServerName;
};

USTRUCT(BlueprintType)
struct FFrostwakeLobbyJoinDecision
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Lobby")
    bool bAccepted = false;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Lobby")
    bool bTravelAllowed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Lobby")
    EFrostwakeLobbyRejectReason RejectReason = EFrostwakeLobbyRejectReason::None;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Lobby")
    FString RejectReasonCode = TEXT("None");

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Lobby")
    FString Message;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Lobby")
    bool bEndpointAvailable = false;
};

UCLASS()
class FROSTWAKE_API UFrostwakeLobbySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Frostwake|Lobby")
    FFrostwakeLobbyJoinDecision EvaluateJoinMetadata(
        const FFrostwakeLobbyMetadata& Metadata,
        const FString& ExpectedBuildId,
        const FString& ExpectedMapId) const;

    UFUNCTION(BlueprintPure, Category = "Frostwake|Lobby")
    bool IsSteamLobbyRuntimeAvailable() const;

    UFUNCTION(BlueprintPure, Category = "Frostwake|Lobby")
    static TMap<FString, FString> ToKeyValueMetadata(const FFrostwakeLobbyMetadata& Metadata);

    UFUNCTION(BlueprintPure, Category = "Frostwake|Lobby")
    static FFrostwakeLobbyMetadata FromKeyValueMetadata(const TMap<FString, FString>& Values);

    UFUNCTION(BlueprintPure, Category = "Frostwake|Lobby")
    static FString RejectReasonToString(EFrostwakeLobbyRejectReason Reason);
};
