#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/ElfBattleUserWidget.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "ElfBattleInfo.generated.h"

UCLASS()
class AITEST_API UElfBattleInfo : public UElfBattleUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "信息")
	void Init(EInfoSide InSide);

	UFUNCTION(BlueprintImplementableEvent, Category = "信息")
	void OnInit(EInfoSide InSide);

	UFUNCTION(BlueprintPure, Category = "信息")
	void GetCurrentStats(int32& OutHP, int32& OutMaxHP, int32& OutEnergy) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "信息")
	void BP_OnHPChanged(int32 CurrentHP, int32 MaxHP);

	UFUNCTION(BlueprintImplementableEvent, Category = "信息")
	void BP_OnEnergyChanged(int32 NewEnergy);

	UFUNCTION(BlueprintImplementableEvent, Category = "信息")
	void BP_OnCreatureSwitched();

	UPROPERTY(BlueprintReadOnly, Category = "信息")
	EInfoSide Side;

private:
	UFUNCTION()
	void OnHPChanged(int32 NewHP, int32 MaxHP);

	UFUNCTION()
	void OnEnergyChanged(int32 NewEnergy);

	UFUNCTION()
	void OnCreatureSwitched(EInfoSide InSide);
};
