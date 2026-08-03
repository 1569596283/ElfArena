#pragma once

#include "GameplayTagContainer.h"

struct FElfGameplayTags
{
public:
    static const FElfGameplayTags& Get() { return GameplayTags; }
    static void InitializeNativeGameplayTags();

    FGameplayTag Input_Jump;

	FGameplayTag Input_Slot1;
	FGameplayTag Input_Slot2;
	FGameplayTag Input_Slot3;
	FGameplayTag Input_Slot4;
	FGameplayTag Input_Slot5;
	FGameplayTag Input_Slot6;

	FGameplayTag Input_Space;
	FGameplayTag Input_X;
	FGameplayTag Input_Q;
	FGameplayTag Input_W;
	FGameplayTag Input_E;
	FGameplayTag Input_R;
	FGameplayTag Input_Escape;

	// 触发时机事件（Battle.Trigger.*）
	FGameplayTag Battle_Trigger_EnterBattle;
	FGameplayTag Battle_Trigger_OnField;
	FGameplayTag Battle_Trigger_TurnStart;
	FGameplayTag Battle_Trigger_TurnEnd;
	FGameplayTag Battle_Trigger_DealSuperEffective;
	FGameplayTag Battle_Trigger_UseElementSkill;
	FGameplayTag Battle_Trigger_FirstAttack;
	FGameplayTag Battle_Trigger_TakeDamage;
	FGameplayTag Battle_Trigger_OnBench;
	FGameplayTag Battle_Trigger_OnDeath;
	FGameplayTag Battle_Trigger_EnemyLeftField;
	FGameplayTag Battle_Trigger_RestoreEnergy;
	FGameplayTag Battle_Trigger_SelfLeftField;
	FGameplayTag Battle_Trigger_SelfHasBuff;
	FGameplayTag Battle_Trigger_EnemyHasBuffOrDebuff;

private:
    static FElfGameplayTags GameplayTags;
};
