#pragma once

#include "CoreMinimal.h"
#include "Ability/ElfAbilityBase.h"
#include "UAbility_WaterDefense.generated.h"

// 水系特性：携带的防御技能能耗 -2
UCLASS()
class AITEST_API UAbility_WaterDefense : public UElfAbilityBase
{
	GENERATED_BODY()

public:
	virtual void ModifyEnergyCost(EInfoSide Side, const FSkillData& SkillData, int32& InOutCost) const override;
};
