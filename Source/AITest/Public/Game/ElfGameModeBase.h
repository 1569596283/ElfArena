#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ElfEnum.h"
#include "ElfGameModeBase.generated.h"

class AElfWorldBase;
class AElfPlayerController;

UCLASS()
class AITEST_API AElfGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "战斗")
	void StartBattle(APlayerController* Player, EBattleType Type, AActor* Opponent);

protected:
	void StartWildBattle(class AElfPlayerController* PC, AElfWorldBase* WildCreature);
	void StartTrainerBattle(class AElfPlayerController* PC, AActor* NPCActor);
	void StartPvPBattle(class AElfPlayerController* PC, AActor* Opponent);
};
