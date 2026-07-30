#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "ElfBaseData.generated.h"

class AElfWorldBase;
class AElfBattleBase;

UENUM(BlueprintType)
enum class EElfType : uint8
{
	None 	 UMETA(DisplayName = "无"),
	Normal   UMETA(DisplayName = "普通"),
	Fire     UMETA(DisplayName = "火"),
	Water    UMETA(DisplayName = "水"),
	Grass    UMETA(DisplayName = "草"),
	Electric UMETA(DisplayName = "电"),
	Earth    UMETA(DisplayName = "地"),
	Wind     UMETA(DisplayName = "风"),
	Ice      UMETA(DisplayName = "冰"),
	Dark     UMETA(DisplayName = "暗"),
	Light    UMETA(DisplayName = "光"),
	Leader   UMETA(DisplayName = "首领")
};

USTRUCT(BlueprintType)
struct FSskillLearnCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "技能学习")
	int32 UnlockLevel = 0;

	UPROPERTY(EditAnywhere, Category = "技能学习")
	FName SkillID;

	UPROPERTY(EditAnywhere, Category = "技能学习")
	bool bNeedSkillStone = false;
};

USTRUCT(BlueprintType)
struct FEvolutionCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "进化")
	int32 RequiredLevel = 0;

	UPROPERTY(EditAnywhere, Category = "进化")
	TSoftObjectPtr<class UTexture2D> EvolutionItem;
};

USTRUCT(BlueprintType)
struct FElfCalculatedStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxHP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ATK = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MATK = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DEF = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MDEF = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SPD = 0;
};

USTRUCT(BlueprintType)
struct FElfBaseData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础信息", meta = (DisplayName = "名称"))
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础信息", meta = (DisplayName = "描述"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础信息", meta = (DisplayName = "图标"))
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础信息", meta = (DisplayName = "系别一"))
	EElfType Type1 = EElfType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础信息", meta = (DisplayName = "系别二"))
	EElfType Type2 = EElfType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础信息", meta = (DisplayName = "血脉属性", ToolTip = "默认与系别一相同。设为Leader时不可使用愿力，可进行超进化"))
	EElfType Type3 = EElfType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "蓝图", meta = (DisplayName = "大世界蓝图"))
	TSoftClassPtr<class AElfWorldBase> WorldBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "蓝图", meta = (DisplayName = "战斗蓝图"))
	TSoftClassPtr<class AElfBattleBase> BattleBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "种族值", meta = (DisplayName = "生命"))
	int32 BaseHP = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "种族值", meta = (DisplayName = "物理攻击"))
	int32 BaseATK = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "种族值", meta = (DisplayName = "魔法攻击"))
	int32 BaseMATK = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "种族值", meta = (DisplayName = "物理防御"))
	int32 BaseDEF = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "种族值", meta = (DisplayName = "魔法防御"))
	int32 BaseMDEF = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "种族值", meta = (DisplayName = "速度"))
	int32 BaseSPD = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "可学技能"))
	TArray<FSskillLearnCondition> LearnableSkills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "进化", meta = (DisplayName = "进化目标"))
	FDataTableRowHandle EvolutionTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "进化", meta = (DisplayName = "进化条件"))
	TArray<FEvolutionCondition> EvolutionConditions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "特性", meta = (DisplayName = "特性ID"))
	FName AbilityID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "捕捉", meta = (DisplayName = "捕捉难度"))
	float CaptureDifficulty = 1.0f;
};
