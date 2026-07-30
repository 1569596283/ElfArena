#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/ElfBattleUserWidget.h"
#include "ElfEnum.h"
#include "ElfBattleHUD.generated.h"

UCLASS()
class AITEST_API UElfBattleHUD : public UElfBattleUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent)
	void SetPlayerHP(int32 CurrentHP, int32 MaxHP);

	UFUNCTION(BlueprintImplementableEvent)
	void SetEnemyHP(int32 CurrentHP, int32 MaxHP);

	UFUNCTION(BlueprintImplementableEvent)
	void EnemyPlayDamageAnim(int32 FromHP, int32 ToHP);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnInputModeChanged"))
	void BP_OnInputModeChanged(EBattleInputMode NewMode);

	UFUNCTION()
	void OnInputModeChanged(EBattleInputMode NewMode);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnBattlePhaseChanged"))
	void BP_OnBattlePhaseChanged(ETurnPhase NewPhase);

	UFUNCTION()
	void OnBattlePhaseChanged(ETurnPhase NewPhase);

};
