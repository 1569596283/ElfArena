#include "ElfGameplayTags.h"
#include "GameplayTagsManager.h"

FElfGameplayTags FElfGameplayTags::GameplayTags;

void FElfGameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	GameplayTags.Input_Jump = Manager.AddNativeGameplayTag(FName("Input.Jump"));

	GameplayTags.Input_Space = Manager.AddNativeGameplayTag(FName("Input.Space"));
	GameplayTags.Input_Slot1 = Manager.AddNativeGameplayTag(FName("Input.Slot1"));
	GameplayTags.Input_Slot2 = Manager.AddNativeGameplayTag(FName("Input.Slot2"));
	GameplayTags.Input_Slot3 = Manager.AddNativeGameplayTag(FName("Input.Slot3"));
	GameplayTags.Input_Slot4 = Manager.AddNativeGameplayTag(FName("Input.Slot4"));
	GameplayTags.Input_Slot5 = Manager.AddNativeGameplayTag(FName("Input.Slot5"));
	GameplayTags.Input_Slot6 = Manager.AddNativeGameplayTag(FName("Input.Slot6"));

	GameplayTags.Input_X = Manager.AddNativeGameplayTag(FName("Input.X"));
	GameplayTags.Input_Q = Manager.AddNativeGameplayTag(FName("Input.Q"));
	GameplayTags.Input_W = Manager.AddNativeGameplayTag(FName("Input.W"));
	GameplayTags.Input_E = Manager.AddNativeGameplayTag(FName("Input.E"));
	GameplayTags.Input_R = Manager.AddNativeGameplayTag(FName("Input.R"));
	GameplayTags.Input_Escape = Manager.AddNativeGameplayTag(FName("Input.Escape"));

	GameplayTags.Battle_Trigger_EnterBattle = Manager.AddNativeGameplayTag(FName("Battle.Trigger.EnterBattle"));
	GameplayTags.Battle_Trigger_OnField = Manager.AddNativeGameplayTag(FName("Battle.Trigger.OnField"));
	GameplayTags.Battle_Trigger_TurnStart = Manager.AddNativeGameplayTag(FName("Battle.Trigger.TurnStart"));
	GameplayTags.Battle_Trigger_TurnEnd = Manager.AddNativeGameplayTag(FName("Battle.Trigger.TurnEnd"));
	GameplayTags.Battle_Trigger_DealSuperEffective = Manager.AddNativeGameplayTag(FName("Battle.Trigger.DealSuperEffective"));
	GameplayTags.Battle_Trigger_UseElementSkill = Manager.AddNativeGameplayTag(FName("Battle.Trigger.UseElementSkill"));
	GameplayTags.Battle_Trigger_FirstAttack = Manager.AddNativeGameplayTag(FName("Battle.Trigger.FirstAttack"));
	GameplayTags.Battle_Trigger_TakeDamage = Manager.AddNativeGameplayTag(FName("Battle.Trigger.TakeDamage"));
	GameplayTags.Battle_Trigger_OnBench = Manager.AddNativeGameplayTag(FName("Battle.Trigger.OnBench"));
	GameplayTags.Battle_Trigger_OnDeath = Manager.AddNativeGameplayTag(FName("Battle.Trigger.OnDeath"));
	GameplayTags.Battle_Trigger_EnemyLeftField = Manager.AddNativeGameplayTag(FName("Battle.Trigger.EnemyLeftField"));
	GameplayTags.Battle_Trigger_RestoreEnergy = Manager.AddNativeGameplayTag(FName("Battle.Trigger.RestoreEnergy"));
	GameplayTags.Battle_Trigger_SelfLeftField = Manager.AddNativeGameplayTag(FName("Battle.Trigger.SelfLeftField"));
	GameplayTags.Battle_Trigger_SelfHasBuff = Manager.AddNativeGameplayTag(FName("Battle.Trigger.SelfHasBuff"));
	GameplayTags.Battle_Trigger_EnemyHasBuffOrDebuff = Manager.AddNativeGameplayTag(FName("Battle.Trigger.EnemyHasBuffOrDebuff"));
}
