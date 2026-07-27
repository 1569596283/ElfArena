#pragma once

#include "CoreMinimal.h"
#include "ElfEnum.generated.h"

UENUM(BlueprintType)
enum class EBattleType : uint8
{
	Wild    UMETA(DisplayName = "野生精灵"),
	Trainer UMETA(DisplayName = "训练家"),
	PvP     UMETA(DisplayName = "玩家对战")
};

UENUM(BlueprintType)
enum class EBattlePhase : uint8
{
	None    UMETA(DisplayName = "无"),
	Intro   UMETA(DisplayName = "开场动画"),
	Switch  UMETA(DisplayName = "切换精灵"),
	Battle  UMETA(DisplayName = "对战中")
};

UENUM(BlueprintType)
enum class ETurnPhase : uint8
{
	None              UMETA(DisplayName = "无"),
	PlayerCommand     UMETA(DisplayName = "选择技能"),
	WaitingForOpponent UMETA(DisplayName = "等待对手"),
	AIDecision        UMETA(DisplayName = "AI 决策"),
	Resolving         UMETA(DisplayName = "行动排序"),
	Executing         UMETA(DisplayName = "执行技能"),
	TurnEnd           UMETA(DisplayName = "回合结束"),
	Switch            UMETA(DisplayName = "切换精灵"),
	BattleEnd         UMETA(DisplayName = "战斗结束")
};

UENUM(BlueprintType)
enum class EBattleResult : uint8
{
	None       UMETA(DisplayName = "无"),
	PlayerWin  UMETA(DisplayName = "玩家胜利"),
	PlayerLose UMETA(DisplayName = "玩家失败"),
	Draw       UMETA(DisplayName = "平局"),
	Run        UMETA(DisplayName = "逃跑")
};

UENUM(BlueprintType)
enum class EBattleInputMode : uint8
{
	Command  UMETA(DisplayName = "技能选择"),
	Item     UMETA(DisplayName = "道具"),
	Capture  UMETA(DisplayName = "捕捉精灵"),
	Crafting UMETA(DisplayName = "精灵球制作")
};
