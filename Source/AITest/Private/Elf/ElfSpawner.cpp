#include "Elf/ElfSpawner.h"
#include "Elf/ElfSpawnTargetComponent.h"
#include "Elf/ElfWorldBase.h"
#include "Elf/ElfCharacterBase.h"
#include "Elf/ElfManager.h"
#include "Components/CapsuleComponent.h"
#include "Data/ElfBaseData.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"

AElfSpawner::AElfSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AElfSpawner::BeginPlay()
{
	Super::BeginPlay();

	GetComponents<UElfSpawnTargetComponent>(SpawnTargets);

	for (int32 i = 0; i < SpawnTargets.Num(); ++i)
	{
		TargetEntryIndex.Add(SpawnTargets[i], i % SpawnEntries.Num());
	}

	SpawnAll();
}

FVector AElfSpawner::GetGroundLocation(const FVector& Origin) const
{
	FVector Start = Origin + FVector(0, 0, TraceUp);
	FVector End = Origin - FVector(0, 0, TraceDown);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundChannel, Params))
	{
		return Hit.Location;
	}

	return Origin;
}

FVector AElfSpawner::GetRandomSpawnLocation(float Radius) const
{
	FVector2D RandomOffset = FMath::RandPointInCircle(Radius);
	FVector Origin = GetActorLocation() + FVector(RandomOffset.X, RandomOffset.Y, 0);
	return GetGroundLocation(Origin);
}

void AElfSpawner::SpawnAll()
{
	for (int32 EntryIdx = 0; EntryIdx < SpawnEntries.Num(); ++EntryIdx)
	{
		const FElfSpawnEntry& Entry = SpawnEntries[EntryIdx];
		if (!Entry.CreatureRow.DataTable) continue;

		for (int32 i = 0; i < Entry.Count; ++i)
		{
			if (i < SpawnTargets.Num())
			{
				SpawnOneCreature(SpawnTargets[i], i, EntryIdx);
			}
			else
			{
				SpawnOneCreatureRandom(EntryIdx);
			}
		}
	}
}

void AElfSpawner::InitCreatureData(AElfWorldBase* Creature, const FElfSpawnEntry& Entry, const FElfBaseData* RowData)
{
	if (!Creature || !RowData) return;

	int32 Level = FMath::RandRange(Entry.LevelMin, Entry.LevelMax);

	Creature->CreatureRowHandle = Entry.CreatureRow;
	Creature->CreatureData.CreatureRowName = Entry.CreatureRow.RowName;
	Creature->CreatureData.Level = Level;
	Creature->CreatureData.CurrentHP = RowData->BaseHP;
	Creature->CreatureData.CurrentEnergy = 10;

	Creature->CreatureData.EquippedSkills.Empty();
	for (const FSskillLearnCondition& Learn : RowData->LearnableSkills)
	{
		if (Learn.UnlockLevel <= Level && !Learn.bNeedSkillStone)
		{
			Creature->CreatureData.EquippedSkills.Add(Learn.SkillID);
			if (Creature->CreatureData.EquippedSkills.Num() >= 4) break;
		}
	}
}

void AElfSpawner::SpawnOneCreature(UElfSpawnTargetComponent* Target, int32 TargetIndex, int32 EntryIndex)
{
	if (!SpawnEntries.IsValidIndex(EntryIndex)) return;

	const FElfSpawnEntry& Entry = SpawnEntries[EntryIndex];
	FElfBaseData* RowData = Entry.CreatureRow.DataTable->FindRow<FElfBaseData>(Entry.CreatureRow.RowName, "");
	if (!RowData) return;

	TSubclassOf<AElfWorldBase> WorldBP = RowData->WorldBlueprint.LoadSynchronous();
	// 未配置 WorldBlueprint 时回退到 Default 行通用模型
	if (!WorldBP)
	{
		FElfBaseData* DefaultData = Entry.CreatureRow.DataTable->FindRow<FElfBaseData>(FName(TEXT("Default")), "");
		if (DefaultData)
			WorldBP = DefaultData->WorldBlueprint.LoadSynchronous();
	}
	if (!WorldBP) return;

	FVector2D RandomOffset = FMath::RandPointInCircle(Target->SpawnRadius);
	FVector SpawnOrigin = Target->GetComponentLocation() + FVector(RandomOffset.X, RandomOffset.Y, 0);
	FVector SpawnLocation = GetGroundLocation(SpawnOrigin);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AElfWorldBase* Creature = GetWorld()->SpawnActor<AElfWorldBase>(WorldBP, SpawnLocation, Target->GetComponentRotation(), Params);
	if (Creature)
	{
		InitCreatureData(Creature, Entry, RowData);

		UCapsuleComponent* Capsule = Creature->GetCapsuleComponent();
		if (Capsule)
		{
			FVector Loc = Creature->GetActorLocation();
			Loc.Z += Capsule->GetScaledCapsuleHalfHeight();
			Creature->SetActorLocation(Loc);
		}

		Creature->SpawnOrigin = Target->GetComponentLocation();
		Creature->WanderRadius = Target->WanderRadius;
		Creature->OwningSpawner = this;
		Creature->SpawnTargetIndex = TargetIndex;
		Creature->SpawnEntryIndex = EntryIndex;
		Creature->SpawnElfController();
	}
}

void AElfSpawner::SpawnOneCreatureRandom(int32 EntryIndex)
{
	if (!SpawnEntries.IsValidIndex(EntryIndex)) return;

	const FElfSpawnEntry& Entry = SpawnEntries[EntryIndex];
	FElfBaseData* RowData = Entry.CreatureRow.DataTable->FindRow<FElfBaseData>(Entry.CreatureRow.RowName, "");
	if (!RowData) return;

	TSubclassOf<AElfWorldBase> WorldBP = RowData->WorldBlueprint.LoadSynchronous();
	// 未配置 WorldBlueprint 时回退到 Default 行通用模型
	if (!WorldBP)
	{
		FElfBaseData* DefaultData = Entry.CreatureRow.DataTable->FindRow<FElfBaseData>(FName(TEXT("Default")), "");
		if (DefaultData)
			WorldBP = DefaultData->WorldBlueprint.LoadSynchronous();
	}
	if (!WorldBP) return;

	FVector SpawnLocation = GetRandomSpawnLocation(Entry.RandomSpawnRadius);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AElfWorldBase* Creature = GetWorld()->SpawnActor<AElfWorldBase>(WorldBP, SpawnLocation, GetActorRotation(), Params);
	if (Creature)
	{
		InitCreatureData(Creature, Entry, RowData);

		UCapsuleComponent* Capsule = Creature->GetCapsuleComponent();
		if (Capsule)
		{
			FVector Loc = Creature->GetActorLocation();
			Loc.Z += Capsule->GetScaledCapsuleHalfHeight();
			Creature->SetActorLocation(Loc);
		}

		Creature->SpawnOrigin = GetActorLocation();
		Creature->WanderRadius = Entry.RandomSpawnRadius;
		Creature->OwningSpawner = this;
		Creature->SpawnTargetIndex = -1;
		Creature->SpawnEntryIndex = EntryIndex;
		Creature->SpawnElfController();
	}
}

void AElfSpawner::OnCreatureDefeated(AElfWorldBase* Creature)
{
	if (!Creature || Creature->SpawnEntryIndex < 0 || !SpawnEntries.IsValidIndex(Creature->SpawnEntryIndex)) return;

	if (SpawnTargets.IsValidIndex(Creature->SpawnTargetIndex))
	{
		StartRespawnTimer(Creature->SpawnEntryIndex, SpawnTargets[Creature->SpawnTargetIndex]);
	}
	else
	{
		StartRespawnTimer(Creature->SpawnEntryIndex);
	}
}

void AElfSpawner::StartRespawnTimer(int32 EntryIndex, UElfSpawnTargetComponent* Target)
{
	if (!SpawnEntries.IsValidIndex(EntryIndex)) return;

	float RespawnTime = SpawnEntries[EntryIndex].RespawnTime;
	if (RespawnTime <= 0.f)
	{
		if (Target)
		{
			SpawnOneCreature(Target, SpawnTargets.IndexOfByKey(Target), EntryIndex);
		}
		else
		{
			SpawnOneCreatureRandom(EntryIndex);
		}
		return;
	}

	if (Target)
	{
		FTimerHandle& Handle = RespawnTimers.FindOrAdd(Target);
		GetWorld()->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &AElfSpawner::OnTargetRespawnTimer, Target),
			RespawnTime, false);
	}
	else
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &AElfSpawner::OnRandomRespawnTimer, EntryIndex),
			RespawnTime, false);
	}
}

void AElfSpawner::OnTargetRespawnTimer(UElfSpawnTargetComponent* Target)
{
	int32 TargetIdx = SpawnTargets.IndexOfByKey(Target);
	int32 EntryIdx = TargetEntryIndex.FindRef(Target);
	if (SpawnEntries.IsValidIndex(EntryIdx))
	{
		SpawnOneCreature(Target, TargetIdx, EntryIdx);
	}
	RespawnTimers.Remove(Target);
}

void AElfSpawner::OnRandomRespawnTimer(int32 EntryIndex)
{
	SpawnOneCreatureRandom(EntryIndex);
}

void AElfSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
