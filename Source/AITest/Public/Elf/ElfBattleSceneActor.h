#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ElfBattleSceneActor.generated.h"

UCLASS(Blueprintable)
class AITEST_API AElfBattleSceneActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗场景")
	AActor* PlayerSpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗场景")
	AActor* EnemySpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗场景")
	AActor* BattleCameraActor;
};
