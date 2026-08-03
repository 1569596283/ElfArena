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
struct FElfCreatureInstance;

UCLASS()
class AITEST_API UElfAbilityManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(UElfBattleController* InBC, UElfBattleModel* InBM);

	// 触发指定侧单只精灵的入场特性（手动切换：立即触发）
	void TriggerEnter(EInfoSide Side);

	// 触发战斗开始双方入场特性（按速度排序，快者先）
	void TriggerEnterBattle();

	// 触发指定侧换将入场特性（阵亡换将：等双方换完后再调用，只有换将侧触发）
	void TriggerEnterForced(EInfoSide Side);

protected:
	// 创建所有特性实例并注入上下文
	void CreateAbilityInstances();

	// 监听全局事件总线
	void HandleGameplayEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature);

	void TriggerByEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature);

	// 单只精灵触发入场特性
	void TriggerCreatureEnter(const FElfCreatureInstance* Creature);

	int32 GetCreatureSpeed(const FElfCreatureInstance* Creature) const;

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
};
