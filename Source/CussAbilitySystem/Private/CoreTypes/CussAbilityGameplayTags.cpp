// Copyright Kyle Cuss and Cuss Programming 2026

#include "CussAbilitySystem/Public/CoreTypes/CussAbilityGameplayTags.h"

namespace CussAbilityTags
{
	// --------------------
	// Abilities
	// --------------------
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Fireball, "Ability.Fireball", "Fireball");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Heal, "Ability.Heal", "Heal");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Burn, "Ability.Burn", "Burn");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Rally, "Ability.Rally", "Rally");

	// --------------------
	// Statuses
	// --------------------
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Dead, "Status.Dead", "Dead");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Stunned, "Status.Stunned", "Stunned");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Silenced, "Status.Silenced", "Silenced");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Buffed, "Status.Buffed", "Buffed");

	// --------------------
	// Stats
	// --------------------
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Health, "Stat.Health", "Health");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Mana, "Stat.Mana", "Mana");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Energy, "Stat.Energy", "Energy");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_AttackPower, "Stat.AttackPower", "Attack Power");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Defense, "Stat.Defense", "Defense");

	// --------------------
	// Cues
	// --------------------
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cue_Cast_Fire, "Cue.Cast.Fire", "Fire Cast");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cue_Impact_Fire, "Cue.Impact.Fire", "Fire Impact");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cue_Cast_Heal, "Cue.Cast.Heal", "Heal Cast");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cue_Impact_Heal, "Cue.Impact.Heal", "Heal Impact");

	// --------------------
	// Cooldowns
	// --------------------
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Magic_Basic, "Cooldown.Magic.Basic", "Magic Basic");
	
	// --------------------
	// Inputs
	// --------------------
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Ability_Primary, "Input.Ability.Primary", "Primary Ability Input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Ability_Secondary, "Input.Ability.Secondary", "Secondary Ability Input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Ability_Utility, "Input.Ability.Utility", "Utility Ability Input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Ability_Ultimate, "Input.Ability.Ultimate", "Ultimate Ability Input");
}
