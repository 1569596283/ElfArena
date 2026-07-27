#include "Player/ElfPlayerState.h"
#include "Net/UnrealNetwork.h"

void AElfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AElfPlayerState, TeamCreatures);
	DOREPLIFETIME(AElfPlayerState, WarehouseCreatures);
	DOREPLIFETIME(AElfPlayerState, AvatarID);
}

void AElfPlayerState::OnRep_TeamCreatures()
{
}

void AElfPlayerState::OnRep_WarehouseCreatures()
{
}

void AElfPlayerState::Server_AddToTeam_Implementation(const FGuid& CreatureID)
{
	if (TeamCreatures.Num() >= 6) return;

	for (int32 i = 0; i < WarehouseCreatures.Num(); ++i)
	{
		if (WarehouseCreatures[i].CreatureID == CreatureID)
		{
			TeamCreatures.Add(WarehouseCreatures[i]);
			WarehouseCreatures.RemoveAt(i);
			return;
		}
	}
}

void AElfPlayerState::Server_RemoveFromTeam_Implementation(const FGuid& CreatureID)
{
	for (int32 i = 0; i < TeamCreatures.Num(); ++i)
	{
		if (TeamCreatures[i].CreatureID == CreatureID)
		{
			WarehouseCreatures.Add(TeamCreatures[i]);
			TeamCreatures.RemoveAt(i);
			return;
		}
	}
}

void AElfPlayerState::Server_AddToWarehouse_Implementation(const FElfCreatureInstance& Instance)
{
	WarehouseCreatures.Add(Instance);
}
