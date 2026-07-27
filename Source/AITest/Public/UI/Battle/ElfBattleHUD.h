#pragma once

#include "CoreMinimal.h"
#include "UI/ElfUserWidget.h"
#include "ElfBattleHUD.generated.h"

class UElfBattleController;

UCLASS()
class AITEST_API UElfBattleHUD : public UElfUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "战斗")
	UElfBattleController* GetBattleController() const;

	UFUNCTION(BlueprintImplementableEvent)
	void SetPlayerHP(int32 CurrentHP, int32 MaxHP);

	UFUNCTION(BlueprintImplementableEvent)
	void SetEnemyHP(int32 CurrentHP, int32 MaxHP);

	UFUNCTION(BlueprintImplementableEvent)
	void EnemyPlayDamageAnim(int32 FromHP, int32 ToHP);

private:
	mutable TObjectPtr<UElfBattleController> CachedBattleController = nullptr;
};
