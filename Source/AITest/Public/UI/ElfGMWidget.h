#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElfGMWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;

// GM 调试面板：输入索引 + 精灵行名 + 4 个技能行名，替换玩家指定索引的精灵
UCLASS()
class AITEST_API UElfGMWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 供蓝图直接调用：用指定精灵替换玩家指定索引的精灵（自动取本 UI 所属玩家的 PlayerController）
	UFUNCTION(BlueprintCallable, Category = "GM")
	bool GMReplaceElf(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex = 0);

protected:
	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCloseClicked();

	void SetStatus(const FString& Text);

	UPROPERTY()
	TObjectPtr<UEditableTextBox> ElfInput;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> IndexInput;

	UPROPERTY()
	TArray<TObjectPtr<UEditableTextBox>> SkillInputs;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};
