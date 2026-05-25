#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AbyssTelemetrySubsystem.generated.h"

UCLASS()
class ABYSSLOCK_API UAbyssTelemetrySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Telemetry")
    void LogEvent(const FString& EventName, const FString& Payload);

protected:
    FString EventLogPath;
    FString SessionId;
    FString RunId;
    FString BuildId;
    FString MapId;
    FString Profile;
    FDateTime SessionStartedUtc;
    double SessionStartedSeconds = 0.0;
    int32 EventSequence = 0;
};
