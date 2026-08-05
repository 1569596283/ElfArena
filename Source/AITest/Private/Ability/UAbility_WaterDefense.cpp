#include "Ability/UAbility_WaterDefense.h"

void UAbility_WaterDefense::ModifyEnergyCost(EInfoSide Side, const FSkillData& SkillData, int32& InOutCost) const
{
	if (SkillData.SkillType == ESkillType::Defense)
		InOutCost -= 2;
}
