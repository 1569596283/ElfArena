#include "Skill/Defensive/DefensiveSkillBase.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Game/ElfGameInstance.h"
#include "Data/ElfSkillData.h"

float UDefensiveSkillBase::GetDamageReduction(UElfBattleController* BattleController, int32 SlotIndex) const
{
	if (!BattleController) return 0.0f;

	FElfCreatureInstance Creature = BattleController->GetSelfCreature(0);
	if (!Creature.EquippedSkills.IsValidIndex(SlotIndex)) return 0.0f;

	UElfGameInstance* GI = BattleController->GetOwnerPC() ? BattleController->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0.0f;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature.EquippedSkills[SlotIndex], SkillData)) return 0.0f;

	for (const FSkillEffect& Effect : SkillData.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			return FMath::Clamp(Effect.Value / 100.0f, 0.0f, 1.0f);
		}
	}
	return 0.0f;
}

float UDefensiveSkillBase::GetInstanceDamageReduction() const
{
	for (const FSkillEffect& Effect : SkillDataRef.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			return FMath::Clamp(Effect.Value / 100.0f, 0.0f, 1.0f);
		}
	}
	return 0.0f;
}
