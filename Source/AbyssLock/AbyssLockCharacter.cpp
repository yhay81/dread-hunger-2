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
#include "TimerManager.h"

AAbyssLockCharacter::AAbyssLockCharacter()
{
    bReplicates = true;
    bUseControllerRotationPitch = true;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    MaxHealth = 100.0f;
    Health = MaxHealth;

    // Survival meters: start full, deplete slowly so the gauges read as moving without killing a
    // debug session quickly. Rates are tunable in defaults; health only drains once a meter is empty.
    MaxSatiation = 100.0f;
    Satiation = MaxSatiation;
    MaxWarmth = 100.0f;
    Warmth = MaxWarmth;
    SatiationDecayPerSecond = 0.20f;    // ~8.3 min from full to empty
    WarmthDecayPerSecond = 0.25f;       // ~6.7 min from full to empty
    StarvationDamagePerSecond = 1.0f;   // health drain once food hits zero
    HypothermiaDamagePerSecond = 1.0f;  // health drain once warmth hits zero
    RationSatiationRestore = 40.0f;     // one ration refills ~40% of food
    FoodItemIds = { FName(TEXT("Ration")) };

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
    DOREPLIFETIME(AAbyssLockCharacter, Satiation);
    DOREPLIFETIME(AAbyssLockCharacter, Warmth);
}

void AAbyssLockCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        // Drive survival decay on the server at a steady 1s cadence (cheap + keeps the math readable).
        GetWorldTimerManager().SetTimer(
            SurvivalTimerHandle, this, &AAbyssLockCharacter::UpdateSurvival, 1.0f, true);
    }
}

void AAbyssLockCharacter::UpdateSurvival()
{
    if (!HasAuthority())
    {
        return;
    }

    // Pause decay while the player is not actively alive (downed / contained / dead).
    const AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (AbyssPlayerState && AbyssPlayerState->GetLifeState() != EAbyssLifeState::Alive)
    {
        return;
    }

    const float DeltaSeconds = 1.0f; // matches the repeating-timer cadence above

    Satiation = FMath::Clamp(Satiation - SatiationDecayPerSecond * DeltaSeconds, 0.0f, MaxSatiation);
    Warmth = FMath::Clamp(Warmth - WarmthDecayPerSecond * DeltaSeconds, 0.0f, MaxWarmth);

    float HealthDrain = 0.0f;
    if (Satiation <= 0.0f)
    {
        HealthDrain += StarvationDamagePerSecond * DeltaSeconds;
    }
    if (Warmth <= 0.0f)
    {
        HealthDrain += HypothermiaDamagePerSecond * DeltaSeconds;
    }

    if (HealthDrain > 0.0f)
    {
        // Reuse the damage path so starvation/exposure also downs the player and emits telemetry.
        ApplyServerDamage(HealthDrain, this);
    }
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
    PlayerInputComponent->BindAction(TEXT("UseItem"), IE_Pressed, this, &AAbyssLockCharacter::UseSelectedItem);
    PlayerInputComponent->BindAction(TEXT("SelectNextItem"), IE_Pressed, this, &AAbyssLockCharacter::SelectNextItem);
    PlayerInputComponent->BindAction(TEXT("SelectPrevItem"), IE_Pressed, this, &AAbyssLockCharacter::SelectPrevItem);
}

void AAbyssLockCharacter::SelectNextItem()
{
    if (InventoryComponent)
    {
        InventoryComponent->CycleSelectedSlot(1);
    }
}

void AAbyssLockCharacter::SelectPrevItem()
{
    if (InventoryComponent)
    {
        InventoryComponent->CycleSelectedSlot(-1);
    }
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

void AAbyssLockCharacter::UseSelectedItem()
{
    if (!InventoryComponent)
    {
        return;
    }

    // SelectedSlot is local to this client; forward it so the server consumes the right slot.
    const int32 SlotIndex = InventoryComponent->GetSelectedSlot();
    if (HasAuthority())
    {
        EatRation(SlotIndex);
        return;
    }

    ServerUseSelectedItem(SlotIndex);
}

void AAbyssLockCharacter::ServerUseSelectedItem_Implementation(int32 SlotIndex)
{
    EatRation(SlotIndex);
}

bool AAbyssLockCharacter::EatRation(int32 SlotIndex)
{
    if (!HasAuthority() || !InventoryComponent)
    {
        return false;
    }

    const AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (!AbyssPlayerState || AbyssPlayerState->GetLifeState() != EAbyssLifeState::Alive)
    {
        return false;
    }

    const FName ItemId = InventoryComponent->GetItemIdAt(SlotIndex);
    if (ItemId.IsNone() || !FoodItemIds.Contains(ItemId))
    {
        return false;
    }

    FName RemovedId = NAME_None;
    if (!InventoryComponent->TryRemoveItemAt(SlotIndex, RemovedId))
    {
        return false;
    }

    const float Before = Satiation;
    Satiation = FMath::Clamp(Satiation + RationSatiationRestore, 0.0f, MaxSatiation);
    const float Restored = Satiation - Before;

    UE_LOG(LogAbyssGameplay, Log, TEXT("player_ate player=%s item=%s satiation=%.1f restored=%.1f"), *GetNameSafe(AbyssPlayerState), *RemovedId.ToString(), Satiation, Restored);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_ate"),
                FString::Printf(
                    TEXT("{\"player\":\"%s\",\"item\":\"%s\",\"satiation\":%.1f,\"restored\":%.1f}"),
                    *GetNameSafe(AbyssPlayerState), *RemovedId.ToString(), Satiation, Restored));
        }
    }

    return true;
}

bool AAbyssLockCharacter::ApplyWarmth(float WarmthAmount, AActor* HeatSource)
{
    if (!HasAuthority() || WarmthAmount <= 0.0f)
    {
        return false;
    }

    const AAbyssLockPlayerState* AbyssPlayerState = GetPlayerState<AAbyssLockPlayerState>();
    if (!AbyssPlayerState || AbyssPlayerState->GetLifeState() != EAbyssLifeState::Alive)
    {
        return false;
    }

    const float Before = Warmth;
    Warmth = FMath::Clamp(Warmth + WarmthAmount, 0.0f, MaxWarmth);
    const float Restored = Warmth - Before;
    if (Restored <= 0.0f)
    {
        return false; // already warm
    }

    UE_LOG(LogAbyssGameplay, Log, TEXT("player_warmed player=%s warmth=%.1f restored=%.1f source=%s"), *GetNameSafe(AbyssPlayerState), Warmth, Restored, *GetNameSafe(HeatSource));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAbyssTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UAbyssTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_warmed"),
                FString::Printf(
                    TEXT("{\"player\":\"%s\",\"warmth\":%.1f,\"restored\":%.1f,\"source\":\"%s\"}"),
                    *GetNameSafe(AbyssPlayerState), Warmth, Restored, *GetNameSafe(HeatSource)));
        }
    }

    return true;
}
