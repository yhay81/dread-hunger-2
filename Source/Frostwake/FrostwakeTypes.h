#pragma once

#include "CoreMinimal.h"
#include "FrostwakeTypes.generated.h"

UENUM(BlueprintType)
enum class EFrostwakeMatchPhase : uint8
{
    WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
    RoleAssignment UMETA(DisplayName = "Role Assignment"),
    InProgress UMETA(DisplayName = "In Progress"),
    FinalApproach UMETA(DisplayName = "Final Approach"),
    MatchEnded UMETA(DisplayName = "Match Ended")
};

UENUM(BlueprintType)
enum class EFrostwakeTeam : uint8
{
    Unassigned UMETA(DisplayName = "Unassigned"),
    Crew UMETA(DisplayName = "Crew"),
    Saboteur UMETA(DisplayName = "Saboteur"),
    Spectator UMETA(DisplayName = "Spectator"),
    // Madman: secretly Saboteur-aligned, but indistinguishable from Crew to everyone else.
    // Has no Saboteur abilities (sabotage gates check == Saboteur, so the Madman is excluded)
    // and shares the Saboteur win condition. Appended last to keep existing values stable.
    Madman UMETA(DisplayName = "Madman")
};

UENUM(BlueprintType)
enum class EFrostwakeLifeState : uint8
{
    Alive UMETA(DisplayName = "Alive"),
    Downed UMETA(DisplayName = "Downed"),
    Contained UMETA(DisplayName = "Contained"),
    Dead UMETA(DisplayName = "Dead"),
    Spectating UMETA(DisplayName = "Spectating")
};

UENUM(BlueprintType)
enum class EFrostwakeShipSystem : uint8
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
enum class EFrostwakeShipTaskMode : uint8
{
    Repair UMETA(DisplayName = "Repair"),
    Sabotage UMETA(DisplayName = "Sabotage")
};

// Selectable match variants. The host picks one when starting a session.
UENUM(BlueprintType)
enum class EFrostwakeMatchMode : uint8
{
    // Default: Crew vs hidden Saboteurs (8p = 6 Crew + 2 Saboteurs).
    Standard UMETA(DisplayName = "Standard"),
    // Adds one Madman who looks like Crew to everyone else (8p = 5 Crew + 2 Saboteurs + 1 Madman).
    Madman UMETA(DisplayName = "Madman")
};

// Difficulty preset. Populates the tuning fields of FFrostwakeMatchConfig.
UENUM(BlueprintType)
enum class EFrostwakeDifficulty : uint8
{
    Easy UMETA(DisplayName = "Easy"),
    Normal UMETA(DisplayName = "Normal"),
    Hard UMETA(DisplayName = "Hard")
};

// Server-authoritative configuration for a single match. The host configures this
// (mode + difficulty preset + optional knob overrides) before the match starts; the
// GameMode resolves and applies it during role assignment and win evaluation.
USTRUCT(BlueprintType)
struct FFrostwakeMatchConfig
{
    GENERATED_BODY()

    // Selected game mode. Drives faction composition (e.g. Madman adds one Madman slot).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    EFrostwakeMatchMode Mode = EFrostwakeMatchMode::Standard;

    // Difficulty preset that fills the tuning fields below.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    EFrostwakeDifficulty Difficulty = EFrostwakeDifficulty::Normal;

    // Saboteur count. -1 = auto from player count (CalculateSaboteurCount).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    int32 SaboteurCount = -1;

    // Madman count. 0 in Standard mode, 1 in Madman mode.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    int32 MadmanCount = 0;

    // Required surviving Crew for a Crew victory. -1 = auto from player count (GetRequiredCrewSurvivors).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    int32 RequiredCrewSurvivorsOverride = -1;

    // Difficulty knob: multiplies Warmth/Satiation decay rate. Consumed by survival decay (follow-up step).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    float SurvivalDecayMultiplier = 1.0f;

    // Difficulty knob: multiplies sabotage severity. Consumed by sabotage actors (follow-up step).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    float SabotageIntensityMultiplier = 1.0f;

    // Difficulty knob: multiplies the voyage fuel burn rate (higher = refuel more often / harder).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frostwake|Match")
    float FuelBurnMultiplier = 1.0f;
};
