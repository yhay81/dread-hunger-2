#include "AbyssSteamIdentitySubsystem.h"

#include "AbyssLockLog.h"
#include "AbyssTelemetrySubsystem.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
constexpr const TCHAR* SteamIdentityPath = TEXT("/v1/identity/steam/session-ticket");

bool IsSuccessStatus(int32 HttpStatus)
{
    return HttpStatus >= 200 && HttpStatus < 300;
}

FString TrimTrailingSlash(FString Value)
{
    while (Value.EndsWith(TEXT("/")))
    {
        Value.LeftChopInline(1, EAllowShrinking::No);
    }
    return Value;
}

void ReadSteamStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, FString& OutValue)
{
    FString ParsedValue;
    if (Object.IsValid() && Object->TryGetStringField(FieldName, ParsedValue))
    {
        OutValue = ParsedValue;
    }
}

void ReadSteamBoolField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, bool& OutValue)
{
    bool ParsedValue = false;
    if (Object.IsValid() && Object->TryGetBoolField(FieldName, ParsedValue))
    {
        OutValue = ParsedValue;
    }
}

FAbyssSteamIdentityVerificationResult MakeLocalError(const TCHAR* ErrorCode, const TCHAR* Message)
{
    FAbyssSteamIdentityVerificationResult Result;
    Result.ErrorCode = ErrorCode;
    Result.Message = Message;
    return Result;
}
} // namespace

bool UAbyssSteamIdentitySubsystem::SubmitSessionTicketToBackend(
    const FString& BackendBaseUrl,
    const FAbyssSteamIdentitySessionTicketRequest& Request,
    FAbyssSteamIdentityVerificationDelegate Callback)
{
    FString RequestJson;
    if (BackendBaseUrl.TrimStartAndEnd().IsEmpty() || !BuildSessionTicketRequestJson(Request, RequestJson))
    {
        Callback.ExecuteIfBound(MakeLocalError(TEXT("invalid_steam_identity_request"), TEXT("Steam identity request was not sent.")));
        return false;
    }

    const FString EndpointUrl = BuildEndpointUrl(BackendBaseUrl);
    UE_LOG(LogAbyssSecurity, Log, TEXT("%s"), *BuildRedactedRequestLogLine(BackendBaseUrl, Request));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("steam_identity_request_sent"),
                FString::Printf(
                    TEXT("{\"endpoint\":\"%s\",\"purpose\":\"%s\",\"ticketBytes\":%d,\"hasLocalRunId\":%s}"),
                    SteamIdentityPath,
                    *Request.Purpose,
                    Request.TicketHex.Len() / 2,
                    Request.LocalRunId.IsEmpty() ? TEXT("false") : TEXT("true")));
        }
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(EndpointUrl);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetContentAsString(RequestJson);
    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, Callback](FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded)
        {
            FAbyssSteamIdentityVerificationResult Result;
            const int32 HttpStatus = Response.IsValid() ? Response->GetResponseCode() : 0;
            const FString ResponseBody = Response.IsValid() ? Response->GetContentAsString() : FString();
            if (!TryParseSessionTicketResponseJson(ResponseBody, HttpStatus, bSucceeded && Response.IsValid(), Result))
            {
                Result = MakeLocalError(TEXT("invalid_steam_identity_response"), TEXT("Steam identity response could not be parsed."));
                Result.HttpStatus = HttpStatus;
                Result.bTransportSucceeded = bSucceeded && Response.IsValid();
            }

            UE_LOG(
                LogAbyssSecurity,
                Log,
                TEXT("steam_identity_response status=%d verified=%s ownsApp=%s error=%s proofId=%s subjectHash=%s"),
                Result.HttpStatus,
                Result.bVerified ? TEXT("true") : TEXT("false"),
                Result.bOwnsApp ? TEXT("true") : TEXT("false"),
                Result.ErrorCode.IsEmpty() ? TEXT("none") : *Result.ErrorCode,
                Result.ProofId.IsEmpty() ? TEXT("none") : *Result.ProofId,
                Result.SubjectHash.IsEmpty() ? TEXT("none") : *Result.SubjectHash);

            if (UGameInstance* GameInstance = GetGameInstance())
            {
                if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
                {
                    TelemetrySubsystem->LogEvent(
                        TEXT("steam_identity_response"),
                        FString::Printf(
                            TEXT("{\"httpStatus\":%d,\"verified\":%s,\"ownsApp\":%s,\"error\":\"%s\",\"hasProofId\":%s,\"hasSubjectHash\":%s}"),
                            Result.HttpStatus,
                            Result.bVerified ? TEXT("true") : TEXT("false"),
                            Result.bOwnsApp ? TEXT("true") : TEXT("false"),
                            Result.ErrorCode.IsEmpty() ? TEXT("") : *Result.ErrorCode,
                            Result.ProofId.IsEmpty() ? TEXT("false") : TEXT("true"),
                            Result.SubjectHash.IsEmpty() ? TEXT("false") : TEXT("true")));
                }
            }

            Callback.ExecuteIfBound(Result);
        });

    if (!HttpRequest->ProcessRequest())
    {
        Callback.ExecuteIfBound(MakeLocalError(TEXT("steam_identity_request_not_started"), TEXT("HTTP request did not start.")));
        return false;
    }

    return true;
}

bool UAbyssSteamIdentitySubsystem::BuildSessionTicketRequestJson(const FAbyssSteamIdentitySessionTicketRequest& Request, FString& OutJson)
{
    if (!IsValidPurpose(Request.Purpose) || !IsEvenLengthHex(Request.TicketHex))
    {
        return false;
    }

    TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
    RootObject->SetStringField(TEXT("ticketHex"), Request.TicketHex.ToLower());
    RootObject->SetStringField(TEXT("purpose"), Request.Purpose);
    if (!Request.LocalRunId.IsEmpty())
    {
        RootObject->SetStringField(TEXT("localRunId"), Request.LocalRunId);
    }

    const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutJson);
    return FJsonSerializer::Serialize(RootObject, Writer);
}

bool UAbyssSteamIdentitySubsystem::TryParseSessionTicketResponseJson(
    const FString& ResponseJson,
    int32 HttpStatus,
    bool bTransportSucceeded,
    FAbyssSteamIdentityVerificationResult& OutResult)
{
    OutResult = FAbyssSteamIdentityVerificationResult();
    OutResult.HttpStatus = HttpStatus;
    OutResult.bTransportSucceeded = bTransportSucceeded;
    if (!bTransportSucceeded)
    {
        OutResult.ErrorCode = TEXT("steam_identity_transport_failed");
        return true;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseJson);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutResult.ErrorCode = TEXT("invalid_steam_identity_response");
        return false;
    }

    if (!IsSuccessStatus(HttpStatus))
    {
        ReadSteamStringField(RootObject, TEXT("error"), OutResult.ErrorCode);
        ReadSteamStringField(RootObject, TEXT("message"), OutResult.Message);
        if (OutResult.ErrorCode.IsEmpty())
        {
            OutResult.ErrorCode = TEXT("steam_identity_http_error");
        }
        return true;
    }

    ReadSteamBoolField(RootObject, TEXT("verified"), OutResult.bVerified);
    ReadSteamBoolField(RootObject, TEXT("ownsApp"), OutResult.bOwnsApp);
    ReadSteamStringField(RootObject, TEXT("subjectHash"), OutResult.SubjectHash);
    ReadSteamStringField(RootObject, TEXT("proofId"), OutResult.ProofId);
    ReadSteamStringField(RootObject, TEXT("expiresAt"), OutResult.ExpiresAt);
    if (!OutResult.bVerified || !OutResult.bOwnsApp || OutResult.SubjectHash.IsEmpty() || OutResult.ProofId.IsEmpty())
    {
        OutResult.ErrorCode = TEXT("incomplete_steam_identity_proof");
    }
    return true;
}

FString UAbyssSteamIdentitySubsystem::BuildRedactedRequestLogLine(
    const FString& BackendBaseUrl,
    const FAbyssSteamIdentitySessionTicketRequest& Request)
{
    return FString::Printf(
        TEXT("steam_identity_request endpoint=%s purpose=%s ticketHex=[REDACTED] ticketBytes=%d hasLocalRunId=%s backendConfigured=%s"),
        SteamIdentityPath,
        *Request.Purpose,
        Request.TicketHex.Len() / 2,
        Request.LocalRunId.IsEmpty() ? TEXT("false") : TEXT("true"),
        BackendBaseUrl.TrimStartAndEnd().IsEmpty() ? TEXT("false") : TEXT("true"));
}

bool UAbyssSteamIdentitySubsystem::IsValidPurpose(const FString& Purpose)
{
    return Purpose == TEXT("moderation") || Purpose == TEXT("server_metadata") || Purpose == TEXT("support");
}

bool UAbyssSteamIdentitySubsystem::IsEvenLengthHex(const FString& Value)
{
    if (Value.Len() < 2 || Value.Len() % 2 != 0)
    {
        return false;
    }

    for (const TCHAR Character : Value)
    {
        if (!FChar::IsHexDigit(Character))
        {
            return false;
        }
    }
    return true;
}

FString UAbyssSteamIdentitySubsystem::BuildEndpointUrl(const FString& BackendBaseUrl)
{
    return TrimTrailingSlash(BackendBaseUrl.TrimStartAndEnd()) + SteamIdentityPath;
}
