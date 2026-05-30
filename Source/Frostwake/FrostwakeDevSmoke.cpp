#include "FrostwakeDevSmoke.h"

#include "FrostwakeCharacter.h"
#include "FrostwakeGameplayTags.h"
#include "FrostwakeLog.h"
#include "ActionSystem/FrostwakeActionComponent.h"
#include "ActionSystem/FrostwakeActionEffect.h"
#include "ActionSystem/FrostwakeColdResistPerkEffect.h"
#include "Engine/World.h"
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
