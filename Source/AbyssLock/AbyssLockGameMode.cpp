#include "AbyssLockGameMode.h"
#include "AbyssDoorActor.h"
#include "AbyssInventoryComponent.h"
#include "AbyssItemPickupActor.h"
#include "AbyssLifeActionActor.h"
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

namespace
{
constexpr int32 AbyssFixedMatchPlayers = 8;
constexpr int32 AbyssPracticePlayers = 1;

// Voyage pacing (Option A): the route advances over time only while the ship is underway (fueled),
// so completing the voyage takes a sustained ~25 min of keeping the ship fuelled (the win), inside
// the ~30 min match timer (the loss). See docs/solo-voyage-completion-spec.md.
constexpr float AbyssVoyageRouteProgressPerSecond = 1.0f / 1500.0f; // ~25 min of uptime to reach 100%
constexpr float AbyssVoyageFuelBurnPerSecond = 1.0f / 360.0f;       // a full fuel tank lasts ~6 min underway

FString AbyssMatchModeToString(EAbyssMatchMode Mode)
{
    return Mode == EAbyssMatchMode::Madman ? TEXT("madman") : TEXT("standard");
}

FString AbyssDifficultyToString(EAbyssDifficulty Difficulty)
{
    switch (Difficulty)
    {
    case EAbyssDifficulty::Easy:
        return TEXT("easy");
    case EAbyssDifficulty::Hard:
        return TEXT("hard");
    case EAbyssDifficulty::Normal:
    default:
        return TEXT("normal");
    }
}

bool AbyssParseMatchMode(const FString& Value, EAbyssMatchMode& OutMode)
{
    const FString Lower = Value.ToLower();
    if (Lower == TEXT("madman"))
    {
        OutMode = EAbyssMatchMode::Madman;
        return true;
    }
    if (Lower == TEXT("standard"))
    {
        OutMode = EAbyssMatchMode::Standard;
        return true;
    }
    return false;
}

bool AbyssParseDifficulty(const FString& Value, EAbyssDifficulty& OutDifficulty)
{
    const FString Lower = Value.ToLower();
    if (Lower == TEXT("easy"))
    {
        OutDifficulty = EAbyssDifficulty::Easy;
        return true;
    }
    if (Lower == TEXT("normal"))
    {
        OutDifficulty = EAbyssDifficulty::Normal;
        return true;
    }
    if (Lower == TEXT("hard"))
    {
        OutDifficulty = EAbyssDifficulty::Hard;
        return true;
    }
    return false;
}

const TCHAR* AbyssShipSystemJsonName(EAbyssShipSystem System)
{
    switch (System)
    {
    case EAbyssShipSystem::Hull:
        return TEXT("hull");
    case EAbyssShipSystem::Fuel:
        return TEXT("fuel");
    case EAbyssShipSystem::Engine:
        return TEXT("engine");
    case EAbyssShipSystem::Power:
        return TEXT("power");
    case EAbyssShipSystem::Radio:
        return TEXT("radio");
    case EAbyssShipSystem::Route:
        return TEXT("route");
    case EAbyssShipSystem::Heat:
        return TEXT("heat");
    case EAbyssShipSystem::Flooding:
        return TEXT("flooding");
    default:
        return TEXT("unknown");
    }
}

FString BuildFatalShipStatePayload(const AAbyssLockGameState& AbyssGameState)
{
    static const EAbyssShipSystem TrackedSystems[] = {
        EAbyssShipSystem::Hull,
        EAbyssShipSystem::Fuel,
        EAbyssShipSystem::Engine,
        EAbyssShipSystem::Power,
        EAbyssShipSystem::Radio,
        EAbyssShipSystem::Route,
        EAbyssShipSystem::Heat,
        EAbyssShipSystem::Flooding,
    };

    FString CriticalSystemsJson;
    FString SystemConditionsJson;
    bool bFirstCriticalSystem = true;
    bool bFirstCondition = true;

    for (const EAbyssShipSystem System : TrackedSystems)
    {
        FAbyssShipSystemStatus Status;
        if (!AbyssGameState.GetShipSystemStatus(System, Status))
        {
            continue;
        }

        const TCHAR* SystemName = AbyssShipSystemJsonName(System);
        if (Status.Condition <= 0.0f)
        {
            if (!bFirstCriticalSystem)
            {
                CriticalSystemsJson += TEXT(",");
            }
            CriticalSystemsJson += FString::Printf(TEXT("\"%s\""), SystemName);
            bFirstCriticalSystem = false;
        }

        if (!bFirstCondition)
        {
            SystemConditionsJson += TEXT(",");
        }
        SystemConditionsJson += FString::Printf(TEXT("\"%s\":%.3f"), SystemName, Status.Condition);
        bFirstCondition = false;
    }

    return FString::Printf(
        TEXT("{\"winner\":\"saboteur\",\"reason\":\"fatal_ship_state\",\"criticalSystems\":[%s],\"systemConditions\":{%s}}"),
        *CriticalSystemsJson,
        *SystemConditionsJson);
}
}

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

void AAbyssLockGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // A travel URL of `?solo` (used by the main-menu "一人モード" button) enables single-player
    // practice for this gameplay map, which the existing PostLogin auto-start path then begins.
    bSoloUrlRequested = Options.Contains(TEXT("solo"));
    if (bSoloUrlRequested)
    {
        UE_LOG(LogAbyssSession, Log, TEXT("solo_url_requested map=%s"), *MapName);
    }
}

void AAbyssLockGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty())
    {
        return;
    }

    const int32 ConnectedPlayers = GameState ? GameState->PlayerArray.Num() : 0;
    FString RejectReason;
    const int32 MaxConnectedPlayers = IsSinglePlayerModeEnabled() ? AbyssPracticePlayers : AbyssFixedMatchPlayers;
    if (ConnectedPlayers >= MaxConnectedPlayers)
    {
        RejectReason = TEXT("lobby_full");
    }
    else if (const AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>())
    {
        if (AbyssGameState->GetMatchPhase() != EAbyssMatchPhase::WaitingForPlayers)
        {
            RejectReason = TEXT("match_already_started");
        }
    }

    if (RejectReason.IsEmpty())
    {
        return;
    }

    ErrorMessage = RejectReason;
    UE_LOG(LogAbyssSession, Log, TEXT("lobby_join_rejected reason=%s connectedPlayers=%d maxPlayers=%d"), *RejectReason, ConnectedPlayers, MaxConnectedPlayers);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("lobby_join_rejected"),
                FString::Printf(TEXT("{\"reason\":\"%s\",\"connectedPlayers\":%d,\"maxPlayers\":%d}"), *RejectReason, ConnectedPlayers, MaxConnectedPlayers));
        }
    }
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

    if (IsSinglePlayerModeEnabled())
    {
        TryStartSinglePlayerMatch();
        return;
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
    AssignRolesForCurrentPlayersInternal(INDEX_NONE, ActiveMatchConfig.MadmanCount);
}

void AAbyssLockGameMode::AssignRolesForCurrentPlayersInternal(int32 SaboteurCountOverride, int32 MadmanCount)
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
    if (PlayerCount <= 0)
    {
        UE_LOG(LogAbyssGameplay, Warning, TEXT("role_assignment_skipped players=%d"), PlayerCount);
        return;
    }

    const int32 DesiredSaboteurCount = SaboteurCountOverride >= 0 ? SaboteurCountOverride : CalculateSaboteurCount(PlayerCount);
    const int32 SaboteurCount = FMath::Clamp(DesiredSaboteurCount, 0, PlayerCount);
    // Madmen fill the next slots after Saboteurs; clamp so Crew is never fully displaced.
    const int32 ClampedMadmanCount = FMath::Clamp(MadmanCount, 0, FMath::Max(0, PlayerCount - SaboteurCount));

    for (int32 Index = 0; Index < ConnectedPlayers.Num(); ++Index)
    {
        const int32 SwapIndex = FMath::RandRange(Index, ConnectedPlayers.Num() - 1);
        ConnectedPlayers.Swap(Index, SwapIndex);
    }

    for (int32 Index = 0; Index < ConnectedPlayers.Num(); ++Index)
    {
        AAbyssLockPlayerState* PlayerState = ConnectedPlayers[Index];
        EAbyssTeam SecretTeam;
        if (Index < SaboteurCount)
        {
            SecretTeam = EAbyssTeam::Saboteur;
        }
        else if (Index < SaboteurCount + ClampedMadmanCount)
        {
            // Madman knows their own role (owner-only SecretTeam) but is indistinguishable
            // from Crew to everyone else, has no Saboteur abilities, and wins iff Saboteurs win.
            SecretTeam = EAbyssTeam::Madman;
        }
        else
        {
            SecretTeam = EAbyssTeam::Crew;
        }
        PlayerState->SetSecretTeam(SecretTeam);
        PlayerState->SetRevealedTeam(EAbyssTeam::Unassigned);
        PlayerState->SetLifeState(EAbyssLifeState::Alive);
    }

    const FString ModeName = AbyssMatchModeToString(ActiveMatchConfig.Mode);
    UE_LOG(LogAbyssGameplay, Log, TEXT("role_assignment_complete players=%d saboteurs=%d madmen=%d mode=%s"), PlayerCount, SaboteurCount, ClampedMadmanCount, *ModeName);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("role_assignment_complete"),
                FString::Printf(TEXT("{\"players\":%d,\"saboteurs\":%d,\"madmen\":%d,\"mode\":\"%s\"}"), PlayerCount, SaboteurCount, ClampedMadmanCount, *ModeName));
        }
    }
}

void AAbyssLockGameMode::SetActiveMatchConfig(const FAbyssMatchConfig& NewConfig)
{
    if (!HasAuthority())
    {
        return;
    }

    ActiveMatchConfig = NewConfig;
    UE_LOG(LogAbyssGameplay, Log, TEXT("match_config_set mode=%s difficulty=%s saboteurs=%d madmen=%d"),
        *AbyssMatchModeToString(ActiveMatchConfig.Mode), *AbyssDifficultyToString(ActiveMatchConfig.Difficulty),
        ActiveMatchConfig.SaboteurCount, ActiveMatchConfig.MadmanCount);
}

FAbyssMatchConfig AAbyssLockGameMode::ResolveMatchConfig(EAbyssMatchMode Mode, EAbyssDifficulty Difficulty) const
{
    FAbyssMatchConfig Config;
    Config.Mode = Mode;
    Config.Difficulty = Difficulty;
    Config.SaboteurCount = -1; // auto from player count
    Config.MadmanCount = (Mode == EAbyssMatchMode::Madman) ? 1 : 0;
    Config.RequiredCrewSurvivorsOverride = -1; // auto unless the host customizes it

    switch (Difficulty)
    {
    case EAbyssDifficulty::Easy:
        Config.SurvivalDecayMultiplier = 0.75f;
        Config.SabotageIntensityMultiplier = 0.8f;
        Config.FuelBurnMultiplier = 0.8f;
        break;
    case EAbyssDifficulty::Hard:
        Config.SurvivalDecayMultiplier = 1.35f;
        Config.SabotageIntensityMultiplier = 1.25f;
        Config.FuelBurnMultiplier = 1.3f;
        break;
    case EAbyssDifficulty::Normal:
    default:
        Config.SurvivalDecayMultiplier = 1.0f;
        Config.SabotageIntensityMultiplier = 1.0f;
        Config.FuelBurnMultiplier = 1.0f;
        break;
    }

    return Config;
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

    EAbyssMatchMode DevMode = ActiveMatchConfig.Mode;
    EAbyssDifficulty DevDifficulty = ActiveMatchConfig.Difficulty;
    FString DevModeString;
    if (FParse::Value(FCommandLine::Get(), TEXT("AbyssMode="), DevModeString))
    {
        AbyssParseMatchMode(DevModeString, DevMode);
    }
    FString DevDifficultyString;
    if (FParse::Value(FCommandLine::Get(), TEXT("AbyssDifficulty="), DevDifficultyString))
    {
        AbyssParseDifficulty(DevDifficultyString, DevDifficulty);
    }
    ActiveMatchConfig = ResolveMatchConfig(DevMode, DevDifficulty);
    AssignRolesForCurrentPlayersInternal(SaboteurOverride, ActiveMatchConfig.MadmanCount);

    if (AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>())
    {
        AbyssGameState->SetMatchPhase(EAbyssMatchPhase::InProgress);
    }
    StartMatchTimer();

    bDevAutoStarted = true;
    UE_LOG(LogAbyssGameplay, Log, TEXT("dev_auto_start players=%d saboteurs=%d"), PlayerCount, FMath::Clamp(SaboteurOverride, 0, PlayerCount));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("match_started"),
                AppendMatchConfigTelemetry(FString::Printf(TEXT("{\"source\":\"dev_auto_start\",\"players\":%d,\"saboteurs\":%d}"), PlayerCount, FMath::Clamp(SaboteurOverride, 0, PlayerCount))));
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
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeLifeAction")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeLifeAction);
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

void AAbyssLockGameMode::StartMatchTimer()
{
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    float ConfiguredDurationSeconds = MatchDurationSeconds;
    FParse::Value(FCommandLine::Get(), TEXT("AbyssMatchDurationSeconds="), ConfiguredDurationSeconds);
    // Difficulty scales the time limit (Easy = more time, Hard = less).
    float DifficultyTimeScale = 1.0f;
    switch (ActiveMatchConfig.Difficulty)
    {
    case EAbyssDifficulty::Easy:
        DifficultyTimeScale = 1.15f;
        break;
    case EAbyssDifficulty::Hard:
        DifficultyTimeScale = 0.85f;
        break;
    default:
        break;
    }
    MatchDurationSeconds = FMath::Max(1.0f, ConfiguredDurationSeconds * DifficultyTimeScale);
    MatchEndWorldSeconds = GetWorld()->GetTimeSeconds() + MatchDurationSeconds;
    LastLoggedRouteProgress = 0.0f;
    bVoyageStalledLogged = false;

    if (AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>())
    {
        AbyssGameState->SetMatchTimeRemaining(MatchDurationSeconds);
    }

    GetWorldTimerManager().ClearTimer(MatchTimerHandle);
    GetWorldTimerManager().SetTimer(MatchTimerHandle, this, &AAbyssLockGameMode::HandleMatchTimerTick, 1.0f, true);

    UE_LOG(LogAbyssGameplay, Log, TEXT("match_timer_started duration=%.1f"), MatchDurationSeconds);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("match_timer_started"),
                FString::Printf(TEXT("{\"durationSeconds\":%.3f}"), MatchDurationSeconds));
        }
    }
}

void AAbyssLockGameMode::StopMatchTimer()
{
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(MatchTimerHandle);
    }
    MatchEndWorldSeconds = 0.0;
    if (AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>())
    {
        AbyssGameState->SetMatchTimeRemaining(0.0f);
    }
}

void AAbyssLockGameMode::HandleMatchTimerTick()
{
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    if (!AbyssGameState || !AbyssGameState->IsMatchActive())
    {
        StopMatchTimer();
        return;
    }

    // Advance the voyage (route progresses over time while fueled), then resolve win/lose every second
    // so the route completing, the timer, or crew death/incapacitation all end the match promptly even
    // with no ship-task interaction this tick.
    TickVoyage(*AbyssGameState);
    if (EvaluateMatchEnd())
    {
        return;
    }

    const float RemainingSeconds = FMath::Max(0.0f, static_cast<float>(MatchEndWorldSeconds - GetWorld()->GetTimeSeconds()));
    AbyssGameState->SetMatchTimeRemaining(RemainingSeconds);
    if (RemainingSeconds > KINDA_SMALL_NUMBER)
    {
        return;
    }

    AbyssGameState->SetMatchResult(EAbyssTeam::Saboteur, TEXT("timer_expired"));
    StopMatchTimer();

    UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=saboteur reason=timer_expired"));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(TEXT("match_timer_expired"), TEXT("{\"winner\":\"saboteur\",\"reason\":\"timer_expired\"}"));
            TelemetrySubsystem->LogEvent(TEXT("match_ended"), AppendMatchConfigTelemetry(TEXT("{\"winner\":\"saboteur\",\"reason\":\"timer_expired\"}")));
        }
    }
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

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    if (!AbyssGameState)
    {
        return;
    }

    // The route now advances over time while underway (see TickVoyage); for the smoke we drive it
    // straight to completion, flip to FinalApproach, and resolve the crew win.
    const float Progress = AbyssGameState->AddRouteProgress(1.0f);
    if (AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress)
    {
        AbyssGameState->SetMatchPhase(EAbyssMatchPhase::FinalApproach);
    }
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(TEXT("route_progress"), FString::Printf(TEXT("{\"progress\":%.3f,\"source\":\"dev_smoke\"}"), Progress));
            TelemetrySubsystem->LogEvent(TEXT("final_approach_started"), TEXT("{\"progress\":1.0,\"source\":\"dev_smoke\"}"));
        }
    }

    EvaluateMatchEnd();

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_route_complete progress=%.2f phase=%d"),
        AbyssGameState->GetRouteProgress(),
        static_cast<int32>(AbyssGameState->GetMatchPhase()));
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

void AAbyssLockGameMode::RunDevSmokeLifeAction()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    const AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    const bool bStartedInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;

    AAbyssLockCharacter* TargetCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Crew));
    AAbyssLockCharacter* InstigatorCharacter = Cast<AAbyssLockCharacter>(FindPawnForTeam(EAbyssTeam::Saboteur));
    if (!InstigatorCharacter || InstigatorCharacter == TargetCharacter)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PlayerController = It->Get();
            AAbyssLockCharacter* Candidate = PlayerController ? Cast<AAbyssLockCharacter>(PlayerController->GetPawn()) : nullptr;
            if (Candidate && Candidate != TargetCharacter)
            {
                InstigatorCharacter = Candidate;
                break;
            }
        }
    }

    AAbyssLifeActionActor* LifeActionActor = nullptr;
    if (TargetCharacter)
    {
        LifeActionActor = GetWorld()->SpawnActor<AAbyssLifeActionActor>(
            AAbyssLifeActionActor::StaticClass(),
            TargetCharacter->GetActorLocation() + FVector(120.0f, 0.0f, 40.0f),
            FRotator::ZeroRotator);
    }

    const bool bDamaged = TargetCharacter ? TargetCharacter->ApplyServerDamage(999.0f, InstigatorCharacter) : false;
    bool bCanRescue = false;
    bool bRescued = false;
    bool bCanContain = false;
    bool bContained = false;
    bool bCanRelease = false;
    bool bReleased = false;

    if (LifeActionActor && TargetCharacter && InstigatorCharacter)
    {
        LifeActionActor->ConfigureLifeAction(TargetCharacter, EAbyssLifeAction::RescueDowned);
        bCanRescue = LifeActionActor->CanInteract(InstigatorCharacter);
        bRescued = LifeActionActor->Interact(InstigatorCharacter);

        LifeActionActor->ConfigureLifeAction(TargetCharacter, EAbyssLifeAction::ContainAlive);
        bCanContain = LifeActionActor->CanInteract(InstigatorCharacter);
        bContained = LifeActionActor->Interact(InstigatorCharacter);

        LifeActionActor->ConfigureLifeAction(TargetCharacter, EAbyssLifeAction::ReleaseContained);
        bCanRelease = LifeActionActor->CanInteract(InstigatorCharacter);
        bReleased = LifeActionActor->Interact(InstigatorCharacter);
    }

    const bool bStillInProgress = AbyssGameState && AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::InProgress;
    const bool bPassed =
        bStartedInProgress &&
        bDamaged &&
        bCanRescue &&
        bRescued &&
        bCanContain &&
        bContained &&
        bCanRelease &&
        bReleased &&
        bStillInProgress;

    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("dev_smoke_life_action result=%s startedInProgress=%s damaged=%s canRescue=%s rescued=%s canContain=%s contained=%s canRelease=%s released=%s stillInProgress=%s target=%s instigator=%s"),
        bPassed ? TEXT("pass") : TEXT("fail"),
        bStartedInProgress ? TEXT("true") : TEXT("false"),
        bDamaged ? TEXT("true") : TEXT("false"),
        bCanRescue ? TEXT("true") : TEXT("false"),
        bRescued ? TEXT("true") : TEXT("false"),
        bCanContain ? TEXT("true") : TEXT("false"),
        bContained ? TEXT("true") : TEXT("false"),
        bCanRelease ? TEXT("true") : TEXT("false"),
        bReleased ? TEXT("true") : TEXT("false"),
        bStillInProgress ? TEXT("true") : TEXT("false"),
        *GetNameSafe(TargetCharacter),
        *GetNameSafe(InstigatorCharacter));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("dev_smoke_life_action"),
                FString::Printf(
                    TEXT("{\"result\":\"%s\",\"startedInProgress\":%s,\"damaged\":%s,\"canRescue\":%s,\"rescued\":%s,\"canContain\":%s,\"contained\":%s,\"canRelease\":%s,\"released\":%s,\"stillInProgress\":%s,\"target\":\"%s\",\"instigator\":\"%s\"}"),
                    bPassed ? TEXT("pass") : TEXT("fail"),
                    bStartedInProgress ? TEXT("true") : TEXT("false"),
                    bDamaged ? TEXT("true") : TEXT("false"),
                    bCanRescue ? TEXT("true") : TEXT("false"),
                    bRescued ? TEXT("true") : TEXT("false"),
                    bCanContain ? TEXT("true") : TEXT("false"),
                    bContained ? TEXT("true") : TEXT("false"),
                    bCanRelease ? TEXT("true") : TEXT("false"),
                    bReleased ? TEXT("true") : TEXT("false"),
                    bStillInProgress ? TEXT("true") : TEXT("false"),
                    *GetNameSafe(TargetCharacter),
                    *GetNameSafe(InstigatorCharacter)));
        }
    }

    if (LifeActionActor)
    {
        LifeActionActor->Destroy();
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
    int32 TotalCrew = 0;
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
        // Only true Crew count toward survival. Saboteurs and the Madman are antagonist-aligned
        // (the Madman wins iff the Saboteurs win), so they are intentionally excluded here.
        if (Team != EAbyssTeam::Crew)
        {
            continue;
        }

        ++TotalCrew;
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
        const FString MatchEndPayload = BuildFatalShipStatePayload(*AbyssGameState);
        AbyssGameState->SetMatchResult(EAbyssTeam::Saboteur, TEXT("fatal_ship_state"));
        StopMatchTimer();
        UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=saboteur reason=fatal_ship_state payload=%s"), *MatchEndPayload);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(TEXT("match_ended"), AppendMatchConfigTelemetry(MatchEndPayload));
            }
        }
        return true;
    }

    if (TotalAssignedPlayers >= 5 && LivingCrew <= GetSaboteurEliminationThreshold(TotalAssignedPlayers))
    {
        AbyssGameState->SetMatchResult(EAbyssTeam::Saboteur, TEXT("crew_threshold"));
        StopMatchTimer();
        UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=saboteur reason=crew_threshold livingCrew=%d"), LivingCrew);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("match_ended"),
                    AppendMatchConfigTelemetry(FString::Printf(TEXT("{\"winner\":\"saboteur\",\"reason\":\"crew_threshold\",\"livingCrew\":%d}"), LivingCrew)));
            }
        }
        return true;
    }

    // No crew left able to act (downed/dead/contained with nobody to revive — e.g. a solo player who
    // starved or froze). The voyage cannot be completed, so the crew loses immediately.
    if (TotalCrew >= 1 && ActiveCrew == 0)
    {
        AbyssGameState->SetMatchResult(EAbyssTeam::Saboteur, TEXT("crew_incapacitated"));
        StopMatchTimer();
        UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=saboteur reason=crew_incapacitated livingCrew=%d totalCrew=%d"), LivingCrew, TotalCrew);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("match_ended"),
                    AppendMatchConfigTelemetry(FString::Printf(TEXT("{\"winner\":\"saboteur\",\"reason\":\"crew_incapacitated\",\"livingCrew\":%d}"), LivingCrew)));
            }
        }
        return true;
    }

    if (AbyssGameState->GetMatchPhase() == EAbyssMatchPhase::FinalApproach && ActiveCrew >= GetRequiredCrewSurvivors(TotalAssignedPlayers))
    {
        AbyssGameState->SetMatchResult(EAbyssTeam::Crew, TEXT("final_approach_complete"));
        StopMatchTimer();
        UE_LOG(LogAbyssGameplay, Log, TEXT("match_end winner=crew reason=final_approach_complete activeCrew=%d"), ActiveCrew);
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("match_ended"),
                    AppendMatchConfigTelemetry(FString::Printf(TEXT("{\"winner\":\"crew\",\"reason\":\"final_approach_complete\",\"activeCrew\":%d}"), ActiveCrew)));
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

    const bool bSinglePlayerMode = IsSinglePlayerModeEnabled();
    int32 RequiredPlayers = bSinglePlayerMode ? AbyssPracticePlayers : AbyssFixedMatchPlayers;
#if !UE_BUILD_SHIPPING
    if (!bSinglePlayerMode)
    {
        FParse::Value(FCommandLine::Get(), TEXT("AbyssLobbyMinPlayers="), RequiredPlayers);
    }
#endif
    const int32 ClampedRequiredPlayers = FMath::Clamp(RequiredPlayers, 1, AbyssFixedMatchPlayers);

    if (PlayerCount != ClampedRequiredPlayers || ReadyCount != PlayerCount)
    {
        UE_LOG(LogAbyssGameplay, Log, TEXT("lobby_start_waiting players=%d ready=%d required=%d"), PlayerCount, ReadyCount, ClampedRequiredPlayers);
        return false;
    }

    int32 SaboteurOverride = bSinglePlayerMode ? 0 : INDEX_NONE;
#if !UE_BUILD_SHIPPING
    if (!bSinglePlayerMode)
    {
        FParse::Value(FCommandLine::Get(), TEXT("AbyssLobbyDevSaboteurs="), SaboteurOverride);
    }
#endif

    AssignRolesForCurrentPlayersInternal(SaboteurOverride, bSinglePlayerMode ? 0 : ActiveMatchConfig.MadmanCount);
    AbyssGameState->SetMatchPhase(EAbyssMatchPhase::InProgress);
    StartMatchTimer();
    const FString StartSource = bSinglePlayerMode ? TEXT("single_player") : TEXT("ready_lobby");
    UE_LOG(LogAbyssGameplay, Log, TEXT("match_started source=%s players=%d ready=%d required=%d"), *StartSource, PlayerCount, ReadyCount, ClampedRequiredPlayers);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("match_started"),
                AppendMatchConfigTelemetry(FString::Printf(TEXT("{\"source\":\"%s\",\"players\":%d,\"ready\":%d,\"requiredPlayers\":%d}"), *StartSource, PlayerCount, ReadyCount, ClampedRequiredPlayers)));
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
    if (FParse::Param(FCommandLine::Get(), TEXT("AbyssSmokeLifeAction")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AAbyssLockGameMode::RunDevSmokeLifeAction);
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

bool AAbyssLockGameMode::TryStartMatchFromHost(APlayerController* RequestingPlayer)
{
    if (!HasAuthority() || !GameState || !RequestingPlayer)
    {
        return false;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    if (!AbyssGameState || AbyssGameState->GetMatchPhase() != EAbyssMatchPhase::WaitingForPlayers)
    {
        return false;
    }

    const bool bIsHostPlayer =
        GameState->PlayerArray.Num() > 0 &&
        RequestingPlayer->PlayerState &&
        GameState->PlayerArray[0] == RequestingPlayer->PlayerState;
    if (!bIsHostPlayer)
    {
        UE_LOG(LogAbyssGameplay, Log, TEXT("lobby_host_start_rejected reason=not_host requester=%s"), *GetNameSafe(RequestingPlayer));
        return false;
    }

    const int32 PlayerCount = GameState->PlayerArray.Num();
    if (PlayerCount != AbyssFixedMatchPlayers)
    {
        UE_LOG(LogAbyssGameplay, Log, TEXT("lobby_host_start_waiting players=%d required=%d"), PlayerCount, AbyssFixedMatchPlayers);
        return false;
    }

    AssignRolesForCurrentPlayersInternal(INDEX_NONE, ActiveMatchConfig.MadmanCount);
    AbyssGameState->SetMatchPhase(EAbyssMatchPhase::InProgress);
    StartMatchTimer();

    UE_LOG(LogAbyssGameplay, Log, TEXT("match_started source=host_start players=%d host=%s mode=%s"), PlayerCount, *GetNameSafe(RequestingPlayer->PlayerState), *AbyssMatchModeToString(ActiveMatchConfig.Mode));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("match_started"),
                AppendMatchConfigTelemetry(FString::Printf(TEXT("{\"source\":\"host_start\",\"players\":%d,\"host\":\"%s\"}"), PlayerCount, *GetNameSafe(RequestingPlayer->PlayerState))));
        }
    }

    return true;
}

bool AAbyssLockGameMode::TryStartSoloMatchFromMenu(APlayerController* RequestingPlayer)
{
    if (!HasAuthority() || !GameState || !RequestingPlayer)
    {
        return false;
    }

    AAbyssLockGameState* AbyssGameState = GetGameState<AAbyssLockGameState>();
    if (!AbyssGameState || AbyssGameState->GetMatchPhase() != EAbyssMatchPhase::WaitingForPlayers)
    {
        return false;
    }

    const int32 PlayerCount = GameState->PlayerArray.Num();
    if (PlayerCount != AbyssPracticePlayers)
    {
        UE_LOG(LogAbyssGameplay, Log, TEXT("solo_start_rejected players=%d required=%d requester=%s"), PlayerCount, AbyssPracticePlayers, *GetNameSafe(RequestingPlayer));
        return false;
    }

    AssignRolesForCurrentPlayersInternal(0);
    AbyssGameState->SetMatchPhase(EAbyssMatchPhase::InProgress);
    StartMatchTimer();

    UE_LOG(LogAbyssGameplay, Log, TEXT("match_started source=solo_menu players=%d player=%s"), PlayerCount, *GetNameSafe(RequestingPlayer->PlayerState));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("match_started"),
                AppendMatchConfigTelemetry(FString::Printf(TEXT("{\"source\":\"solo_menu\",\"players\":%d,\"player\":\"%s\"}"), PlayerCount, *GetNameSafe(RequestingPlayer->PlayerState))));
        }
    }

    return true;
}

bool AAbyssLockGameMode::TryStartPracticeMatch()
{
    return TryStartSinglePlayerMatch();
}

bool AAbyssLockGameMode::TryStartSinglePlayerMatch()
{
    if (!HasAuthority() || !IsSinglePlayerModeEnabled() || !GameState)
    {
        return false;
    }

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        if (AAbyssLockPlayerState* AbyssPlayerState = Cast<AAbyssLockPlayerState>(PlayerState))
        {
            AbyssPlayerState->SetReadyForMatch(true);
        }
    }

    return TryStartMatchFromReady();
}

bool AAbyssLockGameMode::IsPracticeModeEnabled() const
{
    return FParse::Param(FCommandLine::Get(), TEXT("AbyssPracticeMode"));
}

bool AAbyssLockGameMode::IsSinglePlayerModeEnabled() const
{
    return bSoloUrlRequested || FParse::Param(FCommandLine::Get(), TEXT("AbyssSinglePlayer")) || IsPracticeModeEnabled();
}

FString AAbyssLockGameMode::AppendMatchConfigTelemetry(const FString& JsonObject) const
{
    if (!JsonObject.EndsWith(TEXT("}")))
    {
        return JsonObject;
    }

    const FString Fields = FString::Printf(
        TEXT("\"mode\":\"%s\",\"difficulty\":\"%s\""),
        *AbyssMatchModeToString(ActiveMatchConfig.Mode),
        *AbyssDifficultyToString(ActiveMatchConfig.Difficulty));

    const FString Body = JsonObject.LeftChop(1); // everything before the closing brace
    const bool bEmptyObject = Body.TrimEnd().EndsWith(TEXT("{"));
    return bEmptyObject
        ? FString::Printf(TEXT("%s%s}"), *Body, *Fields)
        : FString::Printf(TEXT("%s,%s}"), *Body, *Fields);
}

void AAbyssLockGameMode::TickVoyage(AAbyssLockGameState& AbyssGameState)
{
    // Only advance while underway (InProgress). FinalApproach/MatchEnded do not consume fuel or progress.
    if (AbyssGameState.GetMatchPhase() != EAbyssMatchPhase::InProgress)
    {
        return;
    }

    UAbyssTelemetrySubsystem* TelemetrySubsystem = nullptr;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>();
    }

    FAbyssShipSystemStatus FuelStatus;
    if (!AbyssGameState.GetShipSystemStatus(EAbyssShipSystem::Fuel, FuelStatus) || FuelStatus.Condition <= 0.0f)
    {
        // Out of fuel: the ship is stalled, so the route does not advance (the clock keeps running).
        if (!bVoyageStalledLogged)
        {
            bVoyageStalledLogged = true;
            UE_LOG(LogAbyssGameplay, Log, TEXT("ship_stalled reason=out_of_fuel progress=%.2f"), AbyssGameState.GetRouteProgress());
            if (TelemetrySubsystem)
            {
                TelemetrySubsystem->LogEvent(TEXT("ship_stalled"), FString::Printf(TEXT("{\"reason\":\"out_of_fuel\",\"progress\":%.3f}"), AbyssGameState.GetRouteProgress()));
            }
        }
        return;
    }
    bVoyageStalledLogged = false; // re-armed once the crew refuels

    // Underway: burn fuel (scaled by difficulty) and advance the voyage by one second's worth of progress.
    const float Burn = AbyssVoyageFuelBurnPerSecond * FMath::Max(0.0f, ActiveMatchConfig.FuelBurnMultiplier);
    const float NewFuel = FMath::Clamp(FuelStatus.Condition - Burn, 0.0f, 1.0f);
    AbyssGameState.SetShipSystemStatus(EAbyssShipSystem::Fuel, NewFuel, FuelStatus.bOffline, FuelStatus.bSabotaged);

    const float NewProgress = AbyssGameState.AddRouteProgress(AbyssVoyageRouteProgressPerSecond);

    // Observable progress telemetry: always log the first underway tick, then throttle to every ~1%,
    // so even a short headless run shows the voyage advancing over time and burning fuel.
    if (TelemetrySubsystem && (LastLoggedRouteProgress <= 0.0f || NewProgress - LastLoggedRouteProgress >= 0.01f))
    {
        LastLoggedRouteProgress = NewProgress;
        TelemetrySubsystem->LogEvent(TEXT("voyage_progress"), FString::Printf(TEXT("{\"progress\":%.3f,\"fuel\":%.3f}"), NewProgress, NewFuel));
    }

    if (NewProgress >= 1.0f)
    {
        AbyssGameState.SetMatchPhase(EAbyssMatchPhase::FinalApproach);
        UE_LOG(LogAbyssGameplay, Log, TEXT("final_approach_started progress=%.2f source=voyage"), NewProgress);
        if (TelemetrySubsystem)
        {
            TelemetrySubsystem->LogEvent(TEXT("final_approach_started"), TEXT("{\"progress\":1.0,\"source\":\"voyage\"}"));
        }
    }
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
    if (ActiveMatchConfig.RequiredCrewSurvivorsOverride >= 0)
    {
        return FMath::Clamp(ActiveMatchConfig.RequiredCrewSurvivorsOverride, 1, FMath::Max(1, PlayerCount));
    }

    if (PlayerCount <= AbyssPracticePlayers)
    {
        return 1;
    }

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
