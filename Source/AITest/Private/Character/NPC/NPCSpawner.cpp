#include "Character/NPC/NPCSpawner.h"
#include "Character/NPC/NPCCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ANPCSpawner::ANPCSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANPCSpawner::BeginPlay()
{
	Super::BeginPlay();

	GetComponents<USceneComponent>(SpawnPoints);
	SpawnPoints.RemoveAll([this](USceneComponent* Comp) { return Comp == GetRootComponent(); });

	SpawnAll();
}

void ANPCSpawner::SpawnAll()
{
	if (!NPCClass || NPCDataID.IsNone()) return;

	if (SpawnPoints.IsEmpty())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ANPCCharacter* NPC = GetWorld()->SpawnActor<ANPCCharacter>(NPCClass, GetActorLocation(), GetActorRotation(), Params);
		if (NPC)
		{
			NPC->NPCDataID = NPCDataID;
		}
		return;
	}

	for (USceneComponent* Point : SpawnPoints)
	{
		if (!Point) continue;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ANPCCharacter* NPC = GetWorld()->SpawnActor<ANPCCharacter>(NPCClass, Point->GetComponentLocation(), Point->GetComponentRotation(), Params);
		if (NPC)
		{
			NPC->NPCDataID = NPCDataID;
		}
	}
}
