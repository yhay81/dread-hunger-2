#pragma once

#include "CoreMinimal.h"
#include "AbyssLockTypes.generated.h"

UENUM(BlueprintType)
enum class EAbyssMatchPhase : uint8
{
    WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
    RoleAssignment UMETA(DisplayName = "Role Assignment"),
    InProgress UMETA(DisplayName = "In Progress"),
    FinalApproach UMETA(DisplayName = "Final Approach"),
    MatchEnded UMETA(DisplayName = "Match Ended")
};

UENUM(BlueprintType)
enum class EAbyssTeam : uint8
{
    Unassigned UMETA(DisplayName = "Unassigned"),
    Crew UMETA(DisplayName = "Crew"),
    Saboteur UMETA(DisplayName = "Saboteur"),
    Spectator UMETA(DisplayName = "Spectator")
};

UENUM(BlueprintType)
enum class EAbyssLifeState : uint8
{
    Alive UMETA(DisplayName = "Alive"),
    Downed UMETA(DisplayName = "Downed"),
    Contained UMETA(DisplayName = "Contained"),
    Dead UMETA(DisplayName = "Dead"),
    Spectating UMETA(DisplayName = "Spectating")
};

UENUM(BlueprintType)
enum class EAbyssShipSystem : uint8
{
    Hull UMETA(DisplayName = "Hull"),
    Fuel UMETA(DisplayName = "Fuel"),
    Engine UMETA(DisplayName = "Engine"),
    Power UMETA(DisplayName = "Power"),
    Radio UMETA(DisplayName = "Radio"),
    Route UMETA(DisplayName = "Route"),
    Heat UMETA(DisplayName = "Heat"),
    Flooding UMETA(DisplayName = "Flooding")
};

UENUM(BlueprintType)
enum class EAbyssShipTaskMode : uint8
{
    Repair UMETA(DisplayName = "Repair"),
    Sabotage UMETA(DisplayName = "Sabotage")
};
