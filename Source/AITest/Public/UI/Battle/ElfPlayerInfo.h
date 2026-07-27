#pragma once

#include "CoreMinimal.h"
#include "UI/ElfUserWidget.h"
#include "ElfPlayerInfo.generated.h"

class UElfBattleController;

UENUM(BlueprintType)
enum class EInfoSide : uint8
{
	Self  UMETA(DisplayName = "己方"),
	Enemy UMETA(DisplayName = "敌方")
};

UCLASS()
class AITEST_API UElfPlayerInfo : public UElfUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "信息", meta = (ExposeOnSpawn = "true"))
	EInfoSide InfoSide;

	UFUNCTION(BlueprintPure, Category = "信息")
	UElfBattleController* GetBattleController() const;

private:
	mutable TObjectPtr<UElfBattleController> CachedBattleController = nullptr;
};
