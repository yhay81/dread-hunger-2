#include "FrostwakeCharacter.h"
#include "FrostwakeInteractionComponent.h"
#include "FrostwakeInventoryComponent.h"
#include "FrostwakeItemPickupActor.h"
#include "FrostwakeGameMode.h"
#include "FrostwakeLog.h"
#include "FrostwakePlayerState.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "ActionSystem/FrostwakeMatchSubsystem.h"
#include "ActionSystem/FrostwakeTemperatureSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AFrostwakeCharacter::AFrostwakeCharacter()
{
    bReplicates = true;
    bUseControllerRotationPitch = true;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    // Survival meters: warmth starts full and Hunger starts at 0 (fed), both moving slowly so the gauges
    // read as changing without killing a debug session quickly. Rates are tunable in defaults; health
    // only drains once a meter bottoms out (Hunger maxes out / warmth hits zero).
    HungerIncreasePerSecond = 0.20f;    // ~8.3 min from fed to starving
    StarvationDamagePerSecond = 1.0f;   // health drain once Hunger maxes out
    HypothermiaDamagePerSecond = 1.0f;  // health drain once warmth hits zero
    RationHungerRestore = 40.0f;        // one ration removes ~40% of max hunger
    FoodItemIds = { FName(TEXT("Ration")) };

    InteractionComponent = CreateDefaultSubobject<UFrostwakeInteractionComponent>(TEXT("InteractionComponent"));
    InventoryComponent = CreateDefaultSubobject<UFrostwakeInventoryComponent>(TEXT("InventoryComponent"));
    // The Action System attribute component owns the vitals: Health + Warmth start full, Hunger starts at
    // 0 (fed). Warmth is temperature-driven (plan §3.22-23). All replicate push-model to clients.
    AttributeComponent = CreateDefaultSubobject<UFrostwakeAttributeComponent>(TEXT("AttributeComponent"));

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

void AFrostwakeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Vitals (Health/Warmth/Hunger) replicate via the AttributeComponent (push-model), not the character.
}

void AFrostwakeCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        // Drive survival decay on the server at a steady 1s cadence (cheap + keeps the math readable).
        GetWorldTimerManager().SetTimer(
            SurvivalTimerHandle, this, &AFrostwakeCharacter::UpdateSurvival, 1.0f, true);
    }
}

void AFrostwakeCharacter::UpdateSurvival()
{
    if (!HasAuthority())
    {
        return;
    }

    // Pause decay while the player is not actively alive (downed / contained / dead).
    const AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (FrostwakePlayerState && FrostwakePlayerState->GetLifeState() != EFrostwakeLifeState::Alive)
    {
        return;
    }

    const float DeltaSeconds = 1.0f; // matches the repeating-timer cadence above
    const float DecayMultiplier = GetMatchDecayMultiplier(); // difficulty preset / host override

    if (AttributeComponent)
    {
        // Hunger (DH-semantic, §3.15) rises over time; difficulty makes it rise faster.
        AttributeComponent->ModifyAttribute(EFrostwakeAttribute::Hunger, HungerIncreasePerSecond * DecayMultiplier * DeltaSeconds);

        // Warmth is driven by the shared temperature model (plan §3.22-23): warmth Δ/sec = CurrentTemperature
        // = GlobalTemperature + Σ nearby heat sources. Positive near a fire (warms), negative in the cold
        // (cools). Difficulty scales the cold half only (heating from a fire is not penalized).
        if (const UFrostwakeTemperatureSubsystem* Temperature = UFrostwakeTemperatureSubsystem::Get(this))
        {
            float Rate = Temperature->ComputeTemperatureAt(GetActorLocation());
            if (Rate < 0.0f)
            {
                Rate *= DecayMultiplier;
            }
            if (Rate != 0.0f)
            {
                AttributeComponent->ModifyAttribute(EFrostwakeAttribute::Warmth, Rate * DeltaSeconds);
            }
        }
    }

    float HealthDrain = 0.0f;
    if (AttributeComponent
        && AttributeComponent->GetAttribute(EFrostwakeAttribute::Hunger) >= AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Hunger))
    {
        HealthDrain += StarvationDamagePerSecond * DeltaSeconds; // starving (Hunger maxed)
    }
    if (AttributeComponent && AttributeComponent->GetAttribute(EFrostwakeAttribute::Warmth) <= 0.0f)
    {
        HealthDrain += HypothermiaDamagePerSecond * DeltaSeconds;
    }

    if (HealthDrain > 0.0f)
    {
        // Reuse the damage path so starvation/exposure also downs the player and emits telemetry.
        ApplyServerDamage(HealthDrain, this);
    }
}

void AFrostwakeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AFrostwakeCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AFrostwakeCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFrostwakeCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AFrostwakeCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("PrimaryInteract"), IE_Pressed, this, &AFrostwakeCharacter::TryPrimaryInteract);
    PlayerInputComponent->BindAction(TEXT("DropItem"), IE_Pressed, this, &AFrostwakeCharacter::TryDropItem);
    PlayerInputComponent->BindAction(TEXT("UseItem"), IE_Pressed, this, &AFrostwakeCharacter::UseSelectedItem);
    PlayerInputComponent->BindAction(TEXT("SelectNextItem"), IE_Pressed, this, &AFrostwakeCharacter::SelectNextItem);
    PlayerInputComponent->BindAction(TEXT("SelectPrevItem"), IE_Pressed, this, &AFrostwakeCharacter::SelectPrevItem);
}

void AFrostwakeCharacter::SelectNextItem()
{
    if (InventoryComponent)
    {
        InventoryComponent->CycleSelectedSlot(1);
    }
}

void AFrostwakeCharacter::SelectPrevItem()
{
    if (InventoryComponent)
    {
        InventoryComponent->CycleSelectedSlot(-1);
    }
}

void AFrostwakeCharacter::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

float AFrostwakeCharacter::GetMatchDecayMultiplier() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const AFrostwakeGameMode* GameMode = World->GetAuthGameMode<AFrostwakeGameMode>())
        {
            return FMath::Max(0.0f, GameMode->GetActiveMatchConfig().SurvivalDecayMultiplier);
        }
    }
    return 1.0f;
}

bool AFrostwakeCharacter::ApplyServerDamage(float DamageAmount, AActor* DamageSource)
{
    if (!HasAuthority() || DamageAmount <= 0.0f || !AttributeComponent)
    {
        return false;
    }

    AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (!FrostwakePlayerState || FrostwakePlayerState->GetLifeState() != EFrostwakeLifeState::Alive)
    {
        return false;
    }

    const float NewHealth = AttributeComponent->ModifyAttribute(EFrostwakeAttribute::Health, -DamageAmount);
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_damaged player=%s health=%.1f damage=%.1f source=%s"), *GetNameSafe(FrostwakePlayerState), NewHealth, DamageAmount, *GetNameSafe(DamageSource));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_damaged"),
                FString::Printf(
                    TEXT("{\"player\":\"%s\",\"health\":%.1f,\"damage\":%.1f,\"source\":\"%s\"}"),
                    *GetNameSafe(FrostwakePlayerState),
                    NewHealth,
                    DamageAmount,
                    *GetNameSafe(DamageSource)));
        }
    }

    if (NewHealth <= 0.0f)
    {
        FrostwakePlayerState->SetLifeState(EFrostwakeLifeState::Downed);
        // Decoupling spine (plan §9.1 step 7): notify the match hub so systems can react to a player
        // going down without hard-referencing the character.
        if (UFrostwakeMatchSubsystem* MatchSubsystem = UFrostwakeMatchSubsystem::Get(this))
        {
            MatchSubsystem->NotifyPlayerDied(GetController());
        }
        UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_downed player=%s source=%s"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(DamageSource));
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
            {
                TelemetrySubsystem->LogEvent(
                    TEXT("player_downed"),
                    FString::Printf(TEXT("{\"player\":\"%s\",\"source\":\"%s\"}"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(DamageSource)));
            }
        }
    }

    return true;
}

bool AFrostwakeCharacter::RescueFromDowned(APawn* RescuerPawn)
{
    if (!HasAuthority() || !AttributeComponent)
    {
        return false;
    }

    AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (!FrostwakePlayerState || FrostwakePlayerState->GetLifeState() != EFrostwakeLifeState::Downed)
    {
        return false;
    }

    // Revive to a fraction of max (DH revives partial; full ReservedHealth/poison model lands with §3.17).
    const float ReviveHealth = FMath::Max(AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Health) * 0.35f, 1.0f);
    AttributeComponent->SetAttribute(EFrostwakeAttribute::Health, ReviveHealth);
    FrostwakePlayerState->SetLifeState(EFrostwakeLifeState::Alive);
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_rescued player=%s rescuer=%s health=%.1f"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(RescuerPawn), ReviveHealth);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_rescued"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"rescuer\":\"%s\",\"health\":%.1f}"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(RescuerPawn), ReviveHealth));
        }
    }

    return true;
}

bool AFrostwakeCharacter::ContainPlayer(APawn* InstigatorPawn)
{
    if (!HasAuthority())
    {
        return false;
    }

    AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (!FrostwakePlayerState || FrostwakePlayerState->GetLifeState() != EFrostwakeLifeState::Alive)
    {
        return false;
    }

    FrostwakePlayerState->SetLifeState(EFrostwakeLifeState::Contained);
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_contained player=%s instigator=%s"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(InstigatorPawn));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_contained"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"instigator\":\"%s\"}"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(InstigatorPawn)));
        }
    }

    return true;
}

bool AFrostwakeCharacter::ReleaseFromContainment(APawn* InstigatorPawn)
{
    if (!HasAuthority())
    {
        return false;
    }

    AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (!FrostwakePlayerState || FrostwakePlayerState->GetLifeState() != EFrostwakeLifeState::Contained)
    {
        return false;
    }

    FrostwakePlayerState->SetLifeState(EFrostwakeLifeState::Alive);
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_released player=%s instigator=%s"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(InstigatorPawn));

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_released"),
                FString::Printf(TEXT("{\"player\":\"%s\",\"instigator\":\"%s\"}"), *GetNameSafe(FrostwakePlayerState), *GetNameSafe(InstigatorPawn)));
        }
    }

    return true;
}

void AFrostwakeCharacter::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void AFrostwakeCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AFrostwakeCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AFrostwakeCharacter::TryPrimaryInteract()
{
    if (InteractionComponent)
    {
        InteractionComponent->TryInteract();
    }
}

void AFrostwakeCharacter::TryDropItem()
{
    if (HasAuthority())
    {
        DropFirstInventoryItem();
        return;
    }

    ServerTryDropItem();
}

void AFrostwakeCharacter::ServerTryDropItem_Implementation()
{
    DropFirstInventoryItem();
}

AFrostwakeItemPickupActor* AFrostwakeCharacter::DropFirstInventoryItem()
{
    if (!InventoryComponent)
    {
        return nullptr;
    }

    const FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 120.0f + FVector(0.0f, 0.0f, 20.0f);
    return InventoryComponent->TryDropFirstItem(DropLocation, GetActorRotation());
}

void AFrostwakeCharacter::UseSelectedItem()
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

void AFrostwakeCharacter::ServerUseSelectedItem_Implementation(int32 SlotIndex)
{
    EatRation(SlotIndex);
}

bool AFrostwakeCharacter::EatRation(int32 SlotIndex)
{
    if (!HasAuthority() || !InventoryComponent || !AttributeComponent)
    {
        return false;
    }

    const AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (!FrostwakePlayerState || FrostwakePlayerState->GetLifeState() != EFrostwakeLifeState::Alive)
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

    // Eating reduces the DH-semantic Hunger attribute; report it as restored "satiation" (food remaining).
    const float Before = GetSatiation();
    AttributeComponent->ModifyAttribute(EFrostwakeAttribute::Hunger, -RationHungerRestore);
    const float Satiation = GetSatiation();
    const float Restored = Satiation - Before;

    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_ate player=%s item=%s satiation=%.1f restored=%.1f"), *GetNameSafe(FrostwakePlayerState), *RemovedId.ToString(), Satiation, Restored);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_ate"),
                FString::Printf(
                    TEXT("{\"player\":\"%s\",\"item\":\"%s\",\"satiation\":%.1f,\"restored\":%.1f}"),
                    *GetNameSafe(FrostwakePlayerState), *RemovedId.ToString(), Satiation, Restored));
        }
    }

    return true;
}

float AFrostwakeCharacter::GetHealth() const
{
    return AttributeComponent ? AttributeComponent->GetAttribute(EFrostwakeAttribute::Health) : 0.0f;
}

float AFrostwakeCharacter::GetMaxHealth() const
{
    return AttributeComponent ? AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Health) : 0.0f;
}

float AFrostwakeCharacter::GetSatiation() const
{
    // Food remaining = MaxHunger - Hunger (the DH-semantic Hunger attribute rises while unfed).
    return AttributeComponent
        ? AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Hunger) - AttributeComponent->GetAttribute(EFrostwakeAttribute::Hunger)
        : 0.0f;
}

float AFrostwakeCharacter::GetMaxSatiation() const
{
    return AttributeComponent ? AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Hunger) : 0.0f;
}

float AFrostwakeCharacter::GetWarmth() const
{
    return AttributeComponent ? AttributeComponent->GetAttribute(EFrostwakeAttribute::Warmth) : 0.0f;
}

float AFrostwakeCharacter::GetMaxWarmth() const
{
    return AttributeComponent ? AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Warmth) : 0.0f;
}

bool AFrostwakeCharacter::ApplyWarmth(float WarmthAmount, AActor* HeatSource)
{
    if (!HasAuthority() || WarmthAmount <= 0.0f || !AttributeComponent)
    {
        return false;
    }

    const AFrostwakePlayerState* FrostwakePlayerState = GetPlayerState<AFrostwakePlayerState>();
    if (!FrostwakePlayerState || FrostwakePlayerState->GetLifeState() != EFrostwakeLifeState::Alive)
    {
        return false;
    }

    const float Before = AttributeComponent->GetAttribute(EFrostwakeAttribute::Warmth);
    const float NewWarmth = AttributeComponent->ModifyAttribute(EFrostwakeAttribute::Warmth, WarmthAmount);
    const float Restored = NewWarmth - Before;
    if (Restored <= 0.0f)
    {
        return false; // already warm
    }

    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_warmed player=%s warmth=%.1f restored=%.1f source=%s"), *GetNameSafe(FrostwakePlayerState), NewWarmth, Restored, *GetNameSafe(HeatSource));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_warmed"),
                FString::Printf(
                    TEXT("{\"player\":\"%s\",\"warmth\":%.1f,\"restored\":%.1f,\"source\":\"%s\"}"),
                    *GetNameSafe(FrostwakePlayerState), NewWarmth, Restored, *GetNameSafe(HeatSource)));
        }
    }

    return true;
}
