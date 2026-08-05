#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Data/ElfSkillData.h"
#include "ElfItemData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	General   UMETA(DisplayName = "通用"),
	Battle    UMETA(DisplayName = "战斗道具"),
	Capture   UMETA(DisplayName = "捕捉"),
	Material  UMETA(DisplayName = "材料"),
	SkillBook UMETA(DisplayName = "技能书"),
	Evolve    UMETA(DisplayName = "进化道具")
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "道具", meta = (DisplayName = "显示名称"))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "道具")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "道具")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "道具")
	EItemType ItemType = EItemType::General;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "道具", meta = (DisplayName = "战斗限用次数", ToolTip = "仅战斗道具有效，0=无限"))
	int32 MaxBattleUses = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果")
	EEffectID EffectID = EEffectID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "目标行名", ToolTip = "愿力: 技能ID"))
	FName TargetRowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果")
	TArray<float> Params;
};
