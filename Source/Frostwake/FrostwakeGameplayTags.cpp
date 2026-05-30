#include "FrostwakeGameplayTags.h"

// Definitions for the Frostwake native Gameplay Tag taxonomy declared in FrostwakeGameplayTags.h.
// Tag string = dotted path (the canonical name); variable = underscored. See the header for the
// one-way-door / IP rationale. Comments cite the verified DH source where a name was abstracted.
namespace FrostwakeTags
{
	// Teams
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Team_Crew, "Team.Crew", "Crew faction (the ship's company / explorers).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Team_Saboteur, "Team.Saboteur", "Hidden saboteur faction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Team_Madman, "Team.Madman", "Madman: reads as Crew, wins with the Saboteurs.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Team_Spectator, "Team.Spectator", "Eliminated / observing.");

	// Roles
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Captain, "Role.Captain", "Captain — ship handling.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Chaplain, "Role.Chaplain", "Chaplain — trinket luck.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Cook, "Role.Cook", "Cook — stove speed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Doctor, "Role.Doctor", "Doctor — medical boost.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Engineer, "Role.Engineer", "Engineer — workbench speed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Hunter, "Role.Hunter", "Hunter — meat harvest.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Marine, "Role.Marine", "Marine — reload speed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Navigator, "Role.Navigator", "Navigator — ground speed.");

	// Item categories
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Melee, "Item.Weapon.Melee", "Melee weapon.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Ranged, "Item.Weapon.Ranged", "Ranged weapon.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Food, "Item.Food", "Edible item (restores hunger).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Fuel, "Item.Fuel", "Burns in the boiler (fuel value).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Tool, "Item.Tool", "Tool / utility item.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Resource, "Item.Resource", "Crafting resource / raw material.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable, "Item.Consumable", "Single-use consumable (medicine, poison, etc.).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Quest, "Item.Quest", "Quest / objective item.");

	// Ship systems
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ship_System_Hull, "Ship.System.Hull", "Hull integrity (breaches cause flooding).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ship_System_Boiler, "Ship.System.Boiler", "Boiler — load fuel + ignite to make way.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ship_System_Helm, "Ship.System.Helm", "Helm — steer while the boiler burns.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ship_System_Fuel, "Ship.System.Fuel", "Fuel reserve feeding the boiler.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ship_System_Power, "Ship.System.Power", "Electrical / lighting power.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ship_System_Radio, "Ship.System.Radio", "Radio / comms.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ship_System_Flooding, "Ship.System.Flooding", "Active flooding state (sinks if unrepaired).");

	// Crafting
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Family_Arrowheads, "Crafting.Family.Arrowheads", "Ingredient family: arrowheads.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Family_Fuel, "Crafting.Family.Fuel", "Ingredient family: fuel.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Family_Meats, "Crafting.Family.Meats", "Ingredient family: meats.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Family_Metals, "Crafting.Family.Metals", "Ingredient family: metals.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Family_Repairable, "Crafting.Family.Repairable", "Ingredient family: repairable parts.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Method_Ground, "Crafting.Method.Ground", "Crafted on the ground / at a station.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Method_Inventory, "Crafting.Method.Inventory", "Crafted from the inventory.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Rule_All, "Crafting.Rule.All", "Consumes all of each required ingredient.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Rule_Any, "Crafting.Rule.Any", "Any matching ingredient satisfies.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting_Rule_AnyUseAll, "Crafting.Rule.AnyUseAll", "Any match, but consume all matches.");

	// Damage kinds
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Cold, "Damage.Cold", "Cold / exposure (world-caused).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Drowning, "Damage.Drowning", "Drowning (world-caused).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Starvation, "Damage.Starvation", "Starvation (world-caused).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Poison, "Damage.Poison", "Poison / toxin over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Piercing, "Damage.Piercing", "Piercing (arrows, bullets).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Slashing, "Damage.Slashing", "Slashing (causes dismemberment).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Blunt, "Damage.Blunt", "Blunt impact.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Explosive, "Damage.Explosive", "Explosive (powderkeg, nitro).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Falling, "Damage.Falling", "Fall damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Fire, "Damage.Fire", "Fire / burning.");

	// Status effects
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Cold, "Status.Cold", "Too cold — warmth draining.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Warm, "Status.Warm", "Warmth boost active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Poisoned, "Status.Poisoned", "Poisoned.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Starving, "Status.Starving", "Hunger empty — health draining.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Stunned, "Status.Stunned", "Stunned / incapacitated briefly.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Downed, "Status.Downed", "Downed (revivable).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hidden, "Status.Hidden", "Hidden from detection (saboteur phase).");

	// Role perks
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_ShipHandling, "Perk.Role.ShipHandling", "Captain: faster turning (~+50%).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_TrinketChance, "Perk.Role.TrinketChance", "Chaplain: trinket find rate (~+7%).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_StoveSpeed, "Perk.Role.StoveSpeed", "Cook: cooking speed (~+50%).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_WorkbenchSpeed, "Perk.Role.WorkbenchSpeed", "Engineer: craft/repair speed (~+30%).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_MeatHarvest, "Perk.Role.MeatHarvest", "Hunter: faster meat harvesting.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_ReloadSpeed, "Perk.Role.ReloadSpeed", "Marine: faster reload.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_GroundSpeed, "Perk.Role.GroundSpeed", "Navigator: move speed (~+10%).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Role_MedBoost, "Perk.Role.MedBoost", "Doctor: stronger revive/heal.");

	// Saboteur abilities (abstracted from DH thrall spells)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Saboteur_Phase, "Ability.Saboteur.Phase", "Become intangible / escape confinement (cf. spirit walk).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Saboteur_Silence, "Ability.Saboteur.Silence", "Suppress crew communication (cf. hush).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Saboteur_Fog, "Ability.Saboteur.Fog", "Obscure vision with fog/storm (cf. whiteout).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Saboteur_SummonThreat, "Ability.Saboteur.SummonThreat", "Call an AI threat (cf. cannibal attack).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Saboteur_Impersonate, "Ability.Saboteur.Impersonate", "Disguise as another crew member (cf. doppelganger).");
}
