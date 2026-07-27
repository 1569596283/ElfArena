#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/ElfSkillData.h"
#include "ElfSkillBase.generated.h"

class UElfBattleController;

UCLASS(Abstract, BlueprintType)
class AITEST_API UElfSkillBase : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FSkillData& InData) { SkillDataRef = InData; }

	UFUNCTION(BlueprintCallable, Category = "技能")
	virtual int32 GetEnergyCost(UElfBattleController* BattleController, int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "技能")
	virtual FText GetDescription(UElfBattleController* BattleController, int32 SlotIndex) const;

	virtual int32 GetInstanceEnergyCost() const { return SkillDataRef.EnergyCost; }
	virtual FText GetInstanceDescription() const { return SkillDataRef.Description; }
	virtual void OnSkillUsed() { UseCount++; }

	UFUNCTION(BlueprintPure, Category = "技能")
	int32 GetUseCount() const { return UseCount; }

	UFUNCTION(BlueprintPure, Category = "技能")
	FSkillData GetSkillDataRef() const { return SkillDataRef; }

protected:
	UPROPERTY()
	FSkillData SkillDataRef;

	UPROPERTY()
	int32 UseCount = 0;
};
