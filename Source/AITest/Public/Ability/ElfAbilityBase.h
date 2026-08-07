#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Misc/Guid.h"
#include "Data/ElfAbilityData.h"
#include "ElfAbilityBase.generated.h"

class UElfBattleController;
class UElfBattleModel;
class UElfBuffManager;
class UElfTurnManager;
struct FElfCreatureInstance;
enum class EInfoSide : uint8;

UCLASS(BlueprintType)
class AITEST_API UElfAbilityBase : public UObject
{
	GENERATED_BODY()

public:
	// 配置特性：AbilityID + 触发时机 + 效果列表 + 延迟
	UFUNCTION(BlueprintCallable, Category = "特性")
	virtual void Init(const FName& InAbilityID, const FGameplayTag& InTrigger, const TArray<FSkillEffect>& InEffects, float InTriggerDelay = 0.0f);

	// 配置触发条件（TriggerChance / HPThreshold / TargetElement / EnergyCostCondition，来自 FAbilityData）
	void SetTriggerConditions(float InTriggerChance, float InHPThreshold, EElfType InTargetElement, int32 InEnergyCostCondition = -1);

	// 配置团队被动 / 初始能量0 / 持有者唯一ID（AbilityManager 创建实例时调用）
	void SetTeamConfig(bool InTeamTrigger, bool InStartWithZeroEnergy, const FGuid& InOwnerCreatureID);

	// 配置总技能能耗阈值（<0 不启用；>=0 时入场仅当持有者装备技能总能耗小于该值才触发）
	void SetTotalCostThreshold(int32 InThreshold);

	// 配置死亡时不消耗魔力值（特性 牺牲）
	void SetNoMagicCostOnDeath(bool InValue);

	// 死亡时不消耗魔力值（特性 牺牲）
	bool IsNoMagicCostOnDeath() const { return bNoMagicCostOnDeath; }

	// 配置能量防御（每 1 能量双防 +10%，按当前能量实时更新 buff 层数）
	void SetEnergyDefense(bool InValue);

	// 能量防御特性
	bool IsEnergyDefense() const { return bEnergyDefense; }

	// 配置毒疫（在场时双方中毒额外触发 1 次）
	void SetPoisonExtraTick(bool InValue);

	// 毒疫特性
	bool IsPoisonExtraTick() const { return bPoisonExtraTick; }

	// 总技能能耗阈值（<0 不启用）
	int32 GetTotalCostThreshold() const { return TotalCostThreshold; }

	// 特性效果里施加的第一个增益/减益的行名（供"条件buff"类特性动态加/移除用）
	FName GetConditionBuffRowName() const;

	// 团队被动：己方任意精灵触发该时机都算（效果作用于持有者）
	bool IsTeamTrigger() const { return bTeamTrigger; }

	// 初始能量为 0（进战斗满能量恢复后置 0）
	bool ShouldStartWithZeroEnergy() const { return bStartWithZeroEnergy; }

	// 特性持有者精灵的唯一 ID
	const FGuid& GetOwnerCreatureID() const { return OwnerCreatureID; }

	// 查找持有者精灵（按 CreatureID，可能在场下；找到返回非空并写出所在侧/队伍索引）
	FElfCreatureInstance* FindOwnerCreature(int32& OutSideIndex, int32& OutTeamIndex) const;

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

	// 技能威力增幅倍率（伤害计算时由攻击方特性调用；子类/配置可重写，如"能耗1技能威力+50%"）
	virtual float ModifySkillPower(EInfoSide Side, const FSkillData& SkillData) const;

	// 技能能耗修正（子类可重写，如 水系：防御技能能耗-2）
	virtual void ModifyEnergyCost(EInfoSide Side, const FSkillData& SkillData, int32& InOutCost) const;

	// 防守方特性对受到的攻击伤害的修正倍率（基类返回 1.0；子类如 受到自身携带技能系别攻击 -40%）
	virtual float ModifyIncomingDamage(EInfoSide DefenderSide, const FSkillData& SkillData) const;

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

	// 触发概率（0~1，1=必定触发）
	UPROPERTY()
	float TriggerChance = 1.0f;

	// 血量阈值（0~1，低于此比例触发；0=不启用）
	UPROPERTY()
	float HPThreshold = 0.0f;

	// 指定属性（UseElementSkill 触发时匹配的技能属性）
	UPROPERTY()
	EElfType TargetElement = EElfType::None;

	// 能耗条件（>=0 时仅该能耗的技能触发增伤，-1=不启用）
	UPROPERTY()
	int32 EnergyCostCondition = -1;

	// 总技能能耗阈值（>=0 时入场仅当持有者装备技能总能耗小于该值才触发，-1=不启用）
	UPROPERTY()
	int32 TotalCostThreshold = -1;

	// 死亡时不消耗魔力值（特性 牺牲）
	UPROPERTY()
	bool bNoMagicCostOnDeath = false;

	// 能量防御（每 1 能量双防 +10%）
	UPROPERTY()
	bool bEnergyDefense = false;

	// 毒疫（在场时双方中毒额外触发 1 次）
	UPROPERTY()
	bool bPoisonExtraTick = false;

	// 不弹提示（被动特性用）
	UPROPERTY()
	bool bNoPrompt = false;

	// 团队被动：己方任意精灵触发该时机都算（效果作用于特性持有者）
	UPROPERTY()
	bool bTeamTrigger = false;

	// 初始能量为 0（进战斗满能量恢复后置 0）
	UPROPERTY()
	bool bStartWithZeroEnergy = false;

	// 特性持有者精灵的唯一 ID（CreateAbilityInstances 时记录，用于在场下定位持有者）
	UPROPERTY()
	FGuid OwnerCreatureID;

public:
	bool IsNoPrompt() const { return bNoPrompt; }
	void SetNoPrompt(bool InNoPrompt) { bNoPrompt = InNoPrompt; }
};
