#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "ElfEnum.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "Data/ElfSkillData.h"
#include "Battle/ElfTurnManager.h"
#include "ElfBattleController.generated.h"

class UElfBattleModel;
class UUserWidget;
class UElfTurnManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIntroComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCameraRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreatureSelected, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerReadyStateChanged, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreatureSwitched, EInfoSide, Side);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, int32, NewHP, int32, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnergyChanged, int32, NewEnergy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageAnimStep, int32, FromHP, int32, ToHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillSelected, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefaultSkillSelected, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleActionPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputModeChanged, EBattleInputMode, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotSelected, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUsed, FName, ItemRowName, int32, RemainingUses);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleItemClicked, FName, ItemRowName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattlePhaseChanged, ETurnPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPendingSlotChanged, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCaptureConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillDisplay, EInfoSide, Side, FName, SkillRowName, bool, bIsCounter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisplaySequenceDone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCreatureSummoned, EInfoSide, Side, FName, CreatureRowName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCreatureAbility, EInfoSide, Side, FName, AbilityID);

UCLASS(BlueprintType)
class AITEST_API UElfBattleController : public UObject
{
	GENERATED_BODY()

public:
	void Init(APlayerController* InOwner, EBattleType Type, AActor* Opponent);
	void HandleInput(const FGameplayTag& InputTag);

	// 连接回合管理器（道具/捕捉状态机在 TurnManager 上）
	void SetTurnManager(UElfTurnManager* InTurnManager);
	UElfTurnManager* GetTurnManager() const { return TurnManager; }

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void SetInputMode(EBattleInputMode NewMode);

	UFUNCTION(BlueprintPure, Category = "战斗")
	EBattleInputMode GetInputMode() const { return CurrentInputMode; }

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void RequestRun() { OnRunRequested.Broadcast(); }

	void BroadcastHP();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void CompleteIntro();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void SwitchToBattleCamera();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void SelectCreature(int32 Index);

	// 高亮待切换槽位（等同局内 Switch 模式按数字键的效果：设 PendingSwitchSlot + 广播高亮；由空格/确认触发切换）
	UFUNCTION(BlueprintCallable, Category = "战斗")
	void HighlightSwitchSlot(int32 SlotIndex);

	// 确认切换（等同空格：对当前高亮槽位发起切换并清空高亮）
	UFUNCTION(BlueprintCallable, Category = "战斗")
	void ConfirmSwitch();

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 CalculateSkillPower(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetDefaultSkillPower(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetDefaultEnergyCost(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetSkillEnergyCost(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	EElfType GetSkillElementType(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	EElfType GetActiveCreatureBloodline() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FName GetSkillRowName(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void UseSkill(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	bool UseItem(FName ItemRowName);

	UFUNCTION(BlueprintPure, Category = "战斗")
	bool CanUseBattleItem(FName ItemRowName) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetItemRemainingUses(FName ItemRowName) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	bool IsItemCompatibleWithCreature(FName ItemRowName) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	bool IsBattleItemUsedThisTurn() const { return TurnManager ? TurnManager->IsBattleItemUsedThisTurn() : false; }

	UFUNCTION(BlueprintPure, Category = "战斗")
	bool IsCapturePending() const { return TurnManager ? TurnManager->IsCapturePending() : false; }

	UFUNCTION(BlueprintPure, Category = "战斗")
	float GetCaptureBallRate() const { return TurnManager ? TurnManager->GetCaptureBallRate() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetCaptureItemQuantity(FName ItemRowName) const { return TurnManager ? TurnManager->GetCaptureItemQuantity(ItemRowName) : 0; }

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetPendingSwitchSlot() const { return PendingSwitchSlot; }

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetPendingCaptureSlot() const { return PendingCaptureSlot; }

	void InitCaptureItemQuantities();

	void ClearCapturePending();

	void UseBattleItem();

	UFUNCTION(BlueprintPure, Category = "战斗")
	FName GetBattleItemRowName();

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetBattleItemCount();

	UFUNCTION(BlueprintPure, Category = "战斗")
	FName GetBattleItemAtSlot(int32 FlatIndex);

	const TArray<FName>& GetBattleItemList();
	UFUNCTION(BlueprintCallable, Category = "战斗")
	void UseCaptureItem(int32 FlatIndex);

	// 高亮待使用捕捉球槽位（等同按数字键选择）
	UFUNCTION(BlueprintCallable, Category = "战斗")
	void HighlightCaptureSlot(int32 SlotIndex);

	// 确认捕捉（等同空格：使用当前高亮的捕捉球）
	UFUNCTION(BlueprintCallable, Category = "战斗")
	void ConfirmCapture();

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetCaptureItemCount();

	UFUNCTION(BlueprintPure, Category = "战斗")
	FName GetCaptureItemAtSlot(int32 FlatIndex);


	UFUNCTION(BlueprintPure, Category = "战斗")
	const TArray<FName>& GetCaptureItemList();

	void SetInputModeLocked(bool bLocked) { bInputModeLocked = bLocked; }

	void SetCurrentTurnPhase(ETurnPhase NewPhase) { CurrentTurnPhase = NewPhase; }

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void CancelWish();

	void RefundItem(FName ItemRowName);

	void ResetBattleItemState();

	void ConsumePendingItem();

	FName FindBattleItemRowName(EEffectID EffectID);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void UseDefaultSkill(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void ApplyCounterEffect(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void ConfirmReady();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void CancelReady();

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetSelfTeamCount() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetEnemyTeamCount() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetSelfAliveCount() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetEnemyAliveCount() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FElfCreatureInstance GetSelfCreature(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FElfCreatureInstance GetEnemyCreature(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetCreatureMaxHP(EInfoSide Side, int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	int32 GetCreatureCurrentHP(EInfoSide Side, int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	bool GetElfBaseData(FName RowName, FElfBaseData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	EBattleType GetBattleType() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FString GetOpponentName() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FName GetOpponentAvatarID() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FString GetSelfPlayerName() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FName GetSelfAvatarID() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	FName GetSelfCardID() const;

	UFUNCTION(BlueprintPure, Category = "战斗")
	UElfBattleModel* GetBattleModel() const { return BattleModel; }

	APlayerController* GetOwnerPC() const { return OwnerPC; }



	UPROPERTY(BlueprintAssignable)
	FOnIntroComplete OnIntroComplete;

	UPROPERTY(BlueprintAssignable)
	FOnCameraRequested OnCameraRequested;

	UPROPERTY(BlueprintAssignable)
	FOnCreatureSelected OnCreatureSelected;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerReadyStateChanged OnPlayerReadyStateChanged;

	UPROPERTY(BlueprintAssignable)
	FOnCreatureSwitched OnCreatureSwitched;

	UPROPERTY(BlueprintAssignable)
	FOnHPChanged OnSelfCreatureHPChanged;

	UPROPERTY(BlueprintAssignable)
	FOnHPChanged OnEnemyCreatureHPChanged;

	UPROPERTY(BlueprintAssignable)
	FOnEnergyChanged OnSelfCreatureEnergyChanged;

	UPROPERTY(BlueprintAssignable)
	FOnEnergyChanged OnEnemyCreatureEnergyChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDamageAnimStep OnSelfDamageAnimStep;

	UPROPERTY(BlueprintAssignable)
	FOnDamageAnimStep OnEnemyDamageAnimStep;

	UPROPERTY(BlueprintAssignable)
	FOnSkillSelected OnSkillSelected;

	UPROPERTY(BlueprintAssignable)
	FOnDefaultSkillSelected OnDefaultSkillSelected;

	UPROPERTY(BlueprintAssignable)
	FOnBattleActionPhase OnActionPhaseStarted;

	UPROPERTY(BlueprintAssignable)
	FOnBattleActionPhase OnActionPhaseEnded;

	UPROPERTY(BlueprintAssignable)
	FOnInputModeChanged OnInputModeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnSlotSelected OnItemSlotSelected;

	UPROPERTY(BlueprintAssignable)
	FOnItemUsed OnItemUsed;

	UPROPERTY(BlueprintAssignable)
	FOnBattleItemClicked OnBattleItemClicked;

	UPROPERTY(BlueprintAssignable)
	FOnCaptureConfirmed OnCaptureConfirmed;

	UPROPERTY(BlueprintAssignable)
	FOnRunRequested OnRunRequested;

	UPROPERTY(BlueprintAssignable)
	FOnSkillDisplay OnSkillDisplayStarted;

	UPROPERTY(BlueprintAssignable)
	FOnDisplaySequenceDone OnAllSkillsDisplayed;

	UPROPERTY(BlueprintAssignable)
	FOnCreatureSummoned OnCreatureSummoned;

	UPROPERTY(BlueprintAssignable)
	FOnCreatureAbility OnCreatureAbility;

	UPROPERTY(BlueprintAssignable)
	FOnBattlePhaseChanged OnBattlePhaseChanged;

	UPROPERTY(BlueprintAssignable)
	FOnSlotSelected OnSwitchSlotSelected;

	UPROPERTY(BlueprintAssignable)
	FOnSlotSelected OnCaptureSlotSelected;

	UPROPERTY(BlueprintAssignable)
	FOnSlotSelected OnCraftingSlotSelected;

	UPROPERTY(BlueprintReadOnly)
	int32 SelectedSlotIndex = 0;

	bool bInputModeLocked = false;
	ETurnPhase CurrentTurnPhase = ETurnPhase::None;

	int32 PendingSwitchSlot = -1;
	int32 PendingCaptureSlot = -1;

	UPROPERTY(BlueprintAssignable)
	FOnPendingSlotChanged OnSwitchSlotHighlighted;

	UPROPERTY(BlueprintAssignable)
	FOnPendingSlotChanged OnCaptureSlotHighlighted;

	UPROPERTY(BlueprintReadOnly)
	bool bLocalPlayerReady = false;

protected:
	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPC;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UElfBattleModel> BattleModel;

	UPROPERTY()
	TObjectPtr<UElfTurnManager> TurnManager;

	UPROPERTY()
	EBattleInputMode CurrentInputMode = EBattleInputMode::Command;
};
