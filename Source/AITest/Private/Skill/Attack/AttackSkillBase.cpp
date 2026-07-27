#include "Skill/Attack/AttackSkillBase.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Game/ElfGameInstance.h"
#include "Data/ElfSkillData.h"

int32 UAttackSkillBase::CalculateDamage(UElfBattleController* BattleController, int32 SlotIndex) const
{
	if (!BattleController) return 0;

	FElfCreatureInstance* Self = BattleController->GetBattleModel()->PlayerSide.GetActiveCreature();
	FElfCreatureInstance* Enemy = BattleController->GetBattleModel()->EnemySide.GetActiveCreature();
	FElfCalculatedStats* SelfStats = BattleController->GetBattleModel()->PlayerSide.GetActiveStats();
	FElfCalculatedStats* EnemyStats = BattleController->GetBattleModel()->EnemySide.GetActiveStats();
	if (!Self || !Enemy || !SelfStats || !EnemyStats) return 0;

	if (!Self->EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = BattleController->GetOwnerPC() ? BattleController->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	FName SkillRowName = Self->EquippedSkills[SlotIndex];
	if (!GI->GetSkillData(SkillRowName, SkillData)) return 0;

	int32 BaseDamage = 0;
	for (const FSkillEffect& Effect : SkillData.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			BaseDamage = FMath::RoundToInt(Effect.Value);
			break;
		}
	}
	float TypeEffectiveness = 1.0f;
	return FMath::Max(0, BaseDamage);
}

int32 UAttackSkillBase::GetHitCount(UElfBattleController* BattleController, int32 SlotIndex) const
{
	if (!BattleController) return 1;

	FElfCreatureInstance Creature = BattleController->GetSelfCreature(0);
	if (!Creature.EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = BattleController->GetOwnerPC() ? BattleController->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 1;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature.EquippedSkills[SlotIndex], SkillData)) return 1;

	int32 BasePower = 0;
	for (const FSkillEffect& Effect : SkillData.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			BasePower = FMath::RoundToInt(Effect.Value);
			break;
		}
	}
	return FMath::Max(1, BasePower > 0 ? BasePower / 20 + 1 : 1);
}

int32 UAttackSkillBase::CalculateInstanceDamage(const FElfCalculatedStats& Attacker, const FElfCalculatedStats& Defender) const
{
	int32 BasePower = 0;
	for (const FSkillEffect& Effect : SkillDataRef.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			BasePower = FMath::RoundToInt(Effect.Value);
			break;
		}
	}
	if (BasePower <= 0) return 0;

	int32 Damage = 0;
	if (SkillDataRef.DamageType == EDamageType::Physical)
	{
		Damage = FMath::RoundToInt(BasePower * static_cast<float>(Attacker.ATK) / static_cast<float>(FMath::Max(1, Defender.DEF)));
	}
	else
	{
		Damage = FMath::RoundToInt(BasePower * static_cast<float>(Attacker.MATK) / static_cast<float>(FMath::Max(1, Defender.MDEF)));
	}
	return FMath::Max(1, Damage);
}

int32 UAttackSkillBase::GetInstanceHitCount() const
{
	int32 BasePower = 0;
	for (const FSkillEffect& Effect : SkillDataRef.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			BasePower = FMath::RoundToInt(Effect.Value);
			break;
		}
	}
	return FMath::Max(1, BasePower > 0 ? BasePower / 20 + 1 : 1);
}
