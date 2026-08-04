#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "Data/ElfSkillData.h"
#include "Data/ElfBaseData.h"
#include "ElfAbilityData.generated.h"

class UElfAbilityBase;

USTRUCT(BlueprintType)
struct FAbilityData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "特性", meta = (DisplayName = "特性类"))
	TSubclassOf<class UElfAbilityBase> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "特性", meta = (DisplayName = "名称"))
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "特性", meta = (DisplayName = "描述"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "特性", meta = (DisplayName = "图标"))
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发", meta = (DisplayName = "触发时机", ToolTip = "Battle.Trigger.* 系列的 GameplayTag"))
	FGameplayTag Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件", meta = (DisplayName = "触发概率", ToolTip = "0.0~1.0，1=必定触发", ClampMin = "0.0", ClampMax = "1.0"))
	float TriggerChance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件", meta = (DisplayName = "血量阈值", ToolTip = "0.0~1.0，低于此比例时触发；0=不启用", ClampMin = "0.0", ClampMax = "1.0"))
	float HPThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件", meta = (DisplayName = "指定属性", ToolTip = "UseElementSkill 触发的技能属性；血脉类效果用"))
	EElfType TargetElement = EElfType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件", meta = (DisplayName = "指定增益行名", ToolTip = "SelfHasBuff / EnemyHasBuffOrDebuff 指定 Buff 行名"))
	FName BuffRowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "效果列表", ToolTip = "触发时执行的一次性效果/添加增益，复用技能效果风格"))
	TArray<FSkillEffect> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "触发延迟秒", ToolTip = ">0 时特性完成后等待该秒数再进入下一阶段（用于播动画）；<=0 不延迟"))
	float TriggerDelay = 0.0f;
};

