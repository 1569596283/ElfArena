#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Elf/ElfManager.h"
#include "Data/ElfBaseData.h"
#include "ElfEnum.h"
#include "ElfBattleModel.generated.h"

class UElfSkillBase;
class UElfAbilityBase;

USTRUCT(BlueprintType)
struct FSkillInstanceList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UElfSkillBase*> Instances;
};

USTRUCT(BlueprintType)
struct FBattleSideData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FElfCreatureInstance> Team;

	UPROPERTY()
	TArray<FElfCalculatedStats> CalculatedStats;

	UPROPERTY()
	TArray<FSkillInstanceList> SkillInstances;

	UPROPERTY()
	TArray<UElfAbilityBase*> AbilityInstances;

	UPROPERTY()
	TArray<FSkillInstanceList> DefaultSkillInstances;

	UPROPERTY()
	TArray<FActiveBuff> SideBuffs;

	UPROPERTY()
	int32 ActiveIndex = 0;

	FElfCreatureInstance* GetActiveCreature()
	{
		return Team.IsValidIndex(ActiveIndex) ? &Team[ActiveIndex] : nullptr;
	}

	FElfCalculatedStats* GetActiveStats()
	{
		return CalculatedStats.IsValidIndex(ActiveIndex) ? &CalculatedStats[ActiveIndex] : nullptr;
	}

	UElfSkillBase* GetSkillInstance(int32 CreatureIndex, int32 SlotIndex)
	{
		if (SkillInstances.IsValidIndex(CreatureIndex) && SkillInstances[CreatureIndex].Instances.IsValidIndex(SlotIndex))
		{
			return SkillInstances[CreatureIndex].Instances[SlotIndex];
		}
		return nullptr;
	}

	UElfSkillBase* GetActiveSkillInstance(int32 SlotIndex) const
	{
		if (SkillInstances.IsValidIndex(ActiveIndex) && SkillInstances[ActiveIndex].Instances.IsValidIndex(SlotIndex))
		{
			return SkillInstances[ActiveIndex].Instances[SlotIndex];
		}
		return nullptr;
	}

	UElfSkillBase* GetActiveDefaultSkillInstance(int32 SlotIndex) const
	{
		if (DefaultSkillInstances.IsValidIndex(ActiveIndex) && DefaultSkillInstances[ActiveIndex].Instances.IsValidIndex(SlotIndex))
		{
			return DefaultSkillInstances[ActiveIndex].Instances[SlotIndex];
		}
		return nullptr;
	}

	int32 GetDefaultSkillCount() const
	{
		return DefaultSkillInstances.IsValidIndex(ActiveIndex) ? DefaultSkillInstances[ActiveIndex].Instances.Num() : 0;
	}

	void MoveActiveToEnd()
	{
		if (!Team.IsValidIndex(ActiveIndex)) return;

		FElfCreatureInstance Creature = Team[ActiveIndex];
		Team.RemoveAt(ActiveIndex);

		if (CalculatedStats.IsValidIndex(ActiveIndex))
		{
			FElfCalculatedStats Stats = CalculatedStats[ActiveIndex];
			CalculatedStats.RemoveAt(ActiveIndex);
			CalculatedStats.Add(Stats);
		}

		if (SkillInstances.IsValidIndex(ActiveIndex))
		{
			FSkillInstanceList Skills = SkillInstances[ActiveIndex];
			SkillInstances.RemoveAt(ActiveIndex);
			SkillInstances.Add(Skills);
		}

		if (DefaultSkillInstances.IsValidIndex(ActiveIndex))
		{
			FSkillInstanceList Skills = DefaultSkillInstances[ActiveIndex];
			DefaultSkillInstances.RemoveAt(ActiveIndex);
			DefaultSkillInstances.Add(Skills);
		}

		if (AbilityInstances.IsValidIndex(ActiveIndex))
		{
			UElfAbilityBase* Ability = AbilityInstances[ActiveIndex];
			AbilityInstances.RemoveAt(ActiveIndex);
			AbilityInstances.Add(Ability);
		}

		Team.Add(Creature);
		ActiveIndex = Team.Num() - 1;
	}

	void MoveToFront(int32 Index)
	{
		if (!Team.IsValidIndex(Index)) return;

		FElfCreatureInstance Creature = Team[Index];
		Team.RemoveAt(Index);
		Team.Insert(Creature, 0);

		if (CalculatedStats.IsValidIndex(Index))
		{
			FElfCalculatedStats Stats = CalculatedStats[Index];
			CalculatedStats.RemoveAt(Index);
			CalculatedStats.Insert(Stats, 0);
		}

		if (SkillInstances.IsValidIndex(Index))
		{
			FSkillInstanceList Skills = SkillInstances[Index];
			SkillInstances.RemoveAt(Index);
			SkillInstances.Insert(Skills, 0);
		}

		if (DefaultSkillInstances.IsValidIndex(Index))
		{
			FSkillInstanceList Skills = DefaultSkillInstances[Index];
			DefaultSkillInstances.RemoveAt(Index);
			DefaultSkillInstances.Insert(Skills, 0);
		}

		if (AbilityInstances.IsValidIndex(Index))
		{
			UElfAbilityBase* Ability = AbilityInstances[Index];
			AbilityInstances.RemoveAt(Index);
			AbilityInstances.Insert(Ability, 0);
		}

		ActiveIndex = 0;
	}
};

UCLASS()
class AITEST_API UElfBattleModel : public UObject
{
	GENERATED_BODY()

public:
	void Init(APlayerController* OwnerPC, EBattleType Type, AActor* Opponent);

	UPROPERTY()
	FBattleSideData PlayerSide;

	UPROPERTY()
	FBattleSideData EnemySide;

	UPROPERTY()
	FString OpponentName;

	UPROPERTY()
	FName OpponentAvatarID;

	UPROPERTY()
	EBattleType BattleType;
};
