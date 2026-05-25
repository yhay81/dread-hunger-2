#include "AbyssPveEnemyActor.h"
#include "AbyssLockCharacter.h"
#include "AbyssLockGameState.h"
#include "AbyssLockLog.h"
#include "AbyssLockPlayerState.h"
#include "AbyssTelemetrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAbyssPveEnemyActor::AAbyssPveEnemyActor()
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

void AAbyssPveEnemyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssPveEnemyActor, DamageAmount);
    DOREPLIFETIME(AAbyssPveEnemyActor, AttackCount);
}

void AAbyssPveEnemyActor::ConfigureEnemy(float InDamageAmount)
{
    if (HasAuthority())
    {
        DamageAmount = FMath::Max(0.0f, InDamageAmount);
    }
}

bool AAbyssPveEnemyActor::AttackCharacter(AAbyssLockCharacter* TargetCharacter)
{
    if (!HasAuthority() || !TargetCharacter || DamageAmount <= 0.0f)
    {
        return false;
    }

    const AAbyssLockGameState* AbyssGameState = GetWorld() ? GetWorld()->GetGameState<AAbyssLockGameState>() : nullptr;
    if (!AbyssGameState || !AbyssGameState->IsMatchActive())
    {
        return false;
    }

    const bool bAppliedDamage = TargetCharacter->ApplyServerDamage(DamageAmount, this);
    if (!bAppliedDamage)
    {
        return false;
    }

    ++AttackCount;
    const AAbyssLockPlayerState* TargetPlayerState = TargetCharacter->GetPlayerState<AAbyssLockPlayerState>();
    UE_LOG(
        LogAbyssGameplay,
        Log,
        TEXT("pve_enemy_attack enemy=%s target=%s damage=%.1f health=%.1f attackCount=%d"),
        *GetNameSafe(this),
        *GetNameSafe(TargetPlayerState),
        DamageAmount,
        TargetCharacter->GetHealth(),
        AttackCount);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
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
