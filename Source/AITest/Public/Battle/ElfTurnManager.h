#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ElfEnum.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "Elf/ElfManager.h"
#include "Data/ElfSkillData.h"
#include "ElfTurnManager.generated.h"

class UElfBattleController;
class UElfBattleModel;
class UElfBattleAI;
class UElfGameInstance;
class UElfSkillBase;
class UElfBuffManager;
struct FBattleSideData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnPhaseChanged, ETurnPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwitchRequested, EInfoSide, Side, int32, NextSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleEnded, EBattleResult, Result);

UCLASS()
class AITEST_API UElfTurnManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(UElfBattleController* InBC, UElfBattleModel* InBM);

	void StartTurn();

	UFUNCTION()
	void OnPlayerSkillSelected(int32 SlotIndex);

	UFUNCTION()
	void OnPlayerDefaultSkillSelected(int32 SlotIndex);

	void OnRemoteActionReceived(int32 SlotIndex);

	void OnPlayerSwitchRequest(int32 SlotIndex);

	UFUNCTION(BlueprintPure)
	ETurnPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure)
	bool IsWaitingForInput() const { return CurrentPhase == ETurnPhase::PlayerCommand; }

	int32 ChooseEnemySkill();

	void ChooseEnemyAction();

	// --- 增益减益系统 ---
	UElfBuffManager* GetBuffManager() const { return BuffManager; }

	void OnCreatureEnteredField(EInfoSide Side);

	void ApplyBuffToTarget(EInfoSide TargetSide, FName BuffDefRowName, const FEffectDef& Def, int32 OverrideStack = -1, int32 OverrideDuration = -1, bool bIsBuff = true);
	void ApplyBuffToSide(EInfoSide Side, FName BuffDefRowName, const FEffectDef& Def, int32 OverrideStack = -1, int32 OverrideDuration = -1, bool bIsBuff = true);

	UPROPERTY(BlueprintAssignable)
	FOnTurnPhaseChanged OnTurnPhaseChanged;

	UPROPERTY(BlueprintAssignable)
	FOnSwitchRequested OnSwitchRequested;

	UPROPERTY(BlueprintAssignable)
	FOnBattleEnded OnBattleEnded;

	int32 PlayerChosenSlot = -1;
	int32 EnemyChosenSlot = -1;
	int32 PlayerDefaultSlotIndex = -1;
	int32 EnemyDefaultSlotIndex = -1;
	bool bPlayerUsedDefault = false;
	bool bEnemyUsedDefault = false;

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

	void ApplyAttack(EInfoSide AttackerSide, int32 SlotIndex, EInfoSide TargetSide, float DamageModifier = 1.0f);
	void ApplyStatusEffects(const FTurnAction& Action, UElfSkillBase* SkillInstance);

	void EndTurn();

	void CheckDeath(EInfoSide Side);

	void EnterSwitchPhase(EInfoSide Side);

	void EndBattle(EBattleResult Result);

	FBattleSideData* GetSide(EInfoSide Side);
	FElfCreatureInstance* GetActiveCreature(EInfoSide Side);
	FElfCalculatedStats* GetActiveStats(EInfoSide Side);
	UElfSkillBase* GetActiveSkillInstance(EInfoSide Side, int32 SlotIndex) const;
	UElfSkillBase* GetActiveDefaultSkillInstance(EInfoSide Side, int32 SlotIndex) const;
	bool HasAliveCreatures(EInfoSide Side) const;
	int32 GetEffectiveSpeed(EInfoSide Side);
	int32 GetSkillPriorityFor(EInfoSide Side, int32 SlotIndex) const;
	UElfGameInstance* GetGameInstance() const;

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

	FTimerHandle ExecutionTimer;

	int32 PlayerFaintCount = 0;
	int32 EnemyFaintCount = 0;
};
