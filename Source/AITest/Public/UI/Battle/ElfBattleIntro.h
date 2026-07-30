#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/ElfBattleUserWidget.h"
#include "ElfBattleIntro.generated.h"

UCLASS()
class AITEST_API UElfBattleIntro : public UElfBattleUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "战斗")
	void PlayExitAnimation();
};
