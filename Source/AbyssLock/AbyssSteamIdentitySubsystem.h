#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AbyssSteamIdentitySubsystem.generated.h"

USTRUCT(BlueprintType)
struct FAbyssSteamIdentitySessionTicketRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Identity")
    FString TicketHex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Identity")
    FString Purpose = TEXT("moderation");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss|Identity")
    FString LocalRunId;
};

USTRUCT(BlueprintType)
struct FAbyssSteamIdentityVerificationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    bool bTransportSucceeded = false;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    int32 HttpStatus = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    bool bVerified = false;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    bool bOwnsApp = false;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    FString SubjectHash;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    FString ProofId;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    FString ExpiresAt;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    FString ErrorCode;

    UPROPERTY(BlueprintReadOnly, Category = "Abyss|Identity")
    FString Message;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FAbyssSteamIdentityVerificationDelegate, const FAbyssSteamIdentityVerificationResult&, Result);

UCLASS()
class ABYSSLOCK_API UAbyssSteamIdentitySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Abyss|Identity")
    bool SubmitSessionTicketToBackend(
        const FString& BackendBaseUrl,
        const FAbyssSteamIdentitySessionTicketRequest& Request,
        FAbyssSteamIdentityVerificationDelegate Callback);

    UFUNCTION(BlueprintPure, Category = "Abyss|Identity")
    static bool BuildSessionTicketRequestJson(const FAbyssSteamIdentitySessionTicketRequest& Request, FString& OutJson);

    UFUNCTION(BlueprintPure, Category = "Abyss|Identity")
    static bool TryParseSessionTicketResponseJson(
        const FString& ResponseJson,
        int32 HttpStatus,
        bool bTransportSucceeded,
        FAbyssSteamIdentityVerificationResult& OutResult);

    UFUNCTION(BlueprintPure, Category = "Abyss|Identity")
    static FString BuildRedactedRequestLogLine(
        const FString& BackendBaseUrl,
        const FAbyssSteamIdentitySessionTicketRequest& Request);

private:
    static bool IsValidPurpose(const FString& Purpose);
    static bool IsEvenLengthHex(const FString& Value);
    static FString BuildEndpointUrl(const FString& BackendBaseUrl);
};
