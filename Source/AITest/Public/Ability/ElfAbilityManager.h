#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "ElfAbilityManager.generated.h"

class UElfBattleController;
class UElfBattleModel;
class UElfAbilityBase;
class UElfBuffManager;
class UElfTurnManager;
class UElfEventManager;
class UElfGameInstance;
struct FElfCreatureInstance;

// 特性触发序列完成回调（含延迟等待结束后）
DECLARE_MULTICAST_DELEGATE(FOnAllAbilitiesTriggered);

UCLASS()
class AITEST_API UElfAbilityManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(UElfBattleController* InBC, UElfBattleModel* InBM);

	// 查询该精灵进战斗初始能量是否为 0（读取其特性配置 bStartWithZeroEnergy）
	static bool ShouldStartWithZeroEnergy(UElfGameInstance* GI, const FName& CreatureRowName);

	// 触发指定侧单只精灵的入场特性（手动切换：立即触发）
	void TriggerEnter(EInfoSide Side);

	// 触发战斗开始双方入场特性（按速度排序，快者先）
	void TriggerEnterBattle();

	// 触发指定侧换将入场特性（阵亡换将：等双方换完后再调用，只有换将侧触发）
	void TriggerEnterForced(EInfoSide Side);

	// 触发序列完成回调（所有特性的延迟等待结束后广播）
	FOnAllAbilitiesTriggered OnAllAbilitiesTriggered;

protected:
	// 能耗类 buff 变化时实时重算"总能耗阈值"特性（动态加/移除条件buff）
	void OnEnergyCostBuffChanged(EInfoSide Side);

	// 能量变化时实时刷新"能量防御"特性（buff 层数 = 当前能量）
	UFUNCTION()
	void OnSelfEnergyChanged(int32 Energy);

	UFUNCTION()
	void OnEnemyEnergyChanged(int32 Energy);

	void RefreshEnergyDefenseTraits(EInfoSide Side);

	// 重算指定侧在场精灵的"总能耗阈值"特性
	void RefreshTotalCostTraits(EInfoSide Side);

	// 创建所有特性实例并注入上下文
	void CreateAbilityInstances();

	// 监听全局事件总线
	void HandleGameplayEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature);

	void TriggerByEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature);

	// 单只精灵触发入场特性
	void TriggerCreatureEnter(const FElfCreatureInstance* Creature);

	// 获取某精灵入场特性的触发延迟（无特性/无延迟返回 0）
	float GetCreatureEnterDelay(const FElfCreatureInstance* Creature) const;

	// 收集本轮触发过的延迟，异步等待最长延迟后广播完成
	void HandleTriggerCompletion(const FElfCreatureInstance* Creature);

	int32 GetCreatureSpeed(const FElfCreatureInstance* Creature) const;

	void NotifyAllTriggered();

	UPROPERTY()
	TObjectPtr<UElfBattleController> BattleController;

	UPROPERTY()
	TObjectPtr<UElfBattleModel> BattleModel;

	UPROPERTY()
	TObjectPtr<UElfBuffManager> BuffManager;

	UPROPERTY()
	TObjectPtr<UElfTurnManager> TurnManager;

	UPROPERTY()
	TObjectPtr<UElfEventManager> EventManager;

	// 当前触发序列的最大延迟
	float PendingMaxDelay = 0.0f;

	// 当前触发序列是否在等待延迟
	bool bWaitingDelay = false;

	FTimerHandle DelayTimerHandle;

	// 双方入场特性触发之间的定时器
	FTimerHandle EnterSecondTimerHandle;
};
