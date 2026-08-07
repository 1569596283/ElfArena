#pragma once

#include "CoreMinimal.h"
#include "Ability/ElfAbilityBase.h"
#include "UAbility_TypeResist.generated.h"

// 属性亲和：受到自己携带技能系别（排除默认技能）的攻击伤害 -40%
UCLASS()
class AITEST_API UAbility_TypeResist : public UElfAbilityBase
{
	GENERATED_BODY()

public:
	virtual float ModifyIncomingDamage(EInfoSide DefenderSide, const FSkillData& SkillData) const override;
};
