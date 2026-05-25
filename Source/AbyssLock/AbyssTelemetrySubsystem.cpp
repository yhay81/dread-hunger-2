#include "AbyssTelemetrySubsystem.h"
#include "AbyssLockLog.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
FString AbyssEscapeJsonString(const FString& Value)
{
    FString Result;
    Result.Reserve(Value.Len());

    for (int32 Index = 0; Index < Value.Len(); ++Index)
    {
        const TCHAR Character = Value[Index];
        switch (Character)
        {
            case TCHAR('"'):
                Result += TEXT("\\\"");
                break;
            case TCHAR('\\'):
                Result += TEXT("\\\\");
                break;
            case TCHAR('\b'):
                Result += TEXT("\\b");
                break;
            case TCHAR('\f'):
                Result += TEXT("\\f");
                break;
            case TCHAR('\n'):
                Result += TEXT("\\n");
                break;
            case TCHAR('\r'):
                Result += TEXT("\\r");
                break;
            case TCHAR('\t'):
                Result += TEXT("\\t");
                break;
            default:
                if (Character < TCHAR(0x20))
                {
                    Result += FString::Printf(TEXT("\\u%04x"), static_cast<uint32>(Character));
                }
                else
                {
                    Result.AppendChar(Character);
                }
                break;
        }
    }

    return Result;
}

FString ReadCommandLineValue(const TCHAR* Key, const FString& DefaultValue)
{
    FString Value;
    if (FParse::Value(FCommandLine::Get(), Key, Value) && !Value.IsEmpty())
    {
        return Value;
    }
    return DefaultValue;
}
}

void UAbyssTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SessionStartedUtc = FDateTime::UtcNow();
    SessionStartedSeconds = FPlatformTime::Seconds();
    EventSequence = 0;
    SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);

    FString OverrideEventLogPath;
    if (FParse::Value(FCommandLine::Get(), TEXT("AbyssEventLog="), OverrideEventLogPath) && !OverrideEventLogPath.IsEmpty())
    {
        EventLogPath = OverrideEventLogPath;
    }
    else
    {
        EventLogPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Logs"), TEXT("server_events.jsonl"));
    }

    RunId = ReadCommandLineValue(TEXT("AbyssRunId="), SessionStartedUtc.ToString(TEXT("%Y%m%dT%H%M%SZ")));
    BuildId = ReadCommandLineValue(TEXT("AbyssBuildId="), TEXT("local-dev"));
    MapId = ReadCommandLineValue(TEXT("AbyssMapId="), TEXT("unknown"));
    Profile = ReadCommandLineValue(TEXT("AbyssProfile="), TEXT("custom"));

    LogEvent(
        TEXT("session_started"),
        FString::Printf(
            TEXT("{\"sessionId\":\"%s\",\"runId\":\"%s\",\"buildId\":\"%s\",\"mapId\":\"%s\",\"profile\":\"%s\",\"eventLog\":\"%s\"}"),
            *AbyssEscapeJsonString(SessionId),
            *AbyssEscapeJsonString(RunId),
            *AbyssEscapeJsonString(BuildId),
            *AbyssEscapeJsonString(MapId),
            *AbyssEscapeJsonString(Profile),
            *AbyssEscapeJsonString(EventLogPath)));
}

void UAbyssTelemetrySubsystem::LogEvent(const FString& EventName, const FString& Payload)
{
    UE_LOG(LogAbyssGameplay, Log, TEXT("telemetry_event name=%s payload=%s"), *EventName, *Payload);

    TSharedRef<FJsonObject> EventObject = MakeShared<FJsonObject>();
    EventObject->SetStringField(TEXT("timestamp_utc"), FDateTime::UtcNow().ToIso8601());
    EventObject->SetNumberField(TEXT("sequence"), ++EventSequence);
    EventObject->SetNumberField(TEXT("elapsed_seconds"), FMath::Max(0.0, FPlatformTime::Seconds() - SessionStartedSeconds));
    EventObject->SetStringField(TEXT("session_id"), SessionId);
    EventObject->SetStringField(TEXT("run_id"), RunId);
    EventObject->SetStringField(TEXT("build_id"), BuildId);
    EventObject->SetStringField(TEXT("map_id"), MapId);
    EventObject->SetStringField(TEXT("profile"), Profile);
    EventObject->SetStringField(TEXT("event"), EventName);

    TSharedPtr<FJsonObject> PayloadObject;
    const TSharedRef<TJsonReader<>> PayloadReader = TJsonReaderFactory<>::Create(Payload);
    if (FJsonSerializer::Deserialize(PayloadReader, PayloadObject) && PayloadObject.IsValid())
    {
        EventObject->SetObjectField(TEXT("payload"), PayloadObject);
    }
    else
    {
        EventObject->SetStringField(TEXT("payload"), Payload);
    }

    FString SerializedEvent;
    const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&SerializedEvent);
    if (!FJsonSerializer::Serialize(EventObject, Writer))
    {
        UE_LOG(LogAbyssGameplay, Warning, TEXT("Failed to serialize telemetry event name=%s"), *EventName);
        return;
    }

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(EventLogPath), true);
    FFileHelper::SaveStringToFile(
        SerializedEvent + LINE_TERMINATOR,
        *EventLogPath,
        FFileHelper::EEncodingOptions::AutoDetect,
        &IFileManager::Get(),
        FILEWRITE_Append);
}
