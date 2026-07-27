#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "ElfSpawner.generated.h"

class UElfSpawnTargetComponent;
class AElfWorldBase;

struct FElfBaseData;

USTRUCT(BlueprintType)
struct FElfSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "精灵配置")
	FDataTableRowHandle CreatureRow;

	UPROPERTY(EditAnywhere, Category = "精灵配置")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, Category = "精灵配置")
	int32 LevelMin = 1;

	UPROPERTY(EditAnywhere, Category = "精灵配置")
	int32 LevelMax = 5;

	UPROPERTY(EditAnywhere, Category = "精灵配置")
	float RandomSpawnRadius = 500.f;

	UPROPERTY(EditAnywhere, Category = "精灵配置")
	float RespawnTime = 30.f;
};

UCLASS()
class AITEST_API AElfSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AElfSpawner();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "生成")
	void SpawnAll();

	UFUNCTION(BlueprintCallable, Category = "生成")
	void OnCreatureDefeated(AElfWorldBase* Creature);

protected:
	virtual void BeginPlay() override;

	FVector GetGroundLocation(const FVector& Origin) const;
	FVector GetRandomSpawnLocation(float Radius) const;

	void SpawnOneCreature(UElfSpawnTargetComponent* Target, int32 TargetIndex, int32 EntryIndex);
	void SpawnOneCreatureRandom(int32 EntryIndex);
	void InitCreatureData(AElfWorldBase* Creature, const FElfSpawnEntry& Entry, const FElfBaseData* RowData);
	void StartRespawnTimer(int32 EntryIndex, UElfSpawnTargetComponent* Target = nullptr);
	void OnTargetRespawnTimer(UElfSpawnTargetComponent* Target);
	void OnRandomRespawnTimer(int32 EntryIndex);

	UPROPERTY(EditAnywhere, Category = "生成配置")
	TArray<FElfSpawnEntry> SpawnEntries;

	UPROPERTY(EditAnywhere, Category = "生成配置")
	float TraceUp = 500.f;

	UPROPERTY(EditAnywhere, Category = "生成配置")
	float TraceDown = 500.f;

	UPROPERTY(EditAnywhere, Category = "生成配置")
	TEnumAsByte<ECollisionChannel> GroundChannel = ECC_WorldStatic;

	TArray<TObjectPtr<UElfSpawnTargetComponent>> SpawnTargets;
	TMap<TObjectPtr<UElfSpawnTargetComponent>, int32> TargetEntryIndex;
	TMap<TObjectPtr<UElfSpawnTargetComponent>, FTimerHandle> RespawnTimers;
};
