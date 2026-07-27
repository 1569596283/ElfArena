#pragma once

#include "CoreMinimal.h"
#include "UI/ElfUserWidget.h"
#include "ElfBattleIntro.generated.h"

class UElfBattleController;

UCLASS()
class AITEST_API UElfBattleIntro : public UElfUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "战斗")
	UElfBattleController* GetBattleController() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "战斗")
	void PlayExitAnimation();

private:
	mutable TObjectPtr<UElfBattleController> CachedBattleController = nullptr;
};
