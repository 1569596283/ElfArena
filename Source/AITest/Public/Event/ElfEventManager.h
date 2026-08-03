#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Elf/ElfManager.h"
#include "ElfEventManager.generated.h"

// 战斗事件委托：事件 Tag + 相关精灵指针（可为空）
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayEvent, const FGameplayTag&, const FElfCreatureInstance*);

UCLASS()
class AITEST_API UElfEventManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 广播事件（触发点调用）。Creature 为相关精灵指针，无则传 nullptr
	void BroadcastEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature = nullptr);

	// 订阅事件（特性管理器等调用）
	FOnGameplayEvent OnGameplayEvent;
};
