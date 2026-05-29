#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrostwakeSteamIdentitySubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFrostwakeSteamIdentitySessionTicketRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Identity")
    FString TicketHex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Identity")
    FString Purpose = TEXT("moderation");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Identity")
    FString LocalRunId;
};

USTRUCT(BlueprintType)
struct FFrostwakeSteamIdentityVerificationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    bool bTransportSucceeded = false;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    int32 HttpStatus = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    bool bVerified = false;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    bool bOwnsApp = false;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    FString SubjectHash;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    FString ProofId;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    FString ExpiresAt;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    FString ErrorCode;

    UPROPERTY(BlueprintReadOnly, Category = "Frostwake|Identity")
    FString Message;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FFrostwakeSteamIdentityVerificationDelegate, const FFrostwakeSteamIdentityVerificationResult&, Result);

UCLASS()
class FROSTWAKE_API UFrostwakeSteamIdentitySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Frostwake|Identity")
    bool SubmitSessionTicketToBackend(
        const FString& BackendBaseUrl,
        const FFrostwakeSteamIdentitySessionTicketRequest& Request,
        FFrostwakeSteamIdentityVerificationDelegate Callback);

    UFUNCTION(BlueprintPure, Category = "Frostwake|Identity")
    static bool BuildSessionTicketRequestJson(const FFrostwakeSteamIdentitySessionTicketRequest& Request, FString& OutJson);

    UFUNCTION(BlueprintPure, Category = "Frostwake|Identity")
    static bool TryParseSessionTicketResponseJson(
        const FString& ResponseJson,
        int32 HttpStatus,
        bool bTransportSucceeded,
        FFrostwakeSteamIdentityVerificationResult& OutResult);

    UFUNCTION(BlueprintPure, Category = "Frostwake|Identity")
    static FString BuildRedactedRequestLogLine(
        const FString& BackendBaseUrl,
        const FFrostwakeSteamIdentitySessionTicketRequest& Request);

private:
    static bool IsValidPurpose(const FString& Purpose);
    static bool IsEvenLengthHex(const FString& Value);
    static FString BuildEndpointUrl(const FString& BackendBaseUrl);
};
