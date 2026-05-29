#include "AbyssLockCharacter.h"
#include "AbyssInteractionComponent.h"
#include "AbyssInventoryComponent.h"
#include "AbyssItemPickupActor.h"
#include "AbyssLockLog.h"
#include "AbyssLockPlayerState.h"
#include "AbyssTelemetrySubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

AAbyssLockCharacter::AAbyssLockCharacter()
{
    bReplicates = true;
    bUseControllerRotationPitch = true;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    MaxHealth = 100.0f;
    Health = MaxHealth;

    InteractionComponent = CreateDefaultSubobject<UAbyssInteractionComponent>(TEXT("InteractionComponent"));
    InventoryComponent = CreateDefaultSubobject<UAbyssInventoryComponent>(TEXT("InventoryComponent"));

    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCameraComponent->bUsePawnControlRotation = true;

    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->bOrientRotationToMovement = false;
        MovementComponent->JumpZVelocity = 520.0f;
        MovementComponent->AirControl = 0.35f;
        MovementComponent->MaxWalkSpeed = 450.0f;
    }
}

void AAbyssLockCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssLockCharacter, Health);
}

void AAbyssLockCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAbyssLockCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AAbyssLockCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AAbyssLockCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AAbyssLockCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("PrimaryInteract"), IE_Pressed, this, &AAbyssLockCharacter::TryPrimaryInteract);
    PlayerInputComponent->BindAction(TEXT("DropItem"), IE_Pressed, this, &AAbyssLockCharacter::TryDropItem);
}

void AAbyssLockCharacter::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

bool AAbyssLockCharacter::ApplyServerDamage(float DamageAmount, AActor* DamageSource)
{
    if (!HasAuthority() || DamageAmount <= 0.0f)
    {
        return false;
    }

    AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (!AbyssPlayerState || AbyssPlayerState->GetLifeState() != EAbyssLifeState::Alive)
    {
        return false;
    }

    Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
    UE_LOG(LogAbyssGameplay, Log, TEXT("player_damaged player=%s health=%.1f damage=%.1f source=%s"), *GetNameSafe(AbyssPlayerState), Health, DamageAmount, *GetNameSafe(DamageSource));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_damaged"),
                FString::Printf(
                    TEXT("{\"player\":\"%s\",\"health\":%.1f,\"damage\":%.1f,\"source\":\"%s\"}"),
                    *GetNameSafe(AbyssPlayerState),
                    Health,
                    DamageAmount,
                    *GetNameSafe(DamageSource)));
        }
    }

    if (Health <= 0.0f)
    {
        AbyssPlayerState->SetLifeState(EAbyssLifeState::Downed);
        UE_LOG(LogAbyssGameplay, Log, TEXT("player_downed player=%s source=%s"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(DamageSource));
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("player_downed"),
                    FString::Printf(TEXT("{\"player\":\"%s\",\"source\":\"%s\"}"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(DamageSource)));
            }
        }
    }

    return true;
}

bool AAbyssLockCharacter::RescueFromDowned(APawn* RescuerPawn)
{
    if (!HasAuthority())
    {
        return false;
    }

    AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (!AbyssPlayerState || AbyssPlayerState->GetLifeState() != EAbyssLifeState::Downed)
    {
        return false;
    }

    Health = FMath::Max(MaxHealth * 0.35f, 1.0f);
    AbyssPlayerState->SetLifeState(EAbyssLifeState::Alive);
    UE_LOG(LogAbyssGameplay, Log, TEXT("player_rescued player=%s rescuer=%s health=%.1f"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(RescuerPawn), Health);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_rescued"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"rescuer\":\"%s\",\"health\":%.1f}"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(RescuerPawn), Health));
        }
    }

    return true;
}

bool AAbyssLockCharacter::ContainPlayer(APawn* InstigatorPawn)
{
    if (!HasAuthority())
    {
        return false;
    }

    AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (!AbyssPlayerState || AbyssPlayerState->GetLifeState() != EAbyssLifeState::Alive)
    {
        return false;
    }

    AbyssPlayerState->SetLifeState(EAbyssLifeState::Contained);
    UE_LOG(LogAbyssGameplay, Log, TEXT("player_contained player=%s instigator=%s"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(InstigatorPawn));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_contained"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"instigator\":\"%s\"}"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(InstigatorPawn)));
        }
    }

    return true;
}

bool AAbyssLockCharacter::ReleaseFromContainment(APawn* InstigatorPawn)
{
    if (!HasAuthority())
    {
        return false;
    }

    AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (!AbyssPlayerState || AbyssPlayerState->GetLifeState() != EAbyssLifeState::Contained)
    {
        return false;
    }

    AbyssPlayerState->SetLifeState(EAbyssLifeState::Alive);
    UE_LOG(LogAbyssGameplay, Log, TEXT("player_released player=%s instigator=%s"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(InstigatorPawn));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_released"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"instigator\":\"%s\"}"), *GetNameSafe(AbyssPlayerState), *GetNameSafe(InstigatorPawn)));
        }
    }

    return true;
}

void AAbyssLockCharacter::OnRep_Health()
{
}

void AAbyssLockCharacter::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void AAbyssLockCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AAbyssLockCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AAbyssLockCharacter::TryPrimaryInteract()
{
    if (InteractionComponent)
    {
        InteractionComponent->TryInteract();
    }
}

void AAbyssLockCharacter::TryDropItem()
{
    if (HasAuthority())
    {
        DropFirstInventoryItem();
        return;
    }

    ServerTryDropItem();
}

void AAbyssLockCharacter::ServerTryDropItem_Implementation()
{
    DropFirstInventoryItem();
}

AAbyssItemPickupActor* AAbyssLockCharacter::DropFirstInventoryItem()
{
    if (!InventoryComponent)
    {
        return nullptr;
    }

    const FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 120.0f + FVector(0.0f, 0.0f, 20.0f);
    return InventoryComponent->TryDropFirstItem(DropLocation, GetActorRotation());
}
