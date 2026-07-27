#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCSpawner.generated.h"

class USceneComponent;

UCLASS()
class AITEST_API ANPCSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANPCSpawner();

	UFUNCTION(BlueprintCallable)
	void SpawnAll();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "生成配置")
	TSubclassOf<class ANPCCharacter> NPCClass;

	UPROPERTY(EditAnywhere, Category = "生成配置")
	FName NPCDataID;

	TArray<TObjectPtr<USceneComponent>> SpawnPoints;
};
