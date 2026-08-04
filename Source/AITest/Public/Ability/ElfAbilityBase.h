#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Data/ElfAbilityData.h"
#include "ElfAbilityBase.generated.h"

class UElfBattleController;
class UElfBattleModel;
class UElfBuffManager;
class UElfTurnManager;
struct FElfCreatureInstance;

UCLASS(BlueprintType)
class AITEST_API UElfAbilityBase : public UObject
{
	GENERATED_BODY()

public:
	// 配置特性：AbilityID + 触发时机 + 效果列表 + 延迟
	UFUNCTION(BlueprintCallable, Category = "特性")
	virtual void Init(const FName& InAbilityID, const FGameplayTag& InTrigger, const TArray<FSkillEffect>& InEffects, float InTriggerDelay = 0.0f);

	// 注入本局执行上下文（AbilityManager 初始化时调用）
	void SetContext(UElfBattleModel* InModel, UElfBuffManager* InBuffManager, UElfTurnManager* InTurnManager, UElfBattleController* InBattleController);

	UFUNCTION(BlueprintPure, Category = "特性")
	FName GetAbilityID() const { return AbilityID; }

	UFUNCTION(BlueprintPure, Category = "特性")
	FGameplayTag GetTrigger() const { return Trigger; }

	// 触发时机是否匹配（子类可重写，如 OnBench 要求自己不在场）
	virtual bool IsTriggerMatch(const FGameplayTag& EventTag) const { return Trigger == EventTag; }

	// 附加触发条件（概率/血量阈值等），返回 false 则不触发
	virtual bool CanTrigger(const FElfCreatureInstance* Creature) const;

	// 特性触发后的延迟秒数（>0 时等待该时间再进入下一阶段）
	float GetTriggerDelay() const { return TriggerDelay; }

	// 执行特性效果（默认遍历 Effects 执行通用效果，子类可重写）
	virtual void TriggerAbility(const FElfCreatureInstance* Creature);

	// 定位精灵所属侧（0=己方, 1=敌方）；找不到返回 -1
	int32 GetCreatureSide(const FElfCreatureInstance* Creature) const;

protected:
	// 通过上下文访问本局数据（弱引用，不参与 GC 强环）
	UPROPERTY()
	TObjectPtr<UElfBattleModel> BattleModel;

	UPROPERTY()
	TObjectPtr<UElfBuffManager> BuffManager;

	UPROPERTY()
	TObjectPtr<UElfTurnManager> TurnManager;

	UPROPERTY()
	TObjectPtr<UElfBattleController> BattleController;

	UPROPERTY()
	FName AbilityID;

	UPROPERTY()
	FGameplayTag Trigger;

	// 效果列表（来自 FAbilityData.Effects）
	UPROPERTY()
	TArray<FSkillEffect> Effects;

	// 触发延迟秒数（来自 FAbilityData.TriggerDelay）
	UPROPERTY()
	float TriggerDelay = 0.0f;
};
