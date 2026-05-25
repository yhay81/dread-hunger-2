#include "AbyssLockGameMode.h"
#include "AbyssDoorActor.h"
#include "AbyssInventoryComponent.h"
#include "AbyssItemPickupActor.h"
#include "AbyssLockCharacter.h"
#include "AbyssLockGameState.h"
#include "AbyssLockLog.h"
#include "AbyssLockPlayerController.h"
#include "AbyssLockPlayerState.h"
#include "AbyssPveEnemyActor.h"
#include "AbyssServerConfigSubsystem.h"
#include "AbyssShipTaskActor.h"
#include "AbyssTelemetrySubsystem.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

AAbyssLockGameMode::AAbyssLockGameMode()
{
    bUseSeamlessTravel = true;
    DefaultPawnClass = AAbyssLockCharacter::StaticClass();
    PlayerControllerClass = AAbyssLockPlayerController::StaticClass();
    GameStateClass = AAbyssLockGameState::StaticClass();
    PlayerStateClass = AAbyssLockPlayerState::StaticClass();
}

void AAbyssLockGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (const UAbyssServerConfigSubsystem* ServerConfigSubsystem = GameInstance->GetSubsystem<UAbyssServerConfigSubsystem>())
        {
            const FAbyssServerConfig ServerConfig = ServerConfigSubsystem->GetServerConfig();
            UE_LOG(
                LogAbyssSession,
                Log,
                TEXT("Match GameMode started server=%s map=%s ruleset=%s maxPlayers=%d loadedFromFile=%s"),
                *ServerConfig.ServerName,
                *ServerConfig.Map,
                *ServerConfig.Ruleset,
                ServerConfig.MaxPlayers,
                ServerConfigSubsystem->WasLoadedFromFile() ? TEXT("true") : TEXT("false"));
        }
    }

    TryAutoStartMatchForDev();
}

void AAbyssLockGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    const int32 ConnectedPlayers = GameState ? GameState->PlayerArray.Num() : 0;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("client_connected"),
                FString::Printf(
                    TEXT("{\"player_id\":\"%s\",\"controller\":\"%s\",\"connected_players\":%d,\"source\":\"post_login\"}"),
                    *GetNameSafe(NewPlayer ? NewPlayer->PlayerState : nullptr),
                    *GetNameSafe(NewPlayer),
                    ConnectedPlayers));
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssAutoReady")))
    {
        if (AAbyssLockPlayerState* AbyssPlayerState = NewPlayer ? NewPlayer->GetPlayerState<AAbyssLockPlayerState>() : nullptr)
        {
            AbyssPlayerState->SetReadyForMatch(true);
            UE_LOG(LogAbyssGameplay, Log, TEXT("player_ready_changed player=%s ready=true source=dev_auto_ready"), *GetNameSafe(AbyssPlayerState));
            if (UGameInstance* GameInstance = GetGameInstance())
            {
                if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
                {
                    TelemetrySubsystem->LogEvent(
                        TEXT("player_ready_changed"),
                        FString::Printf(TEXT("{\"player\":\"%s\",\"ready\":true,\"source\":\"dev_auto_ready\"}"), *GetNameSafe(AbyssPlayerState)));
                }
            }
        }
        TryStartMatchFromReady();
    }
#endif
    TryAutoStartMatchForDev();
}

void AAbyssLockGameMode::Logout(AController* Exiting)
{
    const int32 ConnectedPlayersBefore = GameState ? GameState->PlayerArray.Num() : 0;
    const int32 ConnectedPlayersAfter = FMath::Max(0, ConnectedPlayersBefore - 1);
    const FString PlayerId = GetNameSafe(Exiting ? Exiting->PlayerState : nullptr);
    const FString ControllerName = GetNameSafe(Exiting);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("client_disconnected"),
                FString::Printf(
                    TEXT("{\"player_id\":\"%s\",\"controller\":\"%s\",\"connected_players_before\":%d,\"connected_players_after\":%d,\"source\":\"logout\"}"),
                    *PlayerId,
                    *ControllerName,
                    ConnectedPlayersBefore,
                    ConnectedPlayersAfter));
        }
    }

    Super::Logout(Exiting);
}

void AAbyssLockGameMode::AssignRolesForCurrentPlayers()
{
    AssignRolesForCurrentPlayersInternal(INDEX_NONE);
}

void AAbyssLockGameMode::AssignRolesForCurrentPlayersInternal(int32 SaboteurCountOverride)
{
    if (!HasAuthority() || !GameState)
    {
        return;
    }

    TArray<AAbyssLockPlayerState*> ConnectedPlayers;
    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        if (AAbyssLockPlayerState* AbyssPlayerState = Cast<AAbyssLockPlayerState>(PlayerState))
        {
            ConnectedPlayers.Add(AbyssPlayerState);
        }
    }

    const int32 PlayerCount = ConnectedPlayers.Num();
    const int32 DesiredSaboteurCount = SaboteurCountOverride >= 0 ? SaboteurCountOverride : CalculateSaboteurCount(PlayerCount);
    const int32 SaboteurCount = FMath::Clamp(DesiredSaboteurCount, 0, PlayerCount);
    if (PlayerCount <= 0)
    {
        UE_LOG(LogAbyssGameplay, Warning, TEXT("role_assignment_skipped players=%d"), PlayerCount);
        return;
    }

    for (int32 Index = 0; Index < ConnectedPlayers.Num(); ++Index)
    {
        const int32 SwapIndex = FMath::RandRange(Index, ConnectedPlayers.Num() - 1);
        ConnectedPlayers.Swap(Index, SwapIndex);
    }

    for (int32 Index = 0; Index < ConnectedPlayers.Num(); ++Index)
    {
        AAbyssLockPlayerState* PlayerState = ConnectedPlayers[Index];
        const bool bIsSaboteur = Index < SaboteurCount;
        PlayerState->SetSecretTeam(bIsSaboteur ? EAbyssTeam::Saboteur : EAbyssTeam::Crew);
        PlayerState->SetRevealedTeam(EAbyssTeam::Unassigned);
        PlayerState->SetLifeState(EAbyssLifeState::Alive);
    }

    UE_LOG(LogAbyssGameplay, Log, TEXT("role_assignment_complete players=%d saboteurs=%d"), PlayerCount, SaboteurCount);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("role_assignment_complete"),
                FString::Printf(TEXT("{\"players\":%d,\"saboteurs\":%d}"), PlayerCount, SaboteurCount));
        }
    }
}

void AAbyssLockGameMode::TryAutoStartMatchForDev()
{
#if !UE_BUILD_SHIPPING
    if (bDevAutoStarted || !HasAuthority() || !FParse::Param(FCommandLine::Get(), TEXT("AbyssAutoStart")))
    {
        return;
    }

    int32 MinPlayers = 1;
    FParse::Value(FCommandLine::Get(), TEXT("AbyssAutoStartMinPlayers="), MinPlayers);
    const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 0;
    if (PlayerCount < FMath::Max(1, MinPlayers))
    {
        return;
    }

    int32 SaboteurOverride = 0;
    FParse::Value(FCommandLine::Get(), TEXT("AbyssDevSaboteurs="), SaboteurOverride);
    AssignRolesForCurrentPlayersInternal(SaboteurOverride);

    if (AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>())
    {
        AbyssGameState->SetMatchPhase(EAbyssMatchPhase::InProgress);
    }

    bDevAutoStarted = true;
    UE_LOG(LogAbyssGameplay, Log, TEXT("dev_auto_start players=%d saboteurs=%d"), PlayerCount, FMath::Clamp(SaboteurOverride, 0, PlayerCount));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("match_started"),
                FString::Printf(TEXT("{\"source\":\"dev_auto_start\",\"players\":%d,\"saboteurs\":%d}"), PlayerCount, FMath::Clamp(SaboteurOverride, 0, PlayerCount)));
            TelemetrySubsystem->LogEvent(
                TEXT("dev_auto_start"),
                FString::Printf(TEXT("{\"players\":%d,\"saboteurs\":%d}"), PlayerCount, FMath::Clamp(SaboteurOverride, 0, PlayerCount)));
        }
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeInteract")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeTaskInteraction);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeDownRescue")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeDownRescue);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeContainment")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeContainment);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeRouteComplete")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeRouteComplete);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeFatalSabotage")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeFatalSabotage);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeBulkheadLock")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeBulkheadLock);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokePumpFlooding")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokePumpFlooding);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokePveEnemy")) || FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokePvEDamage")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokePveEnemy);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeItemDrop")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeItemDrop);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeCombinedSystems")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeCombinedSystems);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeQaBot")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeQaBot);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeQaPlayerBot")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeQaPlayerBot);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeQaTaskBot")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeQaTaskBot);
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokeTaskInteraction()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    APawn* SaboteurPawn = FindPawnForTeam(EAbyssTeam::Saboteur);
    APawn* AnyAssignedPawn = FindPawnForTeam(EAbyssTeam::Crew);
    if (!AnyAssignedPawn)
    {
        AnyAssignedPawn = SaboteurPawn;
    }

    bool bAppliedSabotage = false;
    bool bAppliedRepair = false;

    if (SaboteurPawn)
    {
        for (TActorIterator<AAbyssShipTaskActor> It(GetWorld()); It; ++It)
        {
            AAbyssShipTaskActor* TaskActor = *It;
            if (TaskActor && TaskActor->GetTaskMode() == EAbyssShipTaskMode::Sabotage)
            {
                bAppliedSabotage = TaskActor->Interact(SaboteurPawn);
                break;
            }
        }
    }

    if (AnyAssignedPawn)
    {
        for (TActorIterator<AAbyssShipTaskActor> It(GetWorld()); It; ++It)
        {
            AAbyssShipTaskActor* TaskActor = *It;
            if (TaskActor && TaskActor->GetTaskMode() == EAbyssShipTaskMode::Repair)
            {
                bAppliedRepair = TaskActor->Interact(AnyAssignedPawn);
                break;
            }
        }
    }

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_task_interaction sabotage=%s repair=%s saboteurPawn=%s assignedPawn=%s"),
        bAppliedSabotage ? TEXT("true") : TEXT("false"),
        bAppliedRepair ? TEXT("true") : TEXT("false"),
        *GetNameSafe(SaboteurPawn),
        *GetNameSafe(AnyAssignedPawn));
#endif
}

void AAbyssLockGameMode::RunDevSmokeDownRescue()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockCharacter* TargetCharacter = nullptr;
    APawn* RescuerPawn = nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (!PlayerController)
        {
            continue;
        }

        AAbyssLockCharacter* Candidate = Cast<AAbyssLockCharacter>(PlayerController->GetPawn());
        if (!Candidate)
        {
            continue;
        }

        if (!TargetCharacter)
        {
            TargetCharacter = Candidate;
        }
        else if (!RescuerPawn)
        {
            RescuerPawn = Candidate;
            break;
        }
    }

    if (!RescuerPawn)
    {
        RescuerPawn = TargetCharacter;
    }

    const bool bDowned = TargetCharacter ? TargetCharacter->ApplyServerDamage(999.0f, RescuerPawn) : false;
    const bool bRescued = TargetCharacter ? TargetCharacter->RescueFromDowned(RescuerPawn) : false;
    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_down_rescue downed=%s rescued=%s target=%s rescuer=%s"),
        bDowned ? TEXT("true") : TEXT("false"),
        bRescued ? TEXT("true") : TEXT("false"),
        *GetNameSafe(TargetCharacter),
        *GetNameSafe(RescuerPawn));
#endif
}

void AAbyssLockGameMode::RunDevSmokeContainment()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockCharacter* TargetCharacter = nullptr;
    APawn* InstigatorPawn = nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (!PlayerController)
        {
            continue;
        }

        AAbyssLockCharacter* Candidate = Cast<AAbyssLockCharacter>(PlayerController->GetPawn());
        if (!Candidate)
        {
            continue;
        }

        if (!TargetCharacter)
        {
            TargetCharacter = Candidate;
        }
        else if (!InstigatorPawn)
        {
            InstigatorPawn = Candidate;
            break;
        }
    }

    if (!InstigatorPawn)
    {
        InstigatorPawn = TargetCharacter;
    }

    const bool bContained = TargetCharacter ? TargetCharacter->ContainPlayer(InstigatorPawn) : false;
    const bool bReleased = TargetCharacter ? TargetCharacter->ReleaseFromContainment(InstigatorPawn) : false;
    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_containment contained=%s released=%s target=%s instigator=%s"),
        bContained ? TEXT("true") : TEXT("false"),
        bReleased ? TEXT("true") : TEXT("false"),
        *GetNameSafe(TargetCharacter),
        *GetNameSafe(InstigatorPawn));
#endif
}

void AAbyssLockGameMode::RunDevSmokeRouteComplete()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    APawn* InstigatorPawn = FindPawnForTeam(EAbyssTeam::Crew);
    if (!InstigatorPawn)
    {
        InstigatorPawn = FindPawnForTeam(EAbyssTeam::Saboteur);
    }

    AAbyssShipTaskActor* RouteTask = nullptr;
    for (TActorIterator<AAbyssShipTaskActor> It(GetWorld()); It; ++It)
    {
        AAbyssShipTaskActor* TaskActor = *It;
        if (TaskActor && TaskActor->GetTaskMode() == EAbyssShipTaskMode::Repair && TaskActor->GetTargetSystem() == EAbyssShipSystem::Route)
        {
            RouteTask = TaskActor;
            break;
        }
    }

    int32 AppliedCount = 0;
    if (RouteTask && InstigatorPawn)
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (RouteTask->Interact(InstigatorPawn))
            {
                ++AppliedCount;
            }
        }
    }

    const AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_route_complete applied=%d progress=%.2f phase=%d task=%s instigator=%s"),
        AppliedCount,
        AbyssGameState ? AbyssGameState->GetRouteProgress() : 0.0f,
        AbyssGameState ? static_cast<int32>(AbyssGameState->GetMatchPhase()) : -1,
        *GetNameSafe(RouteTask),
        *GetNameSafe(InstigatorPawn));
#endif
}

void AAbyssLockGameMode::RunDevSmokeFatalSabotage()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    APawn* SaboteurPawn = FindPawnForTeam(EAbyssTeam::Saboteur);
    AAbyssShipTaskActor* SabotageTask = nullptr;
    for (TActorIterator<AAbyssShipTaskActor> It(GetWorld()); It; ++It)
    {
        AAbyssShipTaskActor* TaskActor = *It;
        if (TaskActor && TaskActor->GetTaskMode() == EAbyssShipTaskMode::Sabotage)
        {
            SabotageTask = TaskActor;
            break;
        }
    }

    int32 AppliedCount = 0;
    if (SabotageTask && SaboteurPawn)
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (SabotageTask->Interact(SaboteurPawn))
            {
                ++AppliedCount;
            }
        }
    }

    const AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_fatal_sabotage applied=%d phase=%d winner=%d task=%s saboteur=%s"),
        AppliedCount,
        AbyssGameState ? static_cast<int32>(AbyssGameState->GetMatchPhase()) : -1,
        AbyssGameState ? static_cast<int32>(AbyssGameState->GetWinningTeam()) : -1,
        *GetNameSafe(SabotageTask),
        *GetNameSafe(SaboteurPawn));
#endif
}

void AAbyssLockGameMode::RunDevSmokeBulkheadLock()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssDoorActor* DoorActor = nullptr;
    for (TActorIterator<AAbyssDoorActor> It(GetWorld()); It; ++It)
    {
        DoorActor = *It;
        if (DoorActor)
        {
            break;
        }
    }

    APawn* SaboteurPawn = FindPawnForTeam(EAbyssTeam::Saboteur);
    APawn* CrewPawn = FindPawnForTeam(EAbyssTeam::Crew);
    if (!CrewPawn)
    {
        CrewPawn = SaboteurPawn;
    }

    const bool bOpenedBeforeLock = DoorActor && CrewPawn ? DoorActor->Interact(CrewPawn) : false;
    const bool bLocked = DoorActor && SaboteurPawn ? DoorActor->LockForSabotage(SaboteurPawn) : false;
    const bool bLockedInteractionSucceeded = DoorActor && CrewPawn ? DoorActor->Interact(CrewPawn) : false;
    const bool bBlockedWhileLocked = bLocked && !bLockedInteractionSucceeded;
    const bool bUnlocked = DoorActor && CrewPawn ? DoorActor->UnlockFromRepair(CrewPawn) : false;
    const bool bOpenedAfterUnlock = DoorActor && CrewPawn ? DoorActor->Interact(CrewPawn) : false;
    const bool bSmokePassed = bOpenedBeforeLock && bLocked && bBlockedWhileLocked && bUnlocked && bOpenedAfterUnlock;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_bulkhead_lock result=%s openedBeforeLock=%s locked=%s blockedWhileLocked=%s unlocked=%s openedAfterUnlock=%s door=%s saboteur=%s crew=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        bOpenedBeforeLock ? TEXT("true") : TEXT("false"),
        bLocked ? TEXT("true") : TEXT("false"),
        bBlockedWhileLocked ? TEXT("true") : TEXT("false"),
        bUnlocked ? TEXT("true") : TEXT("false"),
        bOpenedAfterUnlock ? TEXT("true") : TEXT("false"),
        *GetNameSafe(DoorActor),
        *GetNameSafe(SaboteurPawn),
        *GetNameSafe(CrewPawn));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_bulkhead_lock"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"openedBeforeLock\":%s,\"locked\":%s,\"blockedWhileLocked\":%s,\"unlocked\":%s,\"openedAfterUnlock\":%s,\"door\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    bOpenedBeforeLock ? TEXT("true") : TEXT("false"),
                    bLocked ? TEXT("true") : TEXT("false"),
                    bBlockedWhileLocked ? TEXT("true") : TEXT("false"),
                    bUnlocked ? TEXT("true") : TEXT("false"),
                    bOpenedAfterUnlock ? TEXT("true") : TEXT("false"),
                    *GetNameSafe(DoorActor)));
        }
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokePumpFlooding()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssShipTaskActor* FloodingSabotageTask = nullptr;
    AAbyssShipTaskActor* FloodingRepairTask = nullptr;
    for (TActorIterator<AAbyssShipTaskActor> It(GetWorld()); It; ++It)
    {
        AAbyssShipTaskActor* TaskActor = *It;
        if (!TaskActor || TaskActor->GetTargetSystem() != EAbyssShipSystem::Flooding)
        {
            continue;
        }

        if (TaskActor->GetTaskMode() == EAbyssShipTaskMode::Sabotage)
        {
            FloodingSabotageTask = TaskActor;
        }
        else if (TaskActor->GetTaskMode() == EAbyssShipTaskMode::Repair)
        {
            FloodingRepairTask = TaskActor;
        }
    }

    APawn* SaboteurPawn = FindPawnForTeam(EAbyssTeam::Saboteur);
    APawn* CrewPawn = FindPawnForTeam(EAbyssTeam::Crew);
    if (!CrewPawn)
    {
        CrewPawn = SaboteurPawn;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    const float PressureBefore = AbyssGameState ? AbyssGameState->GetFloodingPressure() : 0.0f;
    int32 SabotageCount = 0;
    if (FloodingSabotageTask && SaboteurPawn)
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (FloodingSabotageTask->Interact(SaboteurPawn))
            {
                ++SabotageCount;
            }
        }
    }
    const float PressureAfterSabotage = AbyssGameState ? AbyssGameState->GetFloodingPressure() : 0.0f;
    const bool bRepaired = FloodingRepairTask && CrewPawn ? FloodingRepairTask->Interact(CrewPawn) : false;
    const float PressureAfterRepair = AbyssGameState ? AbyssGameState->GetFloodingPressure() : 0.0f;
    const bool bSmokePassed = SabotageCount == 2 && bRepaired && PressureAfterSabotage > PressureBefore && PressureAfterRepair < PressureAfterSabotage && PressureAfterRepair > PressureBefore;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_pump_flooding result=%s sabotageCount=%d repaired=%s pressureBefore=%.2f pressureAfterSabotage=%.2f pressureAfterRepair=%.2f sabotageTask=%s repairTask=%s saboteur=%s crew=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        SabotageCount,
        bRepaired ? TEXT("true") : TEXT("false"),
        PressureBefore,
        PressureAfterSabotage,
        PressureAfterRepair,
        *GetNameSafe(FloodingSabotageTask),
        *GetNameSafe(FloodingRepairTask),
        *GetNameSafe(SaboteurPawn),
        *GetNameSafe(CrewPawn));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_pump_flooding"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"sabotageCount\":%d,\"repaired\":%s,\"pressureBefore\":%.3f,\"pressureAfterSabotage\":%.3f,\"pressureAfterRepair\":%.3f}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    SabotageCount,
                    bRepaired ? TEXT("true") : TEXT("false"),
                    PressureBefore,
                    PressureAfterSabotage,
                    PressureAfterRepair));
        }
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokePveEnemy()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockCharacter* TargetCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Crew));
    if (!TargetCharacter)
    {
        TargetCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Saboteur));
    }

    AAbyssPveEnemyActor* EnemyActor = nullptr;
    if (TargetCharacter)
    {
        const FVector SpawnLocation = TargetCharacter->GetActorLocation() + FVector(180.0f, 0.0f, 0.0f);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        EnemyActor = GetWorld()->SpawnActor<AAbyssPveEnemyActor>(AAbyssPveEnemyActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
        if (EnemyActor)
        {
            EnemyActor->ConfigureEnemy(999.0f);
            UE_LOG(LogAbyssGameplay, Log, TEXT("pve_enemy_spawned enemy=%s target=%s"), *GetNameSafe(EnemyActor), *GetNameSafe(TargetCharacter));
            if (UGameInstance* GameInstance = GetGameInstance())
            {
                if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
                {
                    TelemetrySubsystem->LogEvent(
                        TEXT("pve_enemy_spawned"),
                        FString::Printf(TEXT("{\"enemy\":\"%s\",\"target\":\"%s\"}"), *GetNameSafe(EnemyActor), *GetNameSafe(TargetCharacter)));
                    TelemetrySubsystem->LogEvent(
                        TEXT("pve_spawned"),
                        FString::Printf(TEXT("{\"enemy\":\"%s\",\"target\":\"%s\"}"), *GetNameSafe(EnemyActor), *GetNameSafe(TargetCharacter)));
                }
            }
        }
    }

    const bool bAttacked = EnemyActor && TargetCharacter ? EnemyActor->AttackCharacter(TargetCharacter) : false;
    const AAbyssLockPlayerState* TargetPlayerState = TargetCharacter ? TargetCharacter->GetPlayerState<AAbyssLockPlayerState>() : nullptr;
    const bool bTargetDowned = TargetPlayerState && TargetPlayerState->GetLifeState() == EAbyssLifeState::Downed;
    const bool bRescued = bTargetDowned && TargetCharacter ? TargetCharacter->RescueFromDowned(TargetCharacter) : false;
    const bool bSmokePassed = EnemyActor && bAttacked && bTargetDowned && bRescued;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_pve_enemy result=%s spawned=%s attacked=%s targetDowned=%s rescued=%s enemy=%s target=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        EnemyActor ? TEXT("true") : TEXT("false"),
        bAttacked ? TEXT("true") : TEXT("false"),
        bTargetDowned ? TEXT("true") : TEXT("false"),
        bRescued ? TEXT("true") : TEXT("false"),
        *GetNameSafe(EnemyActor),
        *GetNameSafe(TargetCharacter));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_pve_enemy"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"spawned\":%s,\"attacked\":%s,\"targetDowned\":%s,\"rescued\":%s,\"enemy\":\"%s\",\"target\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    EnemyActor ? TEXT("true") : TEXT("false"),
                    bAttacked ? TEXT("true") : TEXT("false"),
                    bTargetDowned ? TEXT("true") : TEXT("false"),
                    bRescued ? TEXT("true") : TEXT("false"),
                    *GetNameSafe(EnemyActor),
                    *GetNameSafe(TargetCharacter)));
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_pve_damage"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"spawned\":%s,\"attacked\":%s,\"targetDowned\":%s,\"rescued\":%s,\"enemy\":\"%s\",\"target\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    EnemyActor ? TEXT("true") : TEXT("false"),
                    bAttacked ? TEXT("true") : TEXT("false"),
                    bTargetDowned ? TEXT("true") : TEXT("false"),
                    bRescued ? TEXT("true") : TEXT("false"),
                    *GetNameSafe(EnemyActor),
                    *GetNameSafe(TargetCharacter)));
        }
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokeItemDrop()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockCharacter* InstigatorCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Crew));
    if (!InstigatorCharacter)
    {
        InstigatorCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Saboteur));
    }

    UAbyssInventoryComponent* InventoryComponent = InstigatorCharacter ? InstigatorCharacter->GetInventoryComponent() : nullptr;
    const FName SmokeItemId(TEXT("SMOKE_FUEL_CELL"));
    const int32 CountBefore = InventoryComponent ? InventoryComponent->GetItemCount() : 0;
    const bool bAdded = InventoryComponent ? InventoryComponent->TryAddItem(SmokeItemId) : false;
    const int32 CountAfterAdd = InventoryComponent ? InventoryComponent->GetItemCount() : 0;

    AAbyssItemPickupActor* DroppedPickup = nullptr;
    if (InventoryComponent && InstigatorCharacter)
    {
        const FVector DropLocation = InstigatorCharacter->GetActorLocation() + InstigatorCharacter->GetActorForwardVector() * 120.0f + FVector(0.0f, 0.0f, 20.0f);
        DroppedPickup = InventoryComponent->TryDropFirstItem(DropLocation, InstigatorCharacter->GetActorRotation());
    }

    const int32 CountAfterDrop = InventoryComponent ? InventoryComponent->GetItemCount() : 0;
    const bool bDroppedSmokeItem = DroppedPickup && DroppedPickup->GetItemId() == SmokeItemId;
    const bool bPickupInteractable = DroppedPickup && InstigatorCharacter ? DroppedPickup->CanInteract(InstigatorCharacter) : false;
    const bool bPickedBackUp = DroppedPickup && InstigatorCharacter ? DroppedPickup->Interact(InstigatorCharacter) : false;
    const bool bConsumed = DroppedPickup ? DroppedPickup->IsConsumed() : false;
    const int32 CountAfterPickup = InventoryComponent ? InventoryComponent->GetItemCount() : 0;
    const bool bInventoryRestored = InventoryComponent && InventoryComponent->HasItem(SmokeItemId) && CountAfterPickup == CountBefore + 1;
    const bool bSmokePassed =
        bAdded &&
        CountAfterAdd == CountBefore + 1 &&
        DroppedPickup &&
        CountAfterDrop == CountBefore &&
        bDroppedSmokeItem &&
        bPickupInteractable &&
        bPickedBackUp &&
        bConsumed &&
        bInventoryRestored;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_item_drop result=%s added=%s dropped=%s pickedBackUp=%s consumed=%s countBefore=%d countAfterAdd=%d countAfterDrop=%d countAfterPickup=%d item=%s pickup=%s pawn=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        bAdded ? TEXT("true") : TEXT("false"),
        DroppedPickup ? TEXT("true") : TEXT("false"),
        bPickedBackUp ? TEXT("true") : TEXT("false"),
        bConsumed ? TEXT("true") : TEXT("false"),
        CountBefore,
        CountAfterAdd,
        CountAfterDrop,
        CountAfterPickup,
        *SmokeItemId.ToString(),
        *GetNameSafe(DroppedPickup),
        *GetNameSafe(InstigatorCharacter));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_item_drop"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"added\":%s,\"dropped\":%s,\"droppedSmokeItem\":%s,\"pickupInteractable\":%s,\"pickedBackUp\":%s,\"consumed\":%s,\"inventoryRestored\":%s,\"countBefore\":%d,\"countAfterAdd\":%d,\"countAfterDrop\":%d,\"countAfterPickup\":%d,\"item\":\"%s\",\"pickup\":\"%s\",\"pawn\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    bAdded ? TEXT("true") : TEXT("false"),
                    DroppedPickup ? TEXT("true") : TEXT("false"),
                    bDroppedSmokeItem ? TEXT("true") : TEXT("false"),
                    bPickupInteractable ? TEXT("true") : TEXT("false"),
                    bPickedBackUp ? TEXT("true") : TEXT("false"),
                    bConsumed ? TEXT("true") : TEXT("false"),
                    bInventoryRestored ? TEXT("true") : TEXT("false"),
                    CountBefore,
                    CountAfterAdd,
                    CountAfterDrop,
                    CountAfterPickup,
                    *SmokeItemId.ToString(),
                    *GetNameSafe(DroppedPickup),
                    *GetNameSafe(InstigatorCharacter)));
        }
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokeCombinedSystems()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    const bool bStartedInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;

    AAbyssLockCharacter* CrewCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Crew));
    AAbyssLockCharacter* SaboteurCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Saboteur));
    if (!CrewCharacter)
    {
        CrewCharacter = SaboteurCharacter;
    }
    if (!SaboteurCharacter)
    {
        SaboteurCharacter = CrewCharacter;
    }

    AAbyssLockCharacter* LifeTargetCharacter = nullptr;
    AAbyssLockCharacter* FallbackNonCrewCharacter = nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PlayerController = It->Get();
        AAbyssLockCharacter* Candidate = PlayerController ? Cast<AAbyssLockCharacter>(PlayerController->GetPawn()) : nullptr;
        if (!Candidate || Candidate == CrewCharacter)
        {
            continue;
        }

        if (!FallbackNonCrewCharacter)
        {
            FallbackNonCrewCharacter = Candidate;
        }

        if (Candidate != SaboteurCharacter)
        {
            LifeTargetCharacter = Candidate;
            break;
        }
    }
    if (!LifeTargetCharacter)
    {
        LifeTargetCharacter = FallbackNonCrewCharacter;
    }
    if (!LifeTargetCharacter)
    {
        LifeTargetCharacter = CrewCharacter;
    }

    UAbyssInventoryComponent* InventoryComponent = CrewCharacter ? CrewCharacter->GetInventoryComponent() : nullptr;
    const FName SmokeItemId(TEXT("SMOKE_COMBINED_FUEL_CELL"));
    const int32 CountBeforeItem = InventoryComponent ? InventoryComponent->GetItemCount() : 0;
    const bool bItemAdded = InventoryComponent ? InventoryComponent->TryAddItem(SmokeItemId) : false;
    const int32 CountAfterItemAdd = InventoryComponent ? InventoryComponent->GetItemCount() : 0;
    AAbyssItemPickupActor* DroppedPickup = nullptr;
    if (InventoryComponent && CrewCharacter)
    {
        const FVector DropLocation = CrewCharacter->GetActorLocation() + CrewCharacter->GetActorForwardVector() * 120.0f + FVector(0.0f, 0.0f, 20.0f);
        DroppedPickup = InventoryComponent->TryDropFirstItem(DropLocation, CrewCharacter->GetActorRotation());
    }
    const int32 CountAfterItemDrop = InventoryComponent ? InventoryComponent->GetItemCount() : 0;
    const bool bDroppedCombinedItem = DroppedPickup && DroppedPickup->GetItemId() == SmokeItemId;
    const bool bDroppedPickupInteractable = DroppedPickup && CrewCharacter ? DroppedPickup->CanInteract(CrewCharacter) : false;
    const bool bPickedDroppedItemBackUp = DroppedPickup && CrewCharacter ? DroppedPickup->Interact(CrewCharacter) : false;
    const bool bDroppedPickupConsumed = DroppedPickup ? DroppedPickup->IsConsumed() : false;
    const int32 CountAfterItemPickup = InventoryComponent ? InventoryComponent->GetItemCount() : 0;
    const bool bItemPassed =
        bItemAdded &&
        CountAfterItemAdd == CountBeforeItem + 1 &&
        DroppedPickup &&
        CountAfterItemDrop == CountBeforeItem &&
        bDroppedCombinedItem &&
        bDroppedPickupInteractable &&
        bPickedDroppedItemBackUp &&
        bDroppedPickupConsumed &&
        InventoryComponent &&
        InventoryComponent->HasItem(SmokeItemId) &&
        CountAfterItemPickup == CountBeforeItem + 1;

    const bool bDowned = LifeTargetCharacter ? LifeTargetCharacter->ApplyServerDamage(999.0f, SaboteurCharacter) : false;
    const bool bRescued = LifeTargetCharacter ? LifeTargetCharacter->RescueFromDowned(CrewCharacter) : false;
    const AAbyssLockPlayerState* LifeTargetState = LifeTargetCharacter ? LifeTargetCharacter->GetPlayerState<AAbyssLockPlayerState>() : nullptr;
    const bool bDownRescuePassed = bDowned && bRescued && LifeTargetState && LifeTargetState->GetLifeState() == EAbyssLifeState::Alive;

    const bool bContained = LifeTargetCharacter ? LifeTargetCharacter->ContainPlayer(SaboteurCharacter) : false;
    const bool bReleased = LifeTargetCharacter ? LifeTargetCharacter->ReleaseFromContainment(CrewCharacter) : false;
    const bool bContainmentPassed = bContained && bReleased && LifeTargetState && LifeTargetState->GetLifeState() == EAbyssLifeState::Alive;

    AAbyssDoorActor* DoorActor = nullptr;
    for (TActorIterator<AAbyssDoorActor> It(GetWorld()); It; ++It)
    {
        DoorActor = *It;
        if (DoorActor)
        {
            break;
        }
    }

    const bool bDoorOpenedBeforeLock = DoorActor && CrewCharacter ? DoorActor->Interact(CrewCharacter) : false;
    const bool bDoorLocked = DoorActor && SaboteurCharacter ? DoorActor->LockForSabotage(SaboteurCharacter) : false;
    const bool bDoorLockedInteractionSucceeded = DoorActor && CrewCharacter ? DoorActor->Interact(CrewCharacter) : false;
    const bool bDoorUnlocked = DoorActor && CrewCharacter ? DoorActor->UnlockFromRepair(CrewCharacter) : false;
    const bool bDoorOpenedAfterUnlock = DoorActor && CrewCharacter ? DoorActor->Interact(CrewCharacter) : false;
    const bool bBulkheadPassed = bDoorOpenedBeforeLock && bDoorLocked && !bDoorLockedInteractionSucceeded && bDoorUnlocked && bDoorOpenedAfterUnlock;

    AAbyssShipTaskActor* FloodingSabotageTask = nullptr;
    AAbyssShipTaskActor* FloodingRepairTask = nullptr;
    for (TActorIterator<AAbyssShipTaskActor> It(GetWorld()); It; ++It)
    {
        AAbyssShipTaskActor* TaskActor = *It;
        if (!TaskActor || TaskActor->GetTargetSystem() != EAbyssShipSystem::Flooding)
        {
            continue;
        }

        if (TaskActor->GetTaskMode() == EAbyssShipTaskMode::Sabotage)
        {
            FloodingSabotageTask = TaskActor;
        }
        else if (TaskActor->GetTaskMode() == EAbyssShipTaskMode::Repair)
        {
            FloodingRepairTask = TaskActor;
        }
    }

    const float PressureBefore = AbyssGameState ? AbyssGameState->GetFloodingPressure() : 0.0f;
    int32 PumpSabotageCount = 0;
    if (FloodingSabotageTask && SaboteurCharacter)
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (FloodingSabotageTask->Interact(SaboteurCharacter))
            {
                ++PumpSabotageCount;
            }
        }
    }
    const float PressureAfterSabotage = AbyssGameState ? AbyssGameState->GetFloodingPressure() : 0.0f;
    const bool bPumpRepaired = FloodingRepairTask && CrewCharacter ? FloodingRepairTask->Interact(CrewCharacter) : false;
    const float PressureAfterRepair = AbyssGameState ? AbyssGameState->GetFloodingPressure() : 0.0f;
    const bool bPumpPassed = PumpSabotageCount == 2 && bPumpRepaired && PressureAfterSabotage > PressureBefore && PressureAfterRepair < PressureAfterSabotage && PressureAfterRepair > PressureBefore;

    AAbyssPveEnemyActor* EnemyActor = nullptr;
    if (LifeTargetCharacter)
    {
        const FVector SpawnLocation = LifeTargetCharacter->GetActorLocation() + FVector(180.0f, 0.0f, 0.0f);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        EnemyActor = GetWorld()->SpawnActor<AAbyssPveEnemyActor>(AAbyssPveEnemyActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
        if (EnemyActor)
        {
            EnemyActor->ConfigureEnemy(999.0f);
            UE_LOG(LogAbyssGameplay, Log, TEXT("pve_enemy_spawned enemy=%s target=%s source=combined_systems"), *GetNameSafe(EnemyActor), *GetNameSafe(LifeTargetCharacter));
            if (UGameInstance* GameInstance = GetGameInstance())
            {
                if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
                {
                    TelemetrySubsystem->LogEvent(
                        TEXT("pve_enemy_spawned"),
                        FString::Printf(TEXT("{\"enemy\":\"%s\",\"target\":\"%s\",\"source\":\"combined_systems\"}"), *GetNameSafe(EnemyActor), *GetNameSafe(LifeTargetCharacter)));
                    TelemetrySubsystem->LogEvent(
                        TEXT("pve_spawned"),
                        FString::Printf(TEXT("{\"enemy\":\"%s\",\"target\":\"%s\",\"source\":\"combined_systems\"}"), *GetNameSafe(EnemyActor), *GetNameSafe(LifeTargetCharacter)));
                }
            }
        }
    }

    const bool bPveAttacked = EnemyActor && LifeTargetCharacter ? EnemyActor->AttackCharacter(LifeTargetCharacter) : false;
    const bool bPveTargetDowned = LifeTargetState && LifeTargetState->GetLifeState() == EAbyssLifeState::Downed;
    const bool bPveRescued = LifeTargetCharacter ? LifeTargetCharacter->RescueFromDowned(CrewCharacter) : false;
    const bool bPvePassed = EnemyActor && bPveAttacked && bPveTargetDowned && bPveRescued && LifeTargetState && LifeTargetState->GetLifeState() == EAbyssLifeState::Alive;

    const bool bStillInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;
    const bool bSmokePassed = bStartedInProgress && bItemPassed && bDownRescuePassed && bContainmentPassed && bBulkheadPassed && bPumpPassed && bPvePassed && bStillInProgress;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_combined_systems result=%s item=%s downRescue=%s containment=%s bulkhead=%s pump=%s pve=%s stillInProgress=%s crew=%s saboteur=%s target=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        bItemPassed ? TEXT("true") : TEXT("false"),
        bDownRescuePassed ? TEXT("true") : TEXT("false"),
        bContainmentPassed ? TEXT("true") : TEXT("false"),
        bBulkheadPassed ? TEXT("true") : TEXT("false"),
        bPumpPassed ? TEXT("true") : TEXT("false"),
        bPvePassed ? TEXT("true") : TEXT("false"),
        bStillInProgress ? TEXT("true") : TEXT("false"),
        *GetNameSafe(CrewCharacter),
        *GetNameSafe(SaboteurCharacter),
        *GetNameSafe(LifeTargetCharacter));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_combined_systems"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"startedInProgress\":%s,\"item\":%s,\"downRescue\":%s,\"containment\":%s,\"bulkhead\":%s,\"pump\":%s,\"pve\":%s,\"stillInProgress\":%s,\"pressureBefore\":%.3f,\"pressureAfterSabotage\":%.3f,\"pressureAfterRepair\":%.3f,\"crew\":\"%s\",\"saboteur\":\"%s\",\"target\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    bStartedInProgress ? TEXT("true") : TEXT("false"),
                    bItemPassed ? TEXT("true") : TEXT("false"),
                    bDownRescuePassed ? TEXT("true") : TEXT("false"),
                    bContainmentPassed ? TEXT("true") : TEXT("false"),
                    bBulkheadPassed ? TEXT("true") : TEXT("false"),
                    bPumpPassed ? TEXT("true") : TEXT("false"),
                    bPvePassed ? TEXT("true") : TEXT("false"),
                    bStillInProgress ? TEXT("true") : TEXT("false"),
                    PressureBefore,
                    PressureAfterSabotage,
                    PressureAfterRepair,
                    *GetNameSafe(CrewCharacter),
                    *GetNameSafe(SaboteurCharacter),
                    *GetNameSafe(LifeTargetCharacter)));
        }
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokeQaBot()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    const FVector StartLocation(0.0f, -700.0f, 140.0f);
    const FVector TargetLocation(360.0f, -700.0f, 140.0f);
    const FName BotItemId(TEXT("SMOKE_QA_BOT_SAMPLE"));

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAbyssLockCharacter* BotPawn = GetWorld()->SpawnActor<AAbyssLockCharacter>(AAbyssLockCharacter::StaticClass(), StartLocation, FRotator::ZeroRotator, SpawnParameters);
    AAbyssItemPickupActor* PickupActor = GetWorld()->SpawnActor<AAbyssItemPickupActor>(AAbyssItemPickupActor::StaticClass(), TargetLocation, FRotator::ZeroRotator, SpawnParameters);
    if (PickupActor)
    {
        PickupActor->ConfigureItem(BotItemId);
    }

    UE_LOG(LogAbyssGameplay, Log, TEXT("qa_bot_spawned bot=%s pickup=%s item=%s"), *GetNameSafe(BotPawn), *GetNameSafe(PickupActor), *BotItemId.ToString());
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("qa_bot_spawned"),
                FString::Printf(TEXT("{\"bot\":\"%s\",\"pickup\":\"%s\",\"item\":\"%s\"}"), *GetNameSafe(BotPawn), *GetNameSafe(PickupActor), *BotItemId.ToString()));
        }
    }

    constexpr int32 MoveSteps = 4;
    bool bMoved = false;
    float DistanceRemaining = 0.0f;
    if (BotPawn)
    {
        for (int32 Step = 1; Step <= MoveSteps; ++Step)
        {
            const float Alpha = static_cast<float>(Step) / static_cast<float>(MoveSteps);
            const FVector NextLocation = FMath::Lerp(StartLocation, TargetLocation + FVector(-40.0f, 0.0f, 0.0f), Alpha);
            BotPawn->SetActorLocation(NextLocation, false, nullptr, ETeleportType::TeleportPhysics);
        }
        DistanceRemaining = FVector::Dist(BotPawn->GetActorLocation(), TargetLocation);
        bMoved = DistanceRemaining <= 60.0f;
    }

    UE_LOG(LogAbyssGameplay, Log, TEXT("qa_bot_moved bot=%s steps=%d distanceRemaining=%.1f"), *GetNameSafe(BotPawn), MoveSteps, DistanceRemaining);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("qa_bot_moved"),
                FString::Printf(TEXT("{\"bot\":\"%s\",\"steps\":%d,\"distanceRemaining\":%.1f,\"moved\":%s}"), *GetNameSafe(BotPawn), MoveSteps, DistanceRemaining, bMoved ? TEXT("true") : TEXT("false")));
        }
    }

    const bool bCanInteract = PickupActor && BotPawn ? PickupActor->CanInteract(BotPawn) : false;
    const bool bInteracted = PickupActor && BotPawn ? PickupActor->Interact(BotPawn) : false;
    const UAbyssInventoryComponent* InventoryComponent = BotPawn ? BotPawn->GetInventoryComponent() : nullptr;
    const bool bInventoryHasItem = InventoryComponent && InventoryComponent->HasItem(BotItemId);
    const bool bPickupConsumed = PickupActor && PickupActor->IsConsumed();
    const bool bSmokePassed = BotPawn && PickupActor && bMoved && bCanInteract && bInteracted && bInventoryHasItem && bPickupConsumed;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_qa_bot result=%s spawned=%s moved=%s canInteract=%s interacted=%s inventoryHasItem=%s pickupConsumed=%s bot=%s pickup=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        (BotPawn && PickupActor) ? TEXT("true") : TEXT("false"),
        bMoved ? TEXT("true") : TEXT("false"),
        bCanInteract ? TEXT("true") : TEXT("false"),
        bInteracted ? TEXT("true") : TEXT("false"),
        bInventoryHasItem ? TEXT("true") : TEXT("false"),
        bPickupConsumed ? TEXT("true") : TEXT("false"),
        *GetNameSafe(BotPawn),
        *GetNameSafe(PickupActor));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("qa_bot_interacted"),
                FString::Printf(
                    TEXT("{\"bot\":\"%s\",\"pickup\":\"%s\",\"item\":\"%s\",\"canInteract\":%s,\"interacted\":%s,\"inventoryHasItem\":%s,\"pickupConsumed\":%s}"),
                    *GetNameSafe(BotPawn),
                    *GetNameSafe(PickupActor),
                    *BotItemId.ToString(),
                    bCanInteract ? TEXT("true") : TEXT("false"),
                    bInteracted ? TEXT("true") : TEXT("false"),
                    bInventoryHasItem ? TEXT("true") : TEXT("false"),
                    bPickupConsumed ? TEXT("true") : TEXT("false")));
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_qa_bot"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"spawned\":%s,\"moved\":%s,\"canInteract\":%s,\"interacted\":%s,\"inventoryHasItem\":%s,\"pickupConsumed\":%s,\"distanceRemaining\":%.1f,\"bot\":\"%s\",\"pickup\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    (BotPawn && PickupActor) ? TEXT("true") : TEXT("false"),
                    bMoved ? TEXT("true") : TEXT("false"),
                    bCanInteract ? TEXT("true") : TEXT("false"),
                    bInteracted ? TEXT("true") : TEXT("false"),
                    bInventoryHasItem ? TEXT("true") : TEXT("false"),
                    bPickupConsumed ? TEXT("true") : TEXT("false"),
                    DistanceRemaining,
                    *GetNameSafe(BotPawn),
                    *GetNameSafe(PickupActor)));
        }
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokeQaPlayerBot()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    const bool bStartedInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;

    AAbyssLockCharacter* BotPawn = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Crew));
    if (!BotPawn)
    {
        BotPawn = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Saboteur));
    }

    AAbyssDoorActor* DoorActor = nullptr;
    for (TActorIterator<AAbyssDoorActor> It(GetWorld()); It; ++It)
    {
        DoorActor = *It;
        if (DoorActor)
        {
            break;
        }
    }

    const FVector StartLocation = BotPawn ? BotPawn->GetActorLocation() : FVector::ZeroVector;
    const FVector TargetLocation = DoorActor ? DoorActor->GetActorLocation() + FVector(-120.0f, 0.0f, 0.0f) : FVector::ZeroVector;
    bool bMoved = false;
    float DistanceRemaining = 0.0f;
    if (BotPawn && DoorActor)
    {
        BotPawn->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
        DistanceRemaining = FVector::Dist(BotPawn->GetActorLocation(), DoorActor->GetActorLocation());
        bMoved = DistanceRemaining <= 140.0f;
    }

    UE_LOG(LogAbyssGameplay, Log, TEXT("qa_player_bot_moved pawn=%s door=%s distanceRemaining=%.1f"), *GetNameSafe(BotPawn), *GetNameSafe(DoorActor), DistanceRemaining);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("qa_player_bot_moved"),
                FString::Printf(
                    TEXT("{\"pawn\":\"%s\",\"door\":\"%s\",\"distanceRemaining\":%.1f,\"moved\":%s}"),
                    *GetNameSafe(BotPawn),
                    *GetNameSafe(DoorActor),
                    DistanceRemaining,
                    bMoved ? TEXT("true") : TEXT("false")));
        }
    }

    const bool bCanInteractBeforeOpen = DoorActor && BotPawn ? DoorActor->CanInteract(BotPawn) : false;
    const bool bOpened = DoorActor && BotPawn ? DoorActor->Interact(BotPawn) : false;
    const bool bOpenState = DoorActor && DoorActor->IsOpen();
    const bool bCanInteractBeforeClose = DoorActor && BotPawn ? DoorActor->CanInteract(BotPawn) : false;
    const bool bClosed = DoorActor && BotPawn ? DoorActor->Interact(BotPawn) : false;
    const bool bClosedState = DoorActor && !DoorActor->IsOpen() && !DoorActor->IsLocked();
    const bool bStillInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;
    const bool bSmokePassed =
        bStartedInProgress &&
        BotPawn &&
        DoorActor &&
        bMoved &&
        bCanInteractBeforeOpen &&
        bOpened &&
        bOpenState &&
        bCanInteractBeforeClose &&
        bClosed &&
        bClosedState &&
        bStillInProgress;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_qa_player_bot result=%s startedInProgress=%s moved=%s canOpen=%s opened=%s openState=%s canClose=%s closed=%s closedState=%s stillInProgress=%s pawn=%s door=%s start=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        bStartedInProgress ? TEXT("true") : TEXT("false"),
        bMoved ? TEXT("true") : TEXT("false"),
        bCanInteractBeforeOpen ? TEXT("true") : TEXT("false"),
        bOpened ? TEXT("true") : TEXT("false"),
        bOpenState ? TEXT("true") : TEXT("false"),
        bCanInteractBeforeClose ? TEXT("true") : TEXT("false"),
        bClosed ? TEXT("true") : TEXT("false"),
        bClosedState ? TEXT("true") : TEXT("false"),
        bStillInProgress ? TEXT("true") : TEXT("false"),
        *GetNameSafe(BotPawn),
        *GetNameSafe(DoorActor),
        *StartLocation.ToCompactString());

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("qa_player_bot_interacted"),
                FString::Printf(
                    TEXT("{\"pawn\":\"%s\",\"door\":\"%s\",\"canOpen\":%s,\"opened\":%s,\"openState\":%s,\"canClose\":%s,\"closed\":%s,\"closedState\":%s}"),
                    *GetNameSafe(BotPawn),
                    *GetNameSafe(DoorActor),
                    bCanInteractBeforeOpen ? TEXT("true") : TEXT("false"),
                    bOpened ? TEXT("true") : TEXT("false"),
                    bOpenState ? TEXT("true") : TEXT("false"),
                    bCanInteractBeforeClose ? TEXT("true") : TEXT("false"),
                    bClosed ? TEXT("true") : TEXT("false"),
                    bClosedState ? TEXT("true") : TEXT("false")));
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_qa_player_bot"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"startedInProgress\":%s,\"moved\":%s,\"canOpen\":%s,\"opened\":%s,\"openState\":%s,\"canClose\":%s,\"closed\":%s,\"closedState\":%s,\"stillInProgress\":%s,\"distanceRemaining\":%.1f,\"pawn\":\"%s\",\"door\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    bStartedInProgress ? TEXT("true") : TEXT("false"),
                    bMoved ? TEXT("true") : TEXT("false"),
                    bCanInteractBeforeOpen ? TEXT("true") : TEXT("false"),
                    bOpened ? TEXT("true") : TEXT("false"),
                    bOpenState ? TEXT("true") : TEXT("false"),
                    bCanInteractBeforeClose ? TEXT("true") : TEXT("false"),
                    bClosed ? TEXT("true") : TEXT("false"),
                    bClosedState ? TEXT("true") : TEXT("false"),
                    bStillInProgress ? TEXT("true") : TEXT("false"),
                    DistanceRemaining,
                    *GetNameSafe(BotPawn),
                    *GetNameSafe(DoorActor)));
        }
    }
#endif
}

void AAbyssLockGameMode::RunDevSmokeQaTaskBot()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    const bool bStartedInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;

    APawn* CrewPawn = FindPawnForTeam(EAbyssTeam::Crew);
    APawn* SaboteurPawn = FindPawnForTeam(EAbyssTeam::Saboteur);

    AAbyssShipTaskActor* SabotageTask = nullptr;
    AAbyssShipTaskActor* RepairTask = nullptr;
    for (TActorIterator<AAbyssShipTaskActor> SabotageIt(GetWorld()); SabotageIt; ++SabotageIt)
    {
        AAbyssShipTaskActor* CandidateSabotage = *SabotageIt;
        if (!CandidateSabotage || CandidateSabotage->GetTaskMode() != EAbyssShipTaskMode::Sabotage)
        {
            continue;
        }

        const EAbyssShipSystem CandidateSystem = CandidateSabotage->GetTargetSystem();
        if (CandidateSystem == EAbyssShipSystem::Route || CandidateSystem == EAbyssShipSystem::Flooding)
        {
            continue;
        }

        for (TActorIterator<AAbyssShipTaskActor> RepairIt(GetWorld()); RepairIt; ++RepairIt)
        {
            AAbyssShipTaskActor* CandidateRepair = *RepairIt;
            if (CandidateRepair && CandidateRepair->GetTaskMode() == EAbyssShipTaskMode::Repair && CandidateRepair->GetTargetSystem() == CandidateSystem)
            {
                SabotageTask = CandidateSabotage;
                RepairTask = CandidateRepair;
                break;
            }
        }

        if (SabotageTask && RepairTask)
        {
            break;
        }
    }

    const EAbyssShipSystem TargetSystem = SabotageTask ? SabotageTask->GetTargetSystem() : EAbyssShipSystem::Hull;
    FAbyssShipSystemStatus StatusBefore;
    FAbyssShipSystemStatus StatusAfterSabotage;
    FAbyssShipSystemStatus StatusAfterRepair;
    const bool bHadStatusBefore = AbyssGameState && AbyssGameState->GetShipSystemStatus(TargetSystem, StatusBefore);

    bool bMovedSaboteur = false;
    bool bMovedCrew = false;
    if (SaboteurPawn && SabotageTask)
    {
        SaboteurPawn->SetActorLocation(SabotageTask->GetActorLocation() + FVector(-100.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
        bMovedSaboteur = FVector::Dist(SaboteurPawn->GetActorLocation(), SabotageTask->GetActorLocation()) <= 120.0f;
    }
    if (CrewPawn && RepairTask)
    {
        CrewPawn->SetActorLocation(RepairTask->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
        bMovedCrew = FVector::Dist(CrewPawn->GetActorLocation(), RepairTask->GetActorLocation()) <= 120.0f;
    }

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("qa_task_bot_moved crew=%s saboteur=%s repairTask=%s sabotageTask=%s movedCrew=%s movedSaboteur=%s"),
        *GetNameSafe(CrewPawn),
        *GetNameSafe(SaboteurPawn),
        *GetNameSafe(RepairTask),
        *GetNameSafe(SabotageTask),
        bMovedCrew ? TEXT("true") : TEXT("false"),
        bMovedSaboteur ? TEXT("true") : TEXT("false"));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("qa_task_bot_moved"),
                FString::Printf(
                    TEXT("{\"crew\":\"%s\",\"saboteur\":\"%s\",\"repairTask\":\"%s\",\"sabotageTask\":\"%s\",\"movedCrew\":%s,\"movedSaboteur\":%s}"),
                    *GetNameSafe(CrewPawn),
                    *GetNameSafe(SaboteurPawn),
                    *GetNameSafe(RepairTask),
                    *GetNameSafe(SabotageTask),
                    bMovedCrew ? TEXT("true") : TEXT("false"),
                    bMovedSaboteur ? TEXT("true") : TEXT("false")));
        }
    }

    const bool bCanSabotage = SabotageTask && SaboteurPawn ? SabotageTask->CanInteract(SaboteurPawn) : false;
    const bool bSabotaged = SabotageTask && SaboteurPawn ? SabotageTask->Interact(SaboteurPawn) : false;
    const bool bHadStatusAfterSabotage = AbyssGameState && AbyssGameState->GetShipSystemStatus(TargetSystem, StatusAfterSabotage);
    const bool bCanRepair = RepairTask && CrewPawn ? RepairTask->CanInteract(CrewPawn) : false;
    const bool bRepaired = RepairTask && CrewPawn ? RepairTask->Interact(CrewPawn) : false;
    const bool bHadStatusAfterRepair = AbyssGameState && AbyssGameState->GetShipSystemStatus(TargetSystem, StatusAfterRepair);
    const bool bConditionDropped = bHadStatusBefore && bHadStatusAfterSabotage && StatusAfterSabotage.Condition < StatusBefore.Condition;
    const bool bConditionRestored = bHadStatusBefore && bHadStatusAfterRepair && StatusAfterRepair.Condition >= StatusBefore.Condition;
    const bool bStillInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;
    const bool bSmokePassed =
        bStartedInProgress &&
        bMovedCrew &&
        bMovedSaboteur &&
        bCanSabotage &&
        bSabotaged &&
        bConditionDropped &&
        bCanRepair &&
        bRepaired &&
        bConditionRestored &&
        bStillInProgress;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_qa_task_bot result=%s startedInProgress=%s movedCrew=%s movedSaboteur=%s canSabotage=%s sabotaged=%s conditionBefore=%.2f conditionAfterSabotage=%.2f canRepair=%s repaired=%s conditionAfterRepair=%.2f stillInProgress=%s system=%d crew=%s saboteur=%s repairTask=%s sabotageTask=%s"),
        bSmokePassed ? TEXT("pass") : TEXT("fail"),
        bStartedInProgress ? TEXT("true") : TEXT("false"),
        bMovedCrew ? TEXT("true") : TEXT("false"),
        bMovedSaboteur ? TEXT("true") : TEXT("false"),
        bCanSabotage ? TEXT("true") : TEXT("false"),
        bSabotaged ? TEXT("true") : TEXT("false"),
        bHadStatusBefore ? StatusBefore.Condition : -1.0f,
        bHadStatusAfterSabotage ? StatusAfterSabotage.Condition : -1.0f,
        bCanRepair ? TEXT("true") : TEXT("false"),
        bRepaired ? TEXT("true") : TEXT("false"),
        bHadStatusAfterRepair ? StatusAfterRepair.Condition : -1.0f,
        bStillInProgress ? TEXT("true") : TEXT("false"),
        static_cast<int32>(TargetSystem),
        *GetNameSafe(CrewPawn),
        *GetNameSafe(SaboteurPawn),
        *GetNameSafe(RepairTask),
        *GetNameSafe(SabotageTask));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("qa_task_bot_interacted"),
                FString::Printf(
                    TEXT("{\"crew\":\"%s\",\"saboteur\":\"%s\",\"repairTask\":\"%s\",\"sabotageTask\":\"%s\",\"system\":%d,\"canSabotage\":%s,\"sabotaged\":%s,\"canRepair\":%s,\"repaired\":%s}"),
                    *GetNameSafe(CrewPawn),
                    *GetNameSafe(SaboteurPawn),
                    *GetNameSafe(RepairTask),
                    *GetNameSafe(SabotageTask),
                    static_cast<int32>(TargetSystem),
                    bCanSabotage ? TEXT("true") : TEXT("false"),
                    bSabotaged ? TEXT("true") : TEXT("false"),
                    bCanRepair ? TEXT("true") : TEXT("false"),
                    bRepaired ? TEXT("true") : TEXT("false")));
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_qa_task_bot"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"startedInProgress\":%s,\"movedCrew\":%s,\"movedSaboteur\":%s,\"canSabotage\":%s,\"sabotaged\":%s,\"conditionDropped\":%s,\"canRepair\":%s,\"repaired\":%s,\"conditionRestored\":%s,\"stillInProgress\":%s,\"system\":%d,\"conditionBefore\":%.3f,\"conditionAfterSabotage\":%.3f,\"conditionAfterRepair\":%.3f,\"crew\":\"%s\",\"saboteur\":\"%s\",\"repairTask\":\"%s\",\"sabotageTask\":\"%s\"}"),
                    bSmokePassed ? TEXT("pass") : TEXT("fail"),
                    bStartedInProgress ? TEXT("true") : TEXT("false"),
                    bMovedCrew ? TEXT("true") : TEXT("false"),
                    bMovedSaboteur ? TEXT("true") : TEXT("false"),
                    bCanSabotage ? TEXT("true") : TEXT("false"),
                    bSabotaged ? TEXT("true") : TEXT("false"),
                    bConditionDropped ? TEXT("true") : TEXT("false"),
                    bCanRepair ? TEXT("true") : TEXT("false"),
                    bRepaired ? TEXT("true") : TEXT("false"),
                    bConditionRestored ? TEXT("true") : TEXT("false"),
                    bStillInProgress ? TEXT("true") : TEXT("false"),
                    static_cast<int32>(TargetSystem),
                    bHadStatusBefore ? StatusBefore.Condition : -1.0f,
                    bHadStatusAfterSabotage ? StatusAfterSabotage.Condition : -1.0f,
                    bHadStatusAfterRepair ? StatusAfterRepair.Condition : -1.0f,
                    *GetNameSafe(CrewPawn),
                    *GetNameSafe(SaboteurPawn),
                    *GetNameSafe(RepairTask),
                    *GetNameSafe(SabotageTask)));
        }
    }
#endif
}

APawn* AAbyssLockGameMode::FindPawnForTeam(EAbyssTeam Team) const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PlayerController = It->Get();
        if (!PlayerController)
        {
            continue;
        }

        const AAbyssLockPlayerState* AbyssPlayerState = PlayerController->GetPlayerState<AAbyssLockPlayerState>();
        if (AbyssPlayerState && AbyssPlayerState->GetSecretTeamForOwner() == Team)
        {
            return PlayerController->GetPawn();
        }
    }

    return nullptr;
}

bool AAbyssLockGameMode::EvaluateMatchEnd()
{
    if (!HasAuthority())
    {
        return false;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    if (!AbyssGameState || AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::MatchEnded)
    {
        return false;
    }

    int32 TotalAssignedPlayers = 0;
    int32 LivingCrew = 0;
    int32 ActiveCrew = 0;

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        const AAbyssLockPlayerState* AbyssPlayerState = Cast<AAbyssLockPlayerState>(PlayerState);
        if (!AbyssPlayerState)
        {
            continue;
        }

        const EAbyssTeam Team = AbyssPlayerState->GetSecretTeamForOwner();
        if (Team == EAbyssTeam::Unassigned || Team == EAbyssTeam::Spectator)
        {
            continue;
        }

        ++TotalAssignedPlayers;
        if (Team != EAbyssTeam::Crew)
        {
            continue;
        }

        const EAbyssLifeState LifeState = AbyssPlayerState->GetLifeState();
        if (LifeState == EAbyssLifeState::Alive || LifeState == EAbyssLifeState::Downed || LifeState == EAbyssLifeState::Contained)
        {
            ++LivingCrew;
        }

        if (LifeState == EAbyssLifeState::Alive)
        {
            ++ActiveCrew;
        }
    }

    if (AbyssGameState->HasFatalShipSystem())
    {
        AbyssGameState->SetMatchResult(EAbyssTeam::Saboteur, TEXT("fatal_ship_state"));
        UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=saboteur reason=fatal_ship_state"));
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(TEXT("match_ended"), TEXT("{\"winner\":\"saboteur\",\"reason\":\"fatal_ship_state\"}"));
            }
        }
        return true;
    }

    if (TotalAssignedPlayers >= 5 && LivingCrew <= GetSaboteurEliminationThreshold(TotalAssignedPlayers))
    {
        AbyssGameState->SetMatchResult(EAbyssTeam::Saboteur, TEXT("crew_threshold"));
        UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=saboteur reason=crew_threshold livingCrew=%d"), LivingCrew);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("match_ended"),
                    FString::Printf(TEXT("{\"winner\":\"saboteur\",\"reason\":\"crew_threshold\",\"livingCrew\":%d}"), LivingCrew));
            }
        }
        return true;
    }

    if (AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::FinalApproach && ActiveCrew >= GetRequiredCrewSurvivors(TotalAssignedPlayers))
    {
        AbyssGameState->SetMatchResult(EAbyssTeam::Crew, TEXT("final_approach_complete"));
        UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=crew reason=final_approach_complete activeCrew=%d"), ActiveCrew);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("match_ended"),
                    FString::Printf(TEXT("{\"winner\":\"crew\",\"reason\":\"final_approach_complete\",\"activeCrew\":%d}"), ActiveCrew));
            }
        }
        return true;
    }

    return false;
}

bool AAbyssLockGameMode::TryStartMatchFromReady()
{
    if (!HasAuthority() || !GameState)
    {
        return false;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    if (!AbyssGameState || AbyssGameState->GetMatchPhase() != EAbyssMatchPhase::WaitingForPlayers)
    {
        return false;
    }

    int32 PlayerCount = 0;
    int32 ReadyCount = 0;
    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        const AAbyssLockPlayerState* AbyssPlayerState = Cast<AAbyssLockPlayerState>(PlayerState);
        if (!AbyssPlayerState)
        {
            continue;
        }

        ++PlayerCount;
        if (AbyssPlayerState->IsReadyForMatch())
        {
            ++ReadyCount;
        }
    }

    int32 MinimumPlayers = 5;
#if !UE_BUILD_SHIPPING
    FParse::Value(FCommandLine::Get(), TEXT("AbyssLobbyMinPlayers="), MinimumPlayers);
#endif

    if (PlayerCount < FMath::Max(1, MinimumPlayers) || PlayerCount > 8 || ReadyCount != PlayerCount)
    {
        UE_LOG(LogAbyssGameplay, Log, TEXT("lobby_start_waiting players=%d ready=%d"), PlayerCount, ReadyCount);
        return false;
    }

    int32 SaboteurOverride = INDEX_NONE;
#if !UE_BUILD_SHIPPING
    FParse::Value(FCommandLine::Get(), TEXT("AbyssLobbyDevSaboteurs="), SaboteurOverride);
#endif

    AssignRolesForCurrentPlayersInternal(SaboteurOverride);
    AbyssGameState->SetMatchPhase(EAbyssMatchPhase::InProgress);
    UE_LOG(LogAbyssGameplay, Log, TEXT("match_started source=ready_lobby players=%d ready=%d"), PlayerCount, ReadyCount);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("match_started"),
                FString::Printf(TEXT("{\"source\":\"ready_lobby\",\"players\":%d,\"ready\":%d}"), PlayerCount, ReadyCount));
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeInteract")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeTaskInteraction);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeDownRescue")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeDownRescue);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeContainment")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeContainment);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeRouteComplete")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeRouteComplete);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeFatalSabotage")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeFatalSabotage);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeBulkheadLock")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeBulkheadLock);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokePumpFlooding")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokePumpFlooding);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokePveEnemy")) || FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokePvEDamage")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokePveEnemy);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeItemDrop")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeItemDrop);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeCombinedSystems")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeCombinedSystems);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeQaBot")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeQaBot);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeQaPlayerBot")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeQaPlayerBot);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeQaTaskBot")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeQaTaskBot);
    }
#endif

    return true;
}

int32 AAbyssLockGameMode::CalculateSaboteurCount(int32 PlayerCount) const
{
    if (PlayerCount >= 7)
    {
        return 2;
    }

    if (PlayerCount >= 5)
    {
        return 1;
    }

    return 0;
}

int32 AAbyssLockGameMode::GetRequiredCrewSurvivors(int32 PlayerCount) const
{
    if (PlayerCount >= 8)
    {
        return 5;
    }

    if (PlayerCount >= 6)
    {
        return 4;
    }

    return 3;
}

int32 AAbyssLockGameMode::GetSaboteurEliminationThreshold(int32 PlayerCount) const
{
    if (PlayerCount >= 8)
    {
        return 3;
    }

    return 2;
}
