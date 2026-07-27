#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ElfSpawnTargetComponent.generated.h"

UCLASS()
class AITEST_API UElfSpawnTargetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UElfSpawnTargetComponent();

	UPROPERTY(EditAnywhere, Category = "生成")
	float SpawnRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "生成")
	float WanderRadius = 300.f;
};
