#pragma once

#include "CoreMinimal.h"
#include "UI/ElfUserWidget.h"
#include "ElfBattleSkill.generated.h"

class UElfBattleController;

UCLASS()
class AITEST_API UElfBattleSkill : public UElfUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "技能")
	void Init(int32 InSlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "技能")
	void OnInit(int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "技能")
	void OnClicked();

	UFUNCTION(BlueprintPure, Category = "技能")
	UElfBattleController* GetBattleController() const;

	UFUNCTION(BlueprintPure, Category = "技能")
	int32 GetSkillEnergyCost() const;

	UFUNCTION(BlueprintPure, Category = "技能")
	int32 GetCurrentPower() const;

	UFUNCTION(BlueprintPure, Category = "技能")
	EPowerState GetPowerState() const;

	UFUNCTION(BlueprintPure, Category = "技能")
	int32 GetCurrentEnergyCost() const;

	UFUNCTION(BlueprintPure, Category = "技能")
	EEnergyState GetEnergyState() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "技能")
	void ReceiveEnergyChanged(int32 NewEnergy);

	UPROPERTY(BlueprintReadOnly, Category = "技能")
	int32 SlotIndex;

private:
	UFUNCTION()
	void OnEnergyChanged(int32 NewEnergy);

	mutable TObjectPtr<UElfBattleController> CachedBattleController = nullptr;
};
