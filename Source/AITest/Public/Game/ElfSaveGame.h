#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Elf/ElfManager.h"
#include "ElfSaveGame.generated.h"

UCLASS()
class AITEST_API UElfSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// ===== 玩家数据 =====

	// 当前携带的精灵队伍
	UPROPERTY(SaveGame)
	TArray<FElfCreatureInstance> TeamCreatures;

	// 精灵仓库
	UPROPERTY(SaveGame)
	TArray<FElfCreatureInstance> WarehouseCreatures;

	// 玩家位置
	UPROPERTY(SaveGame)
	FVector PlayerLocation = FVector::ZeroVector;

	// 玩家旋转
	UPROPERTY(SaveGame)
	FRotator PlayerRotation = FRotator::ZeroRotator;

	// ===== 世界数据 =====

	// 大世界野生精灵实例缓存
	UPROPERTY(SaveGame)
	TMap<FString, FElfCreatureInstance> WildCreatureCache;

	// ===== 其他（预留） =====

	// 背包物品（预留）
	//UPROPERTY(SaveGame)
	//TArray<FName> InventoryItems;

	// 任务进度（预留）
	//UPROPERTY(SaveGame)
	//TMap<FName, int32> QuestProgress;
};
