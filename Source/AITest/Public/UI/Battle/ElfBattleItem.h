#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/ElfBattleUserWidget.h"
#include "ElfBattleItem.generated.h"

class UElfBattleController;

UENUM(BlueprintType)
enum class EBattleItemSlotType : uint8
{
	Battle  UMETA(DisplayName = "战斗道具"),
	Capture UMETA(DisplayName = "捕捉道具")
};

UCLASS()
class AITEST_API UElfBattleItem : public UElfBattleUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "道具")
	void Init(int32 InSlotIndex, EBattleItemSlotType InSlotType = EBattleItemSlotType::Battle);

	UFUNCTION(BlueprintImplementableEvent, Category = "道具")
	void OnInit(int32 InSlotIndex, EBattleItemSlotType InSlotType);

	UFUNCTION(BlueprintCallable, Category = "道具")
	void OnClicked();

	UFUNCTION(BlueprintPure, Category = "道具")
	int32 GetRemainingUses() const;

	UFUNCTION(BlueprintPure, Category = "道具")
	bool IsAvailable() const;

	UFUNCTION(BlueprintPure, Category = "道具")
	bool IsSelectedThisTurn() const;

	UFUNCTION(BlueprintPure, Category = "道具")
	bool GetItemData(struct FItemData& OutData) const;

	FName ResolveItemRowName() const;

	UElfBattleController* GetBattleController() const;

	UPROPERTY(BlueprintReadOnly, Category = "道具")
	int32 ItemIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "道具")
	EBattleItemSlotType SlotType;

	UPROPERTY(BlueprintReadOnly, Category = "道具")
	FName CachedItemRowName;

	mutable TObjectPtr<UElfBattleController> CachedBattleController = nullptr;
};
