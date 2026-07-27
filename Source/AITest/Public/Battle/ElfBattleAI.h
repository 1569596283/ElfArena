#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ElfEnum.h"
#include "Data/ElfBaseData.h"
#include "Data/ElfSkillData.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "ElfBattleAI.generated.h"

class UElfBattleController;
class UElfBattleModel;
class UElfGameInstance;
class UElfSkillBase;
struct FBattleSideData;
struct FElfCreatureInstance;
struct FElfCalculatedStats;
struct FSkillData;

UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class AITEST_API UElfBattleAI : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "AI 权重", meta = (ClampMin = "0", ClampMax = "1"))
	float SuperEffectiveWeight = 0.7f;

	UPROPERTY(EditAnywhere, Category = "AI 权重", meta = (ClampMin = "0", ClampMax = "1"))
	float BuffOrEnergyWeight = 0.5f;

	UPROPERTY(EditAnywhere, Category = "AI 权重", meta = (ClampMin = "0", ClampMax = "1"))
	float DefenseWhenWeakWeight = 0.5f;

	UPROPERTY(EditAnywhere, Category = "AI 权重", meta = (ClampMin = "0", ClampMax = "1"))
	float SwitchWhenWeakWeight = 0.3f;

	int32 ChooseSkill(EInfoSide SelfSide, EInfoSide EnemySide,
		UElfBattleController* BC, UElfBattleModel* BM, UElfGameInstance* GI);

	int32 ChooseSwitch(EInfoSide SelfSide, EInfoSide EnemySide,
		UElfBattleModel* BM, UElfGameInstance* GI);

protected:
	struct FSkillInfo
	{
		int32 SlotIndex;
		UElfSkillBase* Instance;
		EElfType ElementType;
		ESkillType SkillType;
		bool bIsDefault;
	};

	void FillSkillList(TArray<FSkillInfo>& OutSkills, EInfoSide Side,
		UElfBattleController* BC, UElfBattleModel* BM);

	float GetTypeEffectiveness(EElfType AttackType, EElfType DefendType1, EElfType DefendType2,
		UElfGameInstance* GI) const;

	int32 PickRandomIndex(const TArray<int32>& Indices) const;
};
