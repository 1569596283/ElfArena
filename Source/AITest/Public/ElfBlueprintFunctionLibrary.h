#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ElfBlueprintFunctionLibrary.generated.h"

class UElfGameInstance;

UCLASS()
class AITEST_API UElfBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Elf", meta = (WorldContext = "WorldContext"))
	static UElfGameInstance* GetElfGameInstance(UObject* WorldContext);
};
