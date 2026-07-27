#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "ElfAssetManager.generated.h"

UCLASS()
class AITEST_API UElfAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UElfAssetManager& Get();
	virtual void StartInitialLoading() override;
};
