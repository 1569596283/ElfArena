#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/ElfBattleUserWidget.h"
#include "ElfPlayerInfo.generated.h"

UENUM(BlueprintType)
enum class EInfoSide : uint8
{
	Self  UMETA(DisplayName = "己方"),
	Enemy UMETA(DisplayName = "敌方")
};

UCLASS()
class AITEST_API UElfPlayerInfo : public UElfBattleUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "信息", meta = (ExposeOnSpawn = "true"))
	EInfoSide InfoSide;
};
