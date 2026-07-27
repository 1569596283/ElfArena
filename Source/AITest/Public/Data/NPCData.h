#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Elf/ElfManager.h"
#include "NPCData.generated.h"

// 精灵成员表：NPC 携带的每只精灵的配置，一行一只
USTRUCT(BlueprintType)
struct FElfMemberData : public FTableRowBase
{
	GENERATED_BODY()

	// 对应 ElfDataTable 中的行名（精灵种类）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵")
	FName CreatureRowName;

	// 精灵等级
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵")
	int32 Level = 1;

	// 性别
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵")
	EElfSex Sex = EElfSex::None;

	// 性格 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵")
	FName NatureID;

	// 携带的技能 ID 列表
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能")
	TArray<FName> Skills;
};

// NPC 表：训练家配置，一行一个 NPC
USTRUCT(BlueprintType)
struct FNPCData : public FTableRowBase
{
	GENERATED_BODY()

	// NPC 名称（显示用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC信息")
	FText Name;

	// 头像 ID（对应 AvatarDataTable 行名）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC信息")
	FName AvatarID;

	// NPC 大世界模型蓝图
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC信息")
	TSoftClassPtr<AActor> Model;

	// 精灵成员行名数组，指向 FElfMemberData
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "队伍")
	TArray<FName> TeamMembers;
};
