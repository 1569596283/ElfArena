#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/ElfBaseData.h"
#include "ElfStatCalculator.generated.h"

UCLASS()
class AITEST_API UElfStatCalculator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "精灵数值")
	static FElfCalculatedStats CalculateStats(const FElfBaseData& BaseData);
};
