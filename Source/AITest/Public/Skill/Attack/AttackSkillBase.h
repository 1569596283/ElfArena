#pragma once

#include "CoreMinimal.h"
#include "Skill/ElfSkillBase.h"
#include "Data/ElfBaseData.h"
#include "AttackSkillBase.generated.h"

class UElfBattleController;

UCLASS(BlueprintType)
class AITEST_API UAttackSkillBase : public UElfSkillBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "技能")
	int32 CalculateDamage(UElfBattleController* BattleController, int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "技能")
	int32 GetHitCount(UElfBattleController* BattleController, int32 SlotIndex) const;

	virtual int32 CalculateInstanceDamage(const FElfCalculatedStats& Attacker, const FElfCalculatedStats& Defender) const;
	virtual int32 GetInstanceHitCount() const;
};
