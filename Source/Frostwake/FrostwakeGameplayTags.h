#pragma once

#include "NativeGameplayTags.h"

/**
 * Frostwake native Gameplay Tag taxonomy — the single source of truth for code-referenced tags.
 *
 * The hierarchical STRUCTURE here is a one-way door (plan §8): content references these tags, so
 * renaming a branch after content exists is a migration. Therefore the structure is locked now,
 * seeded from the verified Dread-Hunger spec (TEST2 native_enums / KNOWLEDGE.md).
 *
 * IP rule (plan §2): leaf names tied to DH-specific *expression* (occult / thrall / totem theme) are
 * abstracted here to FUNCTIONAL terms — IP-safe and resilient to the narrative setting. Generic
 * functional leaves (roles, ship systems, damage kinds, crafting families) are kept verbatim because
 * they are mechanics, not expression. Per-item and per-stat leaves are populated incrementally as the
 * data-driven content (ItemDefinition / RecipeDefinition) lands — only categories are fixed here.
 */
namespace FrostwakeTags
{
	// ── Teams / factions (mirror EFrostwakeTeam) ──────────────────────────────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Crew);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Saboteur);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Madman);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Spectator);

	// ── Roles (8) ─────────────────────────────────────────────────────────────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Captain);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Chaplain);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Cook);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Doctor);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Engineer);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Hunter);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Marine);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Navigator);

	// ── Item categories (per-item leaves come from item_stats.txt later) ──────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_Melee);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Weapon_Ranged);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Food);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Fuel);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Tool);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Resource);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Quest);

	// ── Ship systems (extends EFrostwakeShipSystem with DH boiler/helm) ───────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ship_System_Hull);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ship_System_Boiler);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ship_System_Helm);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ship_System_Fuel);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ship_System_Power);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ship_System_Radio);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ship_System_Flooding);

	// ── Crafting model (families / methods / rules) ───────────────────────────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Family_Arrowheads);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Family_Fuel);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Family_Meats);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Family_Metals);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Family_Repairable);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Method_Ground);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Method_Inventory);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Rule_All);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Rule_Any);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Rule_AnyUseAll);

	// ── Damage kinds (DH DT_* set) ────────────────────────────────────────────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Cold);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Drowning);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Starvation);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Poison);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Piercing);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Slashing);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Blunt);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Explosive);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Falling);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Fire);

	// ── Status effects ────────────────────────────────────────────────────────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Cold);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Warm);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Poisoned);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Starving);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stunned);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Downed);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Hidden);

	// ── Role perks (signature, 8 — values via CROSSCHECK) ─────────────────────
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_ShipHandling);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_TrinketChance);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_StoveSpeed);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_WorkbenchSpeed);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_MeatHarvest);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_ReloadSpeed);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_GroundSpeed);
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Role_MedBoost);

	// ── Saboteur abilities — abstracted from DH thrall spells to functional, IP-safe names ──
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Saboteur_Phase);        // intangible escape  (cf. spirit walk)
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Saboteur_Silence);      // suppress comms     (cf. hush)
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Saboteur_Fog);          // obscure vision     (cf. whiteout)
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Saboteur_SummonThreat); // call an AI threat  (cf. cannibal attack)
	FROSTWAKE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Saboteur_Impersonate);  // disguise as crew   (cf. doppelganger)
}
