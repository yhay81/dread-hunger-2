#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Dedicated home for server-side dev smokes (plan item D / §8: keep behavioral self-tests OUT of the
 * already-overgrown GameMode). Each smoke is a free function taking the World; GameMode only wires a
 * one-line trigger from a -FrostwakeSmoke* command-line flag. New behavioral asserts land here, not as
 * AFrostwakeGameMode methods. (The ~18 legacy RunDevSmoke* still on GameMode migrate here incrementally.)
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
}
