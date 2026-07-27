#include "Elf/ElfManager.h"

bool UElfManager::GetCreatureByID(const FGuid& CreatureID, FElfCreatureInstance& OutInstance)
{
	FString Key = CreatureID.ToString();
	if (FElfCreatureInstance* Found = CreatureMap.Find(Key))
	{
		OutInstance = *Found;
		return true;
	}
	return false;
}

void UElfManager::AddCreature(const FElfCreatureInstance& Instance)
{
	FString Key = Instance.CreatureID.ToString();
	CreatureMap.Add(Key, Instance);
}

void UElfManager::RemoveCreature(const FGuid& CreatureID)
{
	CreatureMap.Remove(CreatureID.ToString());
}

TArray<FElfCreatureInstance> UElfManager::GetAllCreatures() const
{
	TArray<FElfCreatureInstance> Result;
	CreatureMap.GenerateValueArray(Result);
	return Result;
}

void UElfManager::Clear()
{
	CreatureMap.Empty();
}
