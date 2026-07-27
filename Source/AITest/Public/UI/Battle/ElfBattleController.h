#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "ElfEnum.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "ElfBattleController.generated.h"

class UElfBattleModel;
class UUserWidget;

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

UCLASS(BlueprintType)
class AITEST_API UElfBattleController : public UObject
{
	GENERATED_BODY()

public:
	void Init(APlayerController* InOwner, EBattleType Type, AActor* Opponent);
	void HandleInput(const FGameplayTag& InputTag);
	void BroadcastHP();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void CompleteIntro();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void SwitchToBattleCamera();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void SelectCreature(int32 Index);

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
	FName GetSkillRowName(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void UseSkill(int32 SlotIndex);

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

	UPROPERTY(BlueprintReadOnly)
	int32 SelectedSlotIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bLocalPlayerReady = false;

protected:
	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPC;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UElfBattleModel> BattleModel;
};
