#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/ElfBattleUserWidget.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "ElfBattleSelect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCreatureSlotClicked, EInfoSide, Side, int32, SlotIndex);

UCLASS()
class AITEST_API UElfBattleSelect : public UElfBattleUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "选择")
	void Init(EInfoSide InSide, int32 InSlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "选择")
	void OnInit(EInfoSide InSide, int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "选择")
	void OnPress();

	UFUNCTION(BlueprintCallable, Category = "选择")
	void OnRelease();

	UPROPERTY(EditDefaultsOnly, Category = "选择")
	float LongPressDuration = 0.5f;

	UPROPERTY(BlueprintAssignable, Category = "选择")
	FOnCreatureSlotClicked OnCreatureSelected;

	UPROPERTY(BlueprintAssignable, Category = "选择")
	FOnCreatureSlotClicked OnCreatureDetailRequested;

	UPROPERTY(BlueprintReadOnly, Category = "选择")
	EInfoSide Side;

	UPROPERTY(BlueprintReadOnly, Category = "选择")
	int32 SlotIndex;

private:
	void HandleShortPress();
	void HandleLongPress();

	UFUNCTION()
	void OnLongPressTimer();

	FTimerHandle LongPressTimerHandle;
	bool bLongPressTriggered = false;
};
