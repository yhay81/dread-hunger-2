#include "FrostwakeCharacter.h"
#include "FrostwakeInteractionComponent.h"
#include "FrostwakeInventoryComponent.h"
#include "FrostwakeItemPickupActor.h"
#include "FrostwakeGameMode.h"
#include "FrostwakeLog.h"
#include "FrostwakePlayerState.h"
#include "FrostwakeTelemetrySubsystem.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "ActionSystem/FrostwakeActionComponent.h"
#include "ActionSystem/FrostwakeColdExposureEffect.h"
#include "ActionSystem/FrostwakeMatchSubsystem.h"
#include "ActionSystem/FrostwakeTemperatureSubsystem.h"
#include "Data/FrostwakeDataSubsystem.h"
#include "Data/FrostwakeItemDefinition.h"
#include "Data/FrostwakeDamageTypeDefinition.h"
#include "FrostwakeGameplayTags.h"
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
    // Whether an item is food, and how much hunger it removes, now comes from its ItemDefinition
    // (data-driven, §3.2) — no hardcoded item list on the character.

    InteractionComponent = CreateDefaultSubobject<UFrostwakeInteractionComponent>(TEXT("InteractionComponent"));
    InventoryComponent = CreateDefaultSubobject<UFrostwakeInventoryComponent>(TEXT("InventoryComponent"));
    // The Action System attribute component owns the vitals: Health + Warmth start full, Hunger starts at
    // 0 (fed). Warmth is temperature-driven (plan §3.22-23). All replicate push-model to clients.
    AttributeComponent = CreateDefaultSubobject<UFrostwakeAttributeComponent>(TEXT("AttributeComponent"));
    // Action System host for buffs/debuffs/abilities (plan §3.2 Tier 2); finds the AttributeComponent sibling.
    ActionComponent = CreateDefaultSubobject<UFrostwakeActionComponent>(TEXT("ActionComponent"));

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

    // Starvation and cold each apply as their own typed world-damage (§3.17), through the normal damage
    // path (so they down the player + emit telemetry). If the first downs the player the second no-ops.
    if (AttributeComponent
        && AttributeComponent->GetAttribute(EFrostwakeAttribute::Hunger) >= AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Hunger))
    {
        ApplyServerDamage(StarvationDamagePerSecond * DeltaSeconds, FrostwakeTags::Damage_Starvation, this);
    }
    // Cold exposure is an Action System effect (Bundle A / §9.5 step 5): apply it while Warmth is bottomed
    // out (the effect inflicts periodic DT_Cold through the damage path), remove it when Warmth recovers —
    // the debuff runs through the ability framework, not a raw per-tick ApplyServerDamage here.
    if (ActionComponent)
    {
        const bool bFreezing = AttributeComponent && AttributeComponent->GetAttribute(EFrostwakeAttribute::Warmth) <= 0.0f;
        if (bFreezing && !ActiveColdEffect)
        {
            ActiveColdEffect = ActionComponent->ApplyEffect(UFrostwakeColdExposureEffect::StaticClass());
        }
        else if (!bFreezing && ActiveColdEffect)
        {
            ActionComponent->RemoveEffect(ActiveColdEffect);
            ActiveColdEffect = nullptr;
        }
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
        PublishSelectedSlot();
    }
}

void AFrostwakeCharacter::SelectPrevItem()
{
    if (InventoryComponent)
    {
        InventoryComponent->CycleSelectedSlot(-1);
        PublishSelectedSlot();
    }
}

void AFrostwakeCharacter::PublishSelectedSlot()
{
    if (!InventoryComponent)
    {
        return;
    }
    // The held item is server-authoritative; the locally-cycled slot is sent up so the server publishes
    // the visible-to-all HeldItemId (review #2b). On a listen host this resolves on authority directly.
    const int32 SlotIndex = InventoryComponent->GetSelectedSlot();
    if (HasAuthority())
    {
        InventoryComponent->SetServerSelectedSlot(SlotIndex);
    }
    else
    {
        ServerSetSelectedSlot(SlotIndex);
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

float AFrostwakeCharacter::AdjustDamage(float BaseDamage, const FGameplayTag& DamageType, bool& bOutAffectsReservedHealth) const
{
    // Data-driven (plan §3.2/§3.17): the DamageTypeDefinition's multiplier scales the amount, and its
    // bAffectsReservedHealth flag (DT_Poison) routes the result into the ReservedHealth reserve. Unknown
    // type = pass through unchanged.
    bOutAffectsReservedHealth = false;
    float Multiplier = 1.0f;
    bool bAffectedByResistance = false;
    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const UFrostwakeDataSubsystem* DataSubsystem = GameInstance->GetSubsystem<UFrostwakeDataSubsystem>())
        {
            if (const UFrostwakeDamageTypeDefinition* Definition = DataSubsystem->GetDamageTypeDefinitionForTag(DamageType))
            {
                Multiplier = FMath::Max(0.0f, Definition->DamageMultiplier);
                bOutAffectsReservedHealth = Definition->bAffectsReservedHealth;
                bAffectedByResistance = Definition->bAffectedByResistance;
            }
        }
    }

    // §3.20 perk resistances: if this damage type honours resistance, scale by the character's active
    // resistance multiplier. Resistances are supplied by perk ActionEffects (§8: no raw methods) and
    // accumulated on the ActionComponent — so e.g. a cold-resist perk halves incoming Damage.Cold.
    if (bAffectedByResistance && ActionComponent)
    {
        Multiplier *= ActionComponent->GetDamageResistanceMultiplier(DamageType);
    }
    return BaseDamage * Multiplier;
}

bool AFrostwakeCharacter::ApplyServerDamage(float DamageAmount, FGameplayTag DamageType, AActor* DamageSource)
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

    // §3.17: scale the incoming amount by the data-driven damage type before it hits Health.
    bool bAffectsReservedHealth = false;
    const float FinalDamage = AdjustDamage(DamageAmount, DamageType, bAffectsReservedHealth);
    if (FinalDamage <= 0.0f)
    {
        return false;
    }

    // ReservedHealth routing (DH §3.17: 毒は reserve health). A reserve-draining type (DT_Poison) is
    // absorbed by ReservedHealth first; only the overflow reaches living Health (so poison can't down you
    // directly until the reserve is gone).
    float HealthDamage = FinalDamage;
    if (bAffectsReservedHealth)
    {
        const float Reserve = AttributeComponent->GetAttribute(EFrostwakeAttribute::ReservedHealth);
        const float Absorbed = FMath::Min(Reserve, FinalDamage);
        AttributeComponent->ModifyAttribute(EFrostwakeAttribute::ReservedHealth, -Absorbed);
        HealthDamage = FinalDamage - Absorbed;
    }

    const float NewHealth = (HealthDamage > 0.0f)
        ? AttributeComponent->ModifyAttribute(EFrostwakeAttribute::Health, -HealthDamage)
        : AttributeComponent->GetAttribute(EFrostwakeAttribute::Health);
    const FString DamageTypeName = DamageType.IsValid() ? DamageType.GetTagName().ToString() : FString(TEXT("Untyped"));
    UE_LOG(LogFrostwakeGameplay, Log, TEXT("player_damaged player=%s health=%.1f damage=%.1f type=%s source=%s"), *GetNameSafe(FrostwakePlayerState), NewHealth, FinalDamage, *DamageTypeName, *GetNameSafe(DamageSource));
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UFrostwakeTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UFrostwakeTelemetrySubsystem>())
        {
            TelemetrySubsystem->LogEvent(
                TEXT("player_damaged"),
                FString::Printf(
                    TEXT("{\"player\":\"%s\",\"health\":%.1f,\"damage\":%.1f,\"type\":\"%s\",\"source\":\"%s\"}"),
                    *GetNameSafe(FrostwakePlayerState),
                    NewHealth,
                    FinalDamage,
                    *DamageTypeName,
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

    // DH revive (KNOWLEDGE §3.x: 50% HP / 25% Warmth), drawn from the down "reserve" (ReservedHealth) so a
    // rescue after the reserve was drained (e.g. poisoned while down) restores less. Reserve = the body's
    // revivable buffer (ALIVE→CARCASS→DEAD model). Reserve refill (rest/regen) is a later mechanic.
    const float MaxH = AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Health);
    const float Reserve = AttributeComponent->GetAttribute(EFrostwakeAttribute::ReservedHealth);
    const float ReviveHealth = FMath::Clamp(FMath::Min(MaxH * 0.5f, Reserve), 1.0f, MaxH);
    AttributeComponent->SetAttribute(EFrostwakeAttribute::Health, ReviveHealth);
    AttributeComponent->ModifyAttribute(EFrostwakeAttribute::ReservedHealth, -ReviveHealth); // consume the reserve used
    AttributeComponent->SetAttribute(EFrostwakeAttribute::Warmth, AttributeComponent->GetMaxAttribute(EFrostwakeAttribute::Warmth) * 0.25f);
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

void AFrostwakeCharacter::ServerSetSelectedSlot_Implementation(int32 SlotIndex)
{
    if (InventoryComponent)
    {
        InventoryComponent->SetServerSelectedSlot(SlotIndex);
    }
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
    if (ItemId.IsNone())
    {
        return false;
    }

    // Data-driven (plan §3.2 "logic in C++, data in assets"): resolve the held item's definition by its
    // canonical FName and let the DATA decide whether it is edible and how much hunger it removes — no
    // hardcoded food list. The inventory FName == the definition's ItemId (§8 identifier convention).
    const UFrostwakeItemDefinition* Definition = nullptr;
    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const UFrostwakeDataSubsystem* DataSubsystem = GameInstance->GetSubsystem<UFrostwakeDataSubsystem>())
        {
            Definition = DataSubsystem->GetItemDefinition(ItemId);
        }
    }
    if (!Definition || Definition->Category != EFrostwakeItemCategory::Food)
    {
        return false;
    }

    // Don't waste a ration when already fully fed (Hunger at 0). Mirrors ApplyWarmth's "already warm"
    // guard — consume the item only when it can actually do something.
    if (AttributeComponent->GetAttribute(EFrostwakeAttribute::Hunger) <= 0.0f)
    {
        return false;
    }

    FName RemovedId = NAME_None;
    if (!InventoryComponent->TryRemoveItemAt(SlotIndex, RemovedId))
    {
        return false;
    }

    // Eating reduces the DH-semantic Hunger attribute by the definition's value; report it as restored
    // "satiation" (food remaining).
    const float Before = GetSatiation();
    AttributeComponent->ModifyAttribute(EFrostwakeAttribute::Hunger, -Definition->HungerRestore);
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
