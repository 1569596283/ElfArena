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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "特性", meta = (DisplayName = "显示名称"))
	FText DisplayName;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件", meta = (DisplayName = "能耗条件", ToolTip = ">=0 时，仅能耗等于该值的技能触发特性增伤（配合 Effects 里的 Power 威力增幅）；-1=不启用"))
	int32 EnergyCostCondition = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件", meta = (DisplayName = "总技能能耗阈值", ToolTip = ">=0 时，入场特性仅在持有者装备技能总能耗小于该值时触发；-1=不启用"))
	int32 TotalCostThreshold = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件", meta = (DisplayName = "团队被动", ToolTip = "己方任意精灵触发该时机都算（不要求本精灵），效果作用于特性持有者（可在场下）；默认仅持有者自身触发才生效"))
	bool bTeamTrigger = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "初始能量为0", ToolTip = "进战斗满能量恢复后，该精灵初始能量置 0"))
	bool bStartWithZeroEnergy = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "死亡不消耗魔力值", ToolTip = "持有者被击倒时，该侧魔力值不扣除（如 特性 牺牲）"))
	bool bNoMagicCostOnDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "能量防御", ToolTip = "每有 1 能量获得双防 +10%（按当前能量实时更新 buff 层数）"))
	bool bEnergyDefense = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "毒疫", ToolTip = "在场时，双方回合结束的中毒效果额外触发 1 次"))
	bool bPoisonExtraTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "效果列表", ToolTip = "触发时执行的一次性效果/添加增益，复用技能效果风格"))
	TArray<FSkillEffect> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "触发延迟秒", ToolTip = ">0 时特性完成后等待该秒数再进入下一阶段（用于播动画）；<=0 不延迟"))
	float TriggerDelay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "不弹提示", ToolTip = "被动特性（如常驻威力增幅）不广播播报提示"))
	bool bNoPrompt = false;
};

