#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ElfGameInstance.generated.h"

UCLASS()
class AITEST_API UElfGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UPROPERTY(EditAnywhere, Category = "精灵数据")
	TObjectPtr<UDataTable> ElfDataTable;

	UPROPERTY(EditAnywhere, Category = "精灵数据")
	TObjectPtr<UDataTable> ElfMemberDataTable;

	UPROPERTY(EditAnywhere, Category = "技能数据")
	TObjectPtr<UDataTable> SkillDataTable;

	UPROPERTY(EditAnywhere, Category = "NPC数据")
	TObjectPtr<UDataTable> NPCDataTable;

	UPROPERTY(EditAnywhere, Category = "图像数据")
	TObjectPtr<UDataTable> AvatarDataTable;

	UPROPERTY(EditAnywhere, Category = "图像数据")
	TObjectPtr<UDataTable> CardDataTable;

	UPROPERTY(EditAnywhere, Category = "图像数据")
	TObjectPtr<UDataTable> SexDataTable;

	UPROPERTY(EditAnywhere, Category = "图像数据")
	TObjectPtr<UDataTable> TypeDataTable;

	UPROPERTY(EditAnywhere, Category = "初始队伍")
	TArray<FName> StartingTeam;

	UPROPERTY(EditAnywhere, Category = "默认技能", meta = (DisplayName = "默认技能列表", ToolTip = "所有精灵无需装备即可使用的技能"))
	TArray<FName> DefaultSkillIDs;

	UPROPERTY(EditAnywhere, Category = "属性克制", meta = (DisplayName = "属性克制表"))
	TObjectPtr<UDataTable> TypeChartTable;

	UPROPERTY(EditAnywhere, Category = "增益减益", meta = (DisplayName = "Buff定义表"))
	TObjectPtr<UDataTable> BuffDataTable;

	UPROPERTY(EditAnywhere, Category = "特性", meta = (DisplayName = "特性表"))
	TObjectPtr<UDataTable> AbilityDataTable;

	UPROPERTY(EditAnywhere, Category = "道具", meta = (DisplayName = "道具表"))
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY()
	TMap<FName, int32> CaptureItemQuantities;

	void ReplenishCaptureItems();

	// 快捷查询函数
	UFUNCTION(BlueprintCallable, Category = "精灵数据")
	bool GetElfBaseData(FName RowName, struct FElfBaseData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "精灵数据")
	bool GetElfMemberData(FName RowName, struct FElfMemberData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "技能数据")
	bool GetSkillData(FName RowName, struct FSkillData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "NPC数据")
	bool GetNPCData(FName RowName, struct FNPCData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "图像数据")
	bool GetAvatarData(FName RowName, struct FImageData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "图像数据")
	bool GetCardData(FName RowName, struct FImageData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "图像数据")
	bool GetElfGender(uint8 Sex, struct FImageData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "图像数据")
	bool GetElfType(uint8 Type, struct FImageData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "增益减益")
	bool GetBuffData(FName RowName, struct FEffectData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "特性")
	bool GetAbilityData(FName RowName, struct FAbilityData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "道具")
	bool GetItemData(FName RowName, struct FItemData& OutData) const;
};
