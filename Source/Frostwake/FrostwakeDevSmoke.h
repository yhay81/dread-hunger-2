#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Dedicated home for server-side dev smokes (plan item D / §8: keep behavioral self-tests OUT of the
 * already-overgrown GameMode). Each smoke is a free function taking the World; GameMode only wires a
 * one-line trigger from a -FrostwakeSmoke* command-line flag. New behavioral asserts land here, not as
 * AFrostwakeGameMode methods.
 *
 * Migrated so far (plan item D): the "act on the player + world + subsystems" category — perk resist,
 * eat, damage type, survival temperature, action effect, ability, inventory, held item. The remaining ~13
 * RunDevSmoke* still on GameMode are coupled to its private helpers (FindPawnForTeam, match config, role
 * assignment) and migrate as that coupling is untangled.
 *
 * Each smoke UE_LOGs `dev_smoke_<name> result=pass|fail ...`; run-local-smoke gates on that line.
 * Everything is compiled out of shipping builds.
 */
namespace FrostwakeDevSmoke
{
	/**
	 * §3.20 perk resistance via ActionEffect: take a baseline Damage.Cold hit, then wear the cold-resist
	 * perk (a UFrostwakeActionEffect) and take the identical hit — asserting the second hit deals strictly
	 * less, by exactly the perk's resistance fraction. Proves a perk-supplied modifier actually reduces
	 * typed damage through AdjustDamage (the §8 "modifiers go through Action/ActionEffect" rule, used for
	 * its design purpose for the first time).
	 */
	FROSTWAKE_API void RunPerkResist(UWorld* World);

	/** Data-driven item path (§3.2): a Food item (Ration) is eaten, a Tool item (Lantern) is rejected by data. */
	FROSTWAKE_API void RunEat(UWorld* World);

	/** Typed, data-driven damage (§3.17): a Slashing hit lowers Health by base * DamageTypeDefinition.DamageMultiplier. */
	FROSTWAKE_API void RunDamageType(UWorld* World);

	/** §3.22-23 temperature model: net temperature is positive at a heat source, negative far from any source. */
	FROSTWAKE_API void RunSurvival(UWorld* World);

	/** Action System LIVE (§9.5 step 5): applying/removing a real ActionEffect changes Health + the active count. */
	FROSTWAKE_API void RunEffect(UWorld* World);

	/**
	 * §3.20/§3.21 ability framework (review #1): activate the first concrete UFrostwakeAction (Fog) through
	 * AddAction/StartAction and assert the thickened lifecycle — Stamina cost is consumed, the action is
	 * running + on cooldown, and an immediate re-activation is refused (cooldown/running gate). Proves the
	 * Action half of the system is LIVE and expressive enough for the planned spells.
	 */
	FROSTWAKE_API void RunAbility(UWorld* World);

	/**
	 * Inventory stacking (review #2): add MaxStack units of a stackable item (Ration, MaxStack 5) and assert
	 * they occupy ONE slot at full count, then assert the next unit spills into a NEW slot. Proves the
	 * container honours ItemDefinition.MaxStack (the structural fix items(55) needs). Run alone (fresh bag).
	 */
	FROSTWAKE_API void RunInventory(UWorld* World);

	/**
	 * Held-item publication (review #2b): assert the server-authoritative, replicated-to-all HeldItemId
	 * tracks the held slot — empty hands read None; adding/selecting an item publishes it; removing the held
	 * item re-derives it. Proves the foundation for "others see what you carry" (the field replicates
	 * COND_None, unlike the owner-only backpack). Run alone (fresh bag).
	 */
	FROSTWAKE_API void RunHeldItem(UWorld* World);
}
