#pragma once

#include "CoreMinimal.h"
#include "Skill/ElfSkillBase.h"
#include "DefensiveSkillBase.generated.h"

class UElfBattleController;

UCLASS(BlueprintType)
class AITEST_API UDefensiveSkillBase : public UElfSkillBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "技能")
	float GetDamageReduction(UElfBattleController* BattleController, int32 SlotIndex) const;

	virtual float GetInstanceDamageReduction() const;
};
