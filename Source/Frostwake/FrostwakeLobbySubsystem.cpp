#include "FrostwakeLobbySubsystem.h"

#include "Internationalization/Regex.h"

namespace
{
constexpr int32 FixedMatchPlayers = 8;
constexpr int32 StandardMinimumCompletedMatches = 50;

bool IsBlank(const FString& Value)
{
    return Value.TrimStartAndEnd().IsEmpty();
}

bool IsLobbyTypeValid(const FString& Value)
{
    return Value == TEXT("casual") || Value == TEXT("standard");
}

bool IsRulesetValid(const FString& Value)
{
    return Value == TEXT("standard") || Value == TEXT("private_test") || Value == TEXT("developer");
}

bool IsJoinStateValid(const FString& Value)
{
    return Value == TEXT("open") || Value == TEXT("locked") || Value == TEXT("in_match") || Value == TEXT("full");
}

bool IsConnectionModeValid(const FString& Value)
{
    return Value == TEXT("listen") || Value == TEXT("dedicated");
}

bool LooksLikeRawIpEndpoint(const FString& Value)
{
    const FRegexPattern RawEndpointPattern(TEXT("\\b\\d{1,3}(\\.\\d{1,3}){3}:\\d{2,5}\\b"));
    FRegexMatcher Matcher(RawEndpointPattern, Value);
    return Matcher.FindNext();
}

void ReadStringValue(const TMap<FString, FString>& Values, const TCHAR* Key, FString& OutValue)
{
    if (const FString* Value = Values.Find(Key))
    {
        OutValue = *Value;
    }
}

void ReadIntValue(const TMap<FString, FString>& Values, const TCHAR* Key, int32& OutValue)
{
    if (const FString* Value = Values.Find(Key))
    {
        OutValue = FCString::Atoi(**Value);
    }
}

void ReadBoolValue(const TMap<FString, FString>& Values, const TCHAR* Key, bool& OutValue)
{
    if (const FString* Value = Values.Find(Key))
    {
        OutValue = Value->Equals(TEXT("true"), ESearchCase::IgnoreCase) || *Value == TEXT("1");
    }
}

bool HasInvalidDomainRules(const FFrostwakeLobbyMetadata& Metadata)
{
    if (Metadata.SchemaVersion != 1)
    {
        return true;
    }
    if (IsBlank(Metadata.LobbyName) || IsBlank(Metadata.BuildId) || IsBlank(Metadata.MapId))
    {
        return true;
    }
    if (!IsLobbyTypeValid(Metadata.LobbyType) || !IsRulesetValid(Metadata.Ruleset))
    {
        return true;
    }
    if (Metadata.MaxPlayers != FixedMatchPlayers || Metadata.CurrentPlayers < 0 || Metadata.CurrentPlayers > Metadata.MaxPlayers)
    {
        return true;
    }
    if (Metadata.MinimumCompletedMatches < 0)
    {
        return true;
    }
    if (Metadata.LobbyType == TEXT("casual") && Metadata.MinimumCompletedMatches != 0)
    {
        return true;
    }
    if (Metadata.LobbyType == TEXT("standard") && Metadata.MinimumCompletedMatches < StandardMinimumCompletedMatches)
    {
        return true;
    }
    if (!IsJoinStateValid(Metadata.JoinState) || !IsConnectionModeValid(Metadata.ConnectionMode))
    {
        return true;
    }
    if (Metadata.JoinState == TEXT("open") && Metadata.CurrentPlayers >= Metadata.MaxPlayers)
    {
        return true;
    }
    if (Metadata.JoinState == TEXT("full") && Metadata.CurrentPlayers < Metadata.MaxPlayers)
    {
        return true;
    }
    if (LooksLikeRawIpEndpoint(Metadata.EndpointToken))
    {
        return true;
    }

    return false;
}

FFrostwakeLobbyJoinDecision MakeDecision(
    const FFrostwakeLobbyMetadata& Metadata,
    EFrostwakeLobbyRejectReason Reason,
    const TCHAR* Message)
{
    FFrostwakeLobbyJoinDecision Decision;
    Decision.RejectReason = Reason;
    Decision.RejectReasonCode = UFrostwakeLobbySubsystem::RejectReasonToString(Reason);
    Decision.Message = Message;
    Decision.bEndpointAvailable = !IsBlank(Metadata.EndpointToken);
    Decision.bAccepted = Reason == EFrostwakeLobbyRejectReason::None;
    Decision.bTravelAllowed = Decision.bAccepted;
    return Decision;
}
} // namespace

FFrostwakeLobbyJoinDecision UFrostwakeLobbySubsystem::EvaluateJoinMetadata(
    const FFrostwakeLobbyMetadata& Metadata,
    const FString& ExpectedBuildId,
    const FString& ExpectedMapId) const
{
    if (HasInvalidDomainRules(Metadata))
    {
        return MakeDecision(
            Metadata,
            EFrostwakeLobbyRejectReason::InvalidMetadata,
            TEXT("Lobby metadata is invalid; client travel is blocked."));
    }

    if (!ExpectedBuildId.IsEmpty() && Metadata.BuildId != ExpectedBuildId)
    {
        return MakeDecision(
            Metadata,
            EFrostwakeLobbyRejectReason::BuildMismatch,
            TEXT("Lobby build id does not match the local client build; client travel is blocked."));
    }

    if (!ExpectedMapId.IsEmpty() && Metadata.MapId != ExpectedMapId)
    {
        return MakeDecision(
            Metadata,
            EFrostwakeLobbyRejectReason::MapMismatch,
            TEXT("Lobby map id does not match the local client map; client travel is blocked."));
    }

    if (Metadata.JoinState == TEXT("full"))
    {
        return MakeDecision(
            Metadata,
            EFrostwakeLobbyRejectReason::LobbyFull,
            TEXT("Lobby is full; client travel is blocked."));
    }

    if (Metadata.JoinState == TEXT("locked"))
    {
        return MakeDecision(
            Metadata,
            EFrostwakeLobbyRejectReason::LobbyLocked,
            TEXT("Lobby is locked; client travel is blocked."));
    }

    if (Metadata.JoinState == TEXT("in_match"))
    {
        return MakeDecision(
            Metadata,
            EFrostwakeLobbyRejectReason::AlreadyInMatch,
            TEXT("Lobby is already in match; client travel is blocked."));
    }

    if (IsBlank(Metadata.EndpointToken))
    {
        return MakeDecision(
            Metadata,
            EFrostwakeLobbyRejectReason::EndpointUnavailable,
            TEXT("Lobby does not expose an endpoint token; client travel is blocked."));
    }

    return MakeDecision(
        Metadata,
        EFrostwakeLobbyRejectReason::None,
        TEXT("Lobby metadata is compatible; client travel may continue through the active provider."));
}

bool UFrostwakeLobbySubsystem::IsSteamLobbyRuntimeAvailable() const
{
    return false;
}

TMap<FString, FString> UFrostwakeLobbySubsystem::ToKeyValueMetadata(const FFrostwakeLobbyMetadata& Metadata)
{
    TMap<FString, FString> Values;
    Values.Add(TEXT("schemaVersion"), FString::FromInt(Metadata.SchemaVersion));
    Values.Add(TEXT("lobbyName"), Metadata.LobbyName);
    Values.Add(TEXT("lobbyType"), Metadata.LobbyType);
    Values.Add(TEXT("buildId"), Metadata.BuildId);
    Values.Add(TEXT("mapId"), Metadata.MapId);
    Values.Add(TEXT("ruleset"), Metadata.Ruleset);
    Values.Add(TEXT("mode"), Metadata.Mode);
    Values.Add(TEXT("difficulty"), Metadata.Difficulty);
    Values.Add(TEXT("maxPlayers"), FString::FromInt(Metadata.MaxPlayers));
    Values.Add(TEXT("currentPlayers"), FString::FromInt(Metadata.CurrentPlayers));
    Values.Add(TEXT("minimumCompletedMatches"), FString::FromInt(Metadata.MinimumCompletedMatches));
    Values.Add(TEXT("joinState"), Metadata.JoinState);
    Values.Add(TEXT("connectionMode"), Metadata.ConnectionMode);
    Values.Add(TEXT("official"), Metadata.bOfficial ? TEXT("true") : TEXT("false"));
    Values.Add(TEXT("passworded"), Metadata.bPassworded ? TEXT("true") : TEXT("false"));
    Values.Add(TEXT("endpointToken"), Metadata.EndpointToken);

    if (!Metadata.Region.IsEmpty())
    {
        Values.Add(TEXT("region"), Metadata.Region);
    }
    if (!Metadata.ServerName.IsEmpty())
    {
        Values.Add(TEXT("serverName"), Metadata.ServerName);
    }

    return Values;
}

FFrostwakeLobbyMetadata UFrostwakeLobbySubsystem::FromKeyValueMetadata(const TMap<FString, FString>& Values)
{
    FFrostwakeLobbyMetadata Metadata;
    ReadIntValue(Values, TEXT("schemaVersion"), Metadata.SchemaVersion);
    ReadStringValue(Values, TEXT("lobbyName"), Metadata.LobbyName);
    ReadStringValue(Values, TEXT("lobbyType"), Metadata.LobbyType);
    ReadStringValue(Values, TEXT("buildId"), Metadata.BuildId);
    ReadStringValue(Values, TEXT("mapId"), Metadata.MapId);
    ReadStringValue(Values, TEXT("ruleset"), Metadata.Ruleset);
    ReadStringValue(Values, TEXT("mode"), Metadata.Mode);
    ReadStringValue(Values, TEXT("difficulty"), Metadata.Difficulty);
    ReadIntValue(Values, TEXT("maxPlayers"), Metadata.MaxPlayers);
    ReadIntValue(Values, TEXT("currentPlayers"), Metadata.CurrentPlayers);
    ReadIntValue(Values, TEXT("minimumCompletedMatches"), Metadata.MinimumCompletedMatches);
    ReadStringValue(Values, TEXT("joinState"), Metadata.JoinState);
    ReadStringValue(Values, TEXT("connectionMode"), Metadata.ConnectionMode);
    ReadBoolValue(Values, TEXT("official"), Metadata.bOfficial);
    ReadBoolValue(Values, TEXT("passworded"), Metadata.bPassworded);
    ReadStringValue(Values, TEXT("endpointToken"), Metadata.EndpointToken);
    ReadStringValue(Values, TEXT("region"), Metadata.Region);
    ReadStringValue(Values, TEXT("serverName"), Metadata.ServerName);
    return Metadata;
}

FString UFrostwakeLobbySubsystem::RejectReasonToString(EFrostwakeLobbyRejectReason Reason)
{
    switch (Reason)
    {
        case EFrostwakeLobbyRejectReason::None:
            return TEXT("None");
        case EFrostwakeLobbyRejectReason::SteamUnavailable:
            return TEXT("SteamUnavailable");
        case EFrostwakeLobbyRejectReason::InvalidMetadata:
            return TEXT("InvalidMetadata");
        case EFrostwakeLobbyRejectReason::BuildMismatch:
            return TEXT("BuildMismatch");
        case EFrostwakeLobbyRejectReason::MapMismatch:
            return TEXT("MapMismatch");
        case EFrostwakeLobbyRejectReason::LobbyFull:
            return TEXT("LobbyFull");
        case EFrostwakeLobbyRejectReason::LobbyLocked:
            return TEXT("LobbyLocked");
        case EFrostwakeLobbyRejectReason::AlreadyInMatch:
            return TEXT("AlreadyInMatch");
        case EFrostwakeLobbyRejectReason::EndpointUnavailable:
            return TEXT("EndpointUnavailable");
        case EFrostwakeLobbyRejectReason::TravelFailed:
            return TEXT("TravelFailed");
        default:
            return TEXT("InvalidMetadata");
    }
}
