#include "FrostwakePveEnemyActor.h"
#include "FrostwakeCharacter.h"
#include "FrostwakeGameState.h"
#include "FrostwakeGameplayTags.h"
#include "FrostwakeLog.h"
#include "FrostwakePlayerState.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AFrostwakePveEnemyActor::AFrostwakePveEnemyActor()
{
    bReplicates = true;
    SetReplicateMovement(true);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    MarkerMesh->SetupAttachment(SceneRoot);
    MarkerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
    MarkerMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.2f));
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MarkerMesh->SetCollisionObjectType(ECC_WorldDynamic);
    MarkerMesh->SetCollisionResponseToAllChannels(ECR_Block);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MarkerMesh->SetStaticMesh(CubeMesh.Object);
    }

    DamageAmount = 100.0f;
    AttackCount = 0;
}

void AFrostwakePveEnemyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFrostwakePveEnemyActor, DamageAmount);
    DOREPLIFETIME(AFrostwakePveEnemyActor, AttackCount);
}

void AFrostwakePveEnemyActor::ConfigureEnemy(float InDamageAmount)
{
    if (HasAuthority())
    {
        DamageAmount = FMath::Max(0.0f, InDamageAmount);
    }
}

bool AFrostwakePveEnemyActor::AttackCharacter(AFrostwakeCharacter* TargetCharacter)
{
    if (!HasAuthority() || !TargetCharacter || DamageAmount <= 0.0f)
    {
        return false;
    }

    const AFrostwakeGameState* FrostwakeGameState = GetWorld() ? GetWorld()->GetGameState<AFrostwakeGameState>() : nullptr;
    if (!FrostwakeGameState || !FrostwakeGameState->IsMatchActive())
    {
        return false;
    }

    const bool bAppliedDamage = TargetCharacter->ApplyServerDamage(DamageAmount, FrostwakeTags::Damage_Slashing, this);
    if (!bAppliedDamage)
    {
        return false;
    }

    ++AttackCount;
    const AFrostwakePlayerState* TargetPlayerState = TargetCharacter->GetPlayerState<AFrostwakePlayerState>();
    UE_LOG(
        LogFrostwakeGameplay,
        Log,
        TEXT("pve_enemy_attack enemy=%s target=%s damage=%.1f health=%.1f attackCount=%d"),
        *GetNameSafe(this),
        *GetNameSafe(TargetPlayerState),
        DamageAmount,
        TargetCharacter->GetHealth(),
        AttackCount);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("pve_enemy_attack"),
                FString::Printf(
                    TEXT("{\"enemy\":\"%s\",\"target\":\"%s\",\"damage\":%.1f,\"health\":%.1f,\"attackCount\":%d}"),
                    *GetNameSafe(this),
                    *GetNameSafe(TargetPlayerState),
                    DamageAmount,
                    TargetCharacter->GetHealth(),
                    AttackCount));
            TelemetrySubsystem->LogEvent(
                TEXT("pve_damage_applied"),
                FString::Printf(
                    TEXT("{\"enemy\":\"%s\",\"target\":\"%s\",\"damage\":%.1f,\"health\":%.1f,\"attackCount\":%d}"),
                    *GetNameSafe(this),
                    *GetNameSafe(TargetPlayerState),
                    DamageAmount,
                    TargetCharacter->GetHealth(),
                    AttackCount));
        }
    }

    return true;
}
