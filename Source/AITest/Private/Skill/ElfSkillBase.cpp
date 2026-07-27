#include "Skill/ElfSkillBase.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Game/ElfGameInstance.h"
#include "Data/ElfSkillData.h"

int32 UElfSkillBase::GetEnergyCost(UElfBattleController* BattleController, int32 SlotIndex) const
{
	if (!BattleController) return 0;

	FElfCreatureInstance Creature = BattleController->GetSelfCreature(0);
	if (!Creature.EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = BattleController->GetOwnerPC() ? BattleController->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature.EquippedSkills[SlotIndex], SkillData)) return 0;

	return SkillData.EnergyCost;
}

FText UElfSkillBase::GetDescription(UElfBattleController* BattleController, int32 SlotIndex) const
{
	if (!BattleController) return FText::GetEmpty();

	FElfCreatureInstance Creature = BattleController->GetSelfCreature(0);
	if (!Creature.EquippedSkills.IsValidIndex(SlotIndex)) return FText::GetEmpty();

	UElfGameInstance* GI = BattleController->GetOwnerPC() ? BattleController->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return FText::GetEmpty();

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature.EquippedSkills[SlotIndex], SkillData)) return FText::GetEmpty();

	return SkillData.Description;
}
