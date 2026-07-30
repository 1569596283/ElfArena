#pragma once

#include "CoreMinimal.h"
#include "UI/ElfUserWidget.h"
#include "ElfBattleUserWidget.generated.h"

class UElfBattleController;

UCLASS()
class AITEST_API UElfBattleUserWidget : public UElfUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "战斗")
	UElfBattleController* GetBattleController() const;

private:
	mutable TObjectPtr<UElfBattleController> CachedBattleController = nullptr;
};
