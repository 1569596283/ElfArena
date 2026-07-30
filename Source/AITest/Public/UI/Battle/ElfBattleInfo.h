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

	UPROPERTY(BlueprintReadOnly, Category = "信息")
	EInfoSide Side;
};
