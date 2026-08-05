#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ElfEnum.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "Elf/ElfManager.h"
#include "Data/ElfSkillData.h"
#include "Data/ElfItemData.h"
#include "ElfTurnManager.generated.h"

class UElfBattleController;
class UElfBattleModel;
class UElfBattleAI;
class UElfGameInstance;
class UElfSkillBase;
class UElfBuffManager;
class UElfEventManager;
struct FBattleSideData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnPhaseChanged, ETurnPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwitchRequested, EInfoSide, Side, int32, NextSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnForcedSwitchRequested, EInfoSide, Side, int32, NextSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnForcedSwitchAllComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleEnded, EBattleResult, Result);

UCLASS()
class AITEST_API UElfTurnManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(UElfBattleController* InBC, UElfBattleModel* InBM);

	void StartTurn();

	// 进入精灵入场阶段（UI 隐藏按钮/禁用输入，等待特性触发完成）
	void BeginEnterPhase() { ChangePhase(ETurnPhase::EnterPhase); }

	UFUNCTION()
	void OnPlayerSkillSelected(int32 SlotIndex);

	UFUNCTION()
	void OnPlayerDefaultSkillSelected(int32 SlotIndex);

	UFUNCTION()
	void OnCaptureConfirmed();

	void OnRemoteActionReceived(int32 SlotIndex);

	UFUNCTION()
	void OnPlayerSwitchRequest(int32 SlotIndex);

	UFUNCTION()
	void OnPlayerRunRequest();

	UFUNCTION(BlueprintPure)
	ETurnPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure)
	bool IsWaitingForInput() const { return CurrentPhase == ETurnPhase::PlayerDecision; }

	int32 ChooseEnemySkill();

	void ChooseEnemyAction();

	// --- 增益减益系统 ---
	UElfBuffManager* GetBuffManager() const { return BuffManager; }

	// 集中计算技能实际能耗（buff 修正 + 在场精灵特性修正，如 水系：防御技能能耗-2）
	int32 GetSkillEnergyCost(EInfoSide Side, UElfSkillBase* SkillInstance) const;
	// 槽位版：bIsDefault=true 取默认技能实例
	int32 GetSkillEnergyCost(EInfoSide Side, int32 SlotIndex, bool bIsDefault) const;

	void OnCreatureEnteredField(EInfoSide Side);

	// BattleManager 切换完成后回调
	void OnForcedSwitchComplete();

	// 迅捷
	bool bSwiftDone = false;

	// 捕捉
	bool bCaptureAttempted = false;
	void ProcessCapture();

	// 离场机制
	bool bForceSwitchPending = false;
	EInfoSide ForceSwitchSides[2];
	int32 ForceSwitchSideCount = 0;
	int32 ForceSwitchCompleted = 0;

	void ProcessPendingEvolutions();

	void ApplyBuffToTarget(EInfoSide TargetSide, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack = -1, int32 OverrideDuration = -1, bool bIsBuff = true);
	void ApplyBuffToSide(EInfoSide Side, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack = -1, int32 OverrideDuration = -1, bool bIsBuff = true);

	UPROPERTY(BlueprintAssignable)
	FOnTurnPhaseChanged OnTurnPhaseChanged;

	UPROPERTY(BlueprintAssignable)
	FOnSwitchRequested OnSwitchRequested;

	UPROPERTY(BlueprintAssignable)
	FOnForcedSwitchRequested OnForcedSwitchRequested;

	UPROPERTY(BlueprintAssignable)
	FOnForcedSwitchAllComplete OnForcedSwitchAllComplete;

	UPROPERTY(BlueprintAssignable)
	FOnBattleEnded OnBattleEnded;

	int32 PlayerChosenSlot = -1;
	int32 EnemyChosenSlot = -1;
	int32 PlayerDefaultSlotIndex = -1;
	int32 EnemyDefaultSlotIndex = -1;
	bool bPlayerUsedDefault = false;
	bool bEnemyUsedDefault = false;

	// ===== 道具 / 捕捉状态机（由 BattleController 迁移而来） =====
	void InitCaptureItemQuantities();
	bool UseItem(FName ItemRowName);
	bool CanUseBattleItem(FName ItemRowName) const;
	int32 GetItemRemainingUses(FName ItemRowName) const;
	bool IsItemCompatibleWithCreature(FName ItemRowName) const;
	FName FindBattleItemRowName(EEffectID EffectID);
	void UseBattleItem();
	FName GetBattleItemRowName();
	int32 GetBattleItemCount();
	FName GetBattleItemAtSlot(int32 FlatIndex);
	const TArray<FName>& GetBattleItemList();
	void UseCaptureItem(int32 FlatIndex);
	int32 GetCaptureItemCount();
	FName GetCaptureItemAtSlot(int32 FlatIndex);
	const TArray<FName>& GetCaptureItemList();
	int32 GetCaptureItemQuantity(FName ItemRowName) const;
	void ConsumePendingItem();
	void CancelWish();
	void RefundItem(FName ItemRowName);
	void ResetBattleItemState();
	void ClearCapturePending();
	bool IsCapturePending() const { return bCapturePending; }
	float GetCaptureBallRate() const { return PendingCaptureBallRate; }
	bool IsBattleItemUsedThisTurn() const { return bItemUsedThisTurn; }

protected:
	enum class ECounterState : uint8
	{
		None,
		FirstCountersSecond,
		SecondCountersFirst
	};

	struct FTurnAction
	{
		EInfoSide Side;
		int32 SlotIndex;
		bool bIsDefault = false;
	};

	void ProcessForcedSwitches();
	void ResumeAfterForcedSwitch();
	bool HasAliveBackup(EInfoSide Side) const;

	// 迅捷
	void TryExecuteSwiftSkills();
	void ExecuteSwiftSkill(EInfoSide Side, int32 SkillSlotIndex);
	void OnSwiftSkillDone();
	int32 PickRandomAliveCreature(EInfoSide Side) const;
	void CheckSkillForcedSwitch(const FTurnAction& Action, UElfSkillBase* SkillInstance);

	UFUNCTION()
	void ChangePhase(ETurnPhase NewPhase);

	void OnPlayerActionReady();

	void ResolveActions();

	UFUNCTION()
	void OnExecutionTimer();

	ECounterState DetermineCounter(const FTurnAction& A, const FTurnAction& B) const;
	bool IsCounteredBy(const FTurnAction& Target, const FTurnAction& Counter) const;
	float GetCounterModifier(const FTurnAction& Counter) const;
	void ExecuteSingleAction(const FTurnAction& Action, float DamageModifier = 1.0f);
	void ExecuteTurnAction(const FTurnAction& Action);

	void ProcessNextAction();
	void BeginActionPipeline();
	FName GetActionSkillRowName(const FTurnAction& Action);

	void ApplyAttack(EInfoSide AttackerSide, int32 SlotIndex, EInfoSide TargetSide, float DamageModifier = 1.0f);
	void ApplyStatusEffects(const FTurnAction& Action, UElfSkillBase* SkillInstance);

	// 集中计算攻击方总伤害增幅倍率（buff 威力 + 直接伤害乘区 + 攻击方特性增伤），伤害公式处统一乘
	float GetAttackerDamageMultiplier(EInfoSide AttackerSide, EInfoSide TargetSide, const FSkillData& SkillData) const;

	void EndTurn();

	void CheckDeath(EInfoSide Side);

	void EnterSwitchPhase(EInfoSide Side);

	void EndBattle(EBattleResult Result);

	FBattleSideData* GetSide(EInfoSide Side);
	FElfCreatureInstance* GetActiveCreature(EInfoSide Side);
	FElfCalculatedStats* GetActiveStats(EInfoSide Side);
	UElfSkillBase* GetActiveSkillInstance(EInfoSide Side, int32 SlotIndex) const;
	UElfSkillBase* GetActiveDefaultSkillInstance(EInfoSide Side, int32 SlotIndex) const;
	// 按行动取技能实例（默认技能取默认实例，否则取装备实例）
	UElfSkillBase* GetActionSkillInstance(const FTurnAction& Action) const;
	bool HasAliveCreatures(EInfoSide Side) const;
	int32 GetEffectiveSpeed(EInfoSide Side);
	int32 GetSkillPriorityFor(EInfoSide Side, int32 SlotIndex) const;
	UElfGameInstance* GetGameInstance() const;

	UElfEventManager* GetEventManager() const;

	UPROPERTY()
	TObjectPtr<UElfBattleController> BattleController;

	UPROPERTY()
	TObjectPtr<UElfBattleModel> BattleModel;

	UPROPERTY()
	TObjectPtr<UElfBattleAI> BattleAI;

	UPROPERTY()
	TObjectPtr<UElfBuffManager> BuffManager;

	ETurnPhase CurrentPhase = ETurnPhase::None;

	bool bLocalActionChosen = false;
	bool bRemoteActionChosen = false;

	TArray<FTurnAction> ActionQueue;

	// 逐行动作队列：DisplayActions=文字提示顺序（被应对→应对），ExecuteActions=生效顺序（应对→被应对）
	TArray<FTurnAction> DisplayActions;
	TArray<FTurnAction> ExecuteActions;
	int32 CurrentActionIndex = 0;
	bool bInDisplayPhase = false;
	bool bInExecutePhase = false;
	bool bActionSetupDone = false;

	FTimerHandle ExecutionTimer;

	int32 PlayerFaintCount = 0;
	int32 EnemyFaintCount = 0;

	// 道具 / 捕捉状态（迁移自 BattleController）
	UPROPERTY()
	TMap<FName, int32> ItemRemainingUses;

	UPROPERTY()
	bool bItemUsedThisTurn = false;

	UPROPERTY()
	FName PendingItemRowName;

	UPROPERTY()
	bool bCapturePending = false;

	UPROPERTY()
	float PendingCaptureBallRate = 0.0f;

	UPROPERTY()
	TMap<FName, int32> CaptureItemQuantities;

	TArray<FName> CachedCaptureItemList;
	TArray<FName> CachedBattleItemList;
	int32 LastBattleItemActiveIndex = -1;

	FName CachedWishRowName;
	FName CachedEvoRowName;
};
