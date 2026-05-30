#include "FrostwakeDevSmoke.h"

#include "FrostwakeCharacter.h"
#include "FrostwakeGameplayTags.h"
#include "FrostwakeHeatSourceActor.h"
#include "FrostwakeInventoryComponent.h"
#include "FrostwakeLog.h"
#include "ActionSystem/FrostwakeAction.h"
#include "ActionSystem/FrostwakeActionComponent.h"
#include "ActionSystem/FrostwakeActionEffect.h"
#include "ActionSystem/FrostwakeAttributeComponent.h"
#include "ActionSystem/FrostwakeColdExposureEffect.h"
#include "ActionSystem/FrostwakeColdResistPerkEffect.h"
#include "ActionSystem/FrostwakeFogAbility.h"
#include "ActionSystem/FrostwakeTemperatureSubsystem.h"
#include "Data/FrostwakeDamageTypeDefinition.h"
#include "Data/FrostwakeDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

namespace
{
// The first locally-controlled Frostwake character (host pawn). Shared by smokes that act on "the player".
AFrostwakeCharacter* FindFirstPlayerCharacter(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (AFrostwakeCharacter* Candidate = PlayerController ? Cast<AFrostwakeCharacter>(PlayerController->GetPawn()) : nullptr)
		{
			return Candidate;
		}
	}
	return nullptr;
}
}

void FrostwakeDevSmoke::RunPerkResist(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AFrostwakeCharacter* Character = FindFirstPlayerCharacter(World);
	UFrostwakeActionComponent* Action = Character ? Character->FindComponentByClass<UFrostwakeActionComponent>() : nullptr;

	// Read the resist fraction off the perk CDO so the assertion never drifts from the perk's own default.
	const float ResistFraction = GetDefault<UFrostwakeColdResistPerkEffect>()->ResistanceFraction;
	const float BaseDamage = 10.0f;

	bool bApplied = false;
	int32 ActiveBefore = -1;
	int32 ActiveAfterApply = -1;
	float DeltaUnresisted = -1.0f;
	float DeltaResisted = -1.0f;

	if (Character && Action)
	{
		// 1) Baseline: a sub-lethal cold hit with NO perk active.
		const float H0 = Character->GetHealth();
		Character->ApplyServerDamage(BaseDamage, FrostwakeTags::Damage_Cold, Character);
		DeltaUnresisted = H0 - Character->GetHealth();

		// 2) Wear the cold-resist perk (an ActionEffect), then take the identical hit.
		ActiveBefore = Action->GetActiveEffectCount();
		UFrostwakeActionEffect* Perk = Action->ApplyEffect(UFrostwakeColdResistPerkEffect::StaticClass());
		bApplied = Perk != nullptr;
		ActiveAfterApply = Action->GetActiveEffectCount();

		const float H2 = Character->GetHealth();
		Character->ApplyServerDamage(BaseDamage, FrostwakeTags::Damage_Cold, Character);
		DeltaResisted = H2 - Character->GetHealth();
	}

	// The resisted hit must land for exactly (1 - fraction) of the baseline — and strictly less. This holds
	// regardless of Cold's data-driven DamageMultiplier (it cancels in the ratio).
	const float ExpectedResisted = DeltaUnresisted * (1.0f - ResistFraction);
	const bool bPass = bApplied
		&& ActiveAfterApply == ActiveBefore + 1
		&& DeltaUnresisted > 0.0f
		&& DeltaResisted < DeltaUnresisted
		&& FMath::IsNearlyEqual(DeltaResisted, ExpectedResisted, 0.01f);

	UE_LOG(
		LogFrostwakeGameplay,
		Log,
		TEXT("dev_smoke_perk_resist result=%s base=%.1f resistFraction=%.2f deltaUnresisted=%.1f deltaResisted=%.1f expectedResisted=%.1f activeBefore=%d activeAfterApply=%d"),
		bPass ? TEXT("pass") : TEXT("fail"),
		BaseDamage, ResistFraction, DeltaUnresisted, DeltaResisted, ExpectedResisted, ActiveBefore, ActiveAfterApply);
#endif
}

void FrostwakeDevSmoke::RunEat(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AFrostwakeCharacter* Character = FindFirstPlayerCharacter(World);

	// Behavioral proof of the data-driven item path (plan §3.2): EatRation resolves the held item's
	// ItemDefinition by its canonical FName. Positive = a Food item (Ration) is eaten; negative = a Tool
	// item (Lantern) is rejected BY THE DATA (Category != Food). Both succeeding proves the consumer is
	// wired AND the §8 identifier convention (inventory FName == definition ItemId) holds at runtime.
	// Make the player hungry first so eating actually does something: EatRation's full-guard refuses to
	// consume a ration when already fed (Hunger == 0, which is the spawn state).
	if (UFrostwakeAttributeComponent* Attributes = Character ? Character->FindComponentByClass<UFrostwakeAttributeComponent>() : nullptr)
	{
		Attributes->ModifyAttribute(EFrostwakeAttribute::Hunger, 60.0f);
	}

	bool bAteFood = false;
	bool bRejectedTool = false;
	if (UFrostwakeInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr)
	{
		Inventory->TryAddItem(FName(TEXT("Ration")));
		const int32 RationSlot = Inventory->GetItems().IndexOfByKey(FName(TEXT("Ration")));
		if (RationSlot != INDEX_NONE)
		{
			bAteFood = Character->EatRation(RationSlot);
		}

		Inventory->TryAddItem(FName(TEXT("Lantern")));
		const int32 LanternSlot = Inventory->GetItems().IndexOfByKey(FName(TEXT("Lantern")));
		if (LanternSlot != INDEX_NONE)
		{
			bRejectedTool = !Character->EatRation(LanternSlot);
		}
	}

	const bool bPass = bAteFood && bRejectedTool;
	UE_LOG(
		LogFrostwakeGameplay,
		Log,
		TEXT("dev_smoke_eat result=%s ateFood=%s rejectedTool=%s character=%s"),
		bPass ? TEXT("pass") : TEXT("fail"),
		bAteFood ? TEXT("true") : TEXT("false"),
		bRejectedTool ? TEXT("true") : TEXT("false"),
		*GetNameSafe(Character));
#endif
}

void FrostwakeDevSmoke::RunDamageType(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AFrostwakeCharacter* Character = FindFirstPlayerCharacter(World);

	// Behavioral proof of typed, data-driven damage (§3.17): apply a sub-lethal Slashing hit and confirm
	// Health fell by base * DamageTypeDefinition.DamageMultiplier (read from the loaded data). Proves
	// AdjustDamage consumes the damage-type data (a 2nd data-driven consumer beyond items).
	const float BaseDamage = 20.0f;
	float Multiplier = -1.0f;
	if (const UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (const UFrostwakeDataSubsystem* DataSubsystem = GameInstance->GetSubsystem<UFrostwakeDataSubsystem>())
		{
			if (const UFrostwakeDamageTypeDefinition* Def = DataSubsystem->GetDamageTypeDefinitionForTag(FrostwakeTags::Damage_Slashing))
			{
				Multiplier = Def->DamageMultiplier;
			}
		}
	}

	float HealthBefore = -1.0f;
	float HealthAfter = -1.0f;
	bool bApplied = false;
	if (Character)
	{
		HealthBefore = Character->GetHealth();
		bApplied = Character->ApplyServerDamage(BaseDamage, FrostwakeTags::Damage_Slashing, Character);
		HealthAfter = Character->GetHealth();
	}

	const float ActualDelta = HealthBefore - HealthAfter;
	const float ExpectedDelta = (Multiplier >= 0.0f) ? BaseDamage * Multiplier : BaseDamage;
	const bool bPass = bApplied && Multiplier > 0.0f && FMath::IsNearlyEqual(ActualDelta, ExpectedDelta, 0.01f);

	UE_LOG(
		LogFrostwakeGameplay,
		Log,
		TEXT("dev_smoke_damage_type result=%s base=%.1f multiplier=%.2f expectedDelta=%.1f actualDelta=%.1f healthBefore=%.1f healthAfter=%.1f"),
		bPass ? TEXT("pass") : TEXT("fail"),
		BaseDamage, Multiplier, ExpectedDelta, ActualDelta, HealthBefore, HealthAfter);
#endif
}

void FrostwakeDevSmoke::RunSurvival(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	const UFrostwakeTemperatureSubsystem* Temperature = UFrostwakeTemperatureSubsystem::Get(World);
	AFrostwakeHeatSourceActor* HeatSource = nullptr;
	for (TActorIterator<AFrostwakeHeatSourceActor> It(World); It; ++It)
	{
		HeatSource = *It;
		break;
	}

	// Behavioral assertion for the §3.22-23 temperature model that drives Warmth: at a heat source the
	// net temperature is positive (Warmth would rise), far from any source it is negative (Warmth falls).
	// This guards the "near fire = warm up / out in the cold = freeze" mechanic, not just compile/no-crash.
	const bool bHasModel = (Temperature != nullptr) && (HeatSource != nullptr);
	float TempAtSource = 0.0f;
	float TempFar = 0.0f;
	if (bHasModel)
	{
		const FVector SourceLocation = HeatSource->GetActorLocation();
		const FVector FarLocation = SourceLocation + FVector(100000.0f, 0.0f, 0.0f);
		TempAtSource = Temperature->ComputeTemperatureAt(SourceLocation);
		TempFar = Temperature->ComputeTemperatureAt(FarLocation);
	}

	const bool bPass = bHasModel && TempAtSource > 0.0f && TempFar < 0.0f && TempAtSource > TempFar;

	UE_LOG(
		LogFrostwakeGameplay,
		Log,
		TEXT("dev_smoke_survival result=%s heatSource=%s tempAtSource=%.3f tempFar=%.3f"),
		bPass ? TEXT("pass") : TEXT("fail"),
		*GetNameSafe(HeatSource),
		TempAtSource, TempFar);
#endif
}

void FrostwakeDevSmoke::RunEffect(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AFrostwakeCharacter* Character = FindFirstPlayerCharacter(World);

	// Proof that the Action System is LIVE (plan §9.5 step 5): applying a real ActionEffect through the
	// ActionComponent changes an attribute (the cold effect's first DT_Cold bite lowers Health), and
	// removing it returns the active-effect count to zero. No raw method path.
	UFrostwakeActionComponent* Action = Character ? Character->FindComponentByClass<UFrostwakeActionComponent>() : nullptr;
	bool bApplied = false;
	int32 ActiveBefore = -1;
	int32 ActiveAfterApply = -1;
	int32 ActiveAfterRemove = -1;
	float HealthBefore = -1.0f;
	float HealthAfterApply = -1.0f;
	if (Action && Character)
	{
		ActiveBefore = Action->GetActiveEffectCount();
		HealthBefore = Character->GetHealth();
		UFrostwakeActionEffect* Effect = Action->ApplyEffect(UFrostwakeColdExposureEffect::StaticClass());
		bApplied = Effect != nullptr;
		ActiveAfterApply = Action->GetActiveEffectCount();
		HealthAfterApply = Character->GetHealth();
		if (Effect)
		{
			Action->RemoveEffect(Effect);
		}
		ActiveAfterRemove = Action->GetActiveEffectCount();
	}

	const bool bPass = bApplied
		&& ActiveAfterApply == ActiveBefore + 1
		&& ActiveAfterRemove == ActiveBefore
		&& HealthAfterApply < HealthBefore;

	UE_LOG(
		LogFrostwakeGameplay,
		Log,
		TEXT("dev_smoke_effect result=%s activeBefore=%d activeAfterApply=%d activeAfterRemove=%d healthBefore=%.1f healthAfterApply=%.1f"),
		bPass ? TEXT("pass") : TEXT("fail"),
		ActiveBefore, ActiveAfterApply, ActiveAfterRemove, HealthBefore, HealthAfterApply);
#endif
}

void FrostwakeDevSmoke::RunAbility(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AFrostwakeCharacter* Character = FindFirstPlayerCharacter(World);
	UFrostwakeActionComponent* Action = Character ? Character->FindComponentByClass<UFrostwakeActionComponent>() : nullptr;
	UFrostwakeAttributeComponent* Attributes = Character ? Character->FindComponentByClass<UFrostwakeAttributeComponent>() : nullptr;

	// Proof the Action (ability) half is LIVE (review #1): grant + activate the first concrete action through
	// the component, then assert the thickened base lifecycle: the Stamina cost is consumed, the action is
	// running and on cooldown, and an immediate re-activation is refused. Read the expected cost off the CDO
	// so the assertion tracks the ability's own data.
	const float ExpectedCost = GetDefault<UFrostwakeFogAbility>()->Cost;

	bool bStarted = false;
	bool bSecondRefused = false;
	bool bRunning = false;
	bool bOnCooldown = false;
	float StaminaBefore = -1.0f;
	float StaminaAfter = -1.0f;
	if (Action && Attributes)
	{
		UFrostwakeAction* Fog = Action->AddAction(UFrostwakeFogAbility::StaticClass());
		StaminaBefore = Attributes->GetAttribute(EFrostwakeAttribute::Stamina);
		bStarted = Action->StartAction(Fog);                  // off cooldown + enough Stamina -> starts
		StaminaAfter = Attributes->GetAttribute(EFrostwakeAttribute::Stamina);
		bRunning = Fog && Fog->IsRunning();
		bOnCooldown = Fog && Fog->IsOnCooldown();
		bSecondRefused = !Action->StartAction(Fog);           // running + on cooldown -> must be refused
	}

	const float StaminaSpent = StaminaBefore - StaminaAfter;
	const bool bPass = bStarted
		&& bSecondRefused
		&& bRunning
		&& bOnCooldown
		&& FMath::IsNearlyEqual(StaminaSpent, ExpectedCost, 0.01f);

	UE_LOG(
		LogFrostwakeGameplay,
		Log,
		TEXT("dev_smoke_ability result=%s started=%s secondRefused=%s running=%s onCooldown=%s cost=%.1f staminaBefore=%.1f staminaAfter=%.1f"),
		bPass ? TEXT("pass") : TEXT("fail"),
		bStarted ? TEXT("true") : TEXT("false"),
		bSecondRefused ? TEXT("true") : TEXT("false"),
		bRunning ? TEXT("true") : TEXT("false"),
		bOnCooldown ? TEXT("true") : TEXT("false"),
		StaminaSpent, StaminaBefore, StaminaAfter);
#endif
}
