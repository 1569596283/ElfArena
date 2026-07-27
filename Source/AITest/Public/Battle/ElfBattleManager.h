#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "ElfEnum.h"
#include "Elf/ElfManager.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "ElfBattleManager.generated.h"

class UUIManager;
class UElfBattleController;
class UElfTurnManager;
class UUserWidget;

UCLASS(Blueprintable)
class AITEST_API UElfBattleManager : public UObject
{
	GENERATED_BODY()

public:
	UElfBattleManager();

	void Init(UUIManager* InUIManager, APlayerController* InOwner);

	void HandleInput(const FGameplayTag& InputTag);

	UFUNCTION(BlueprintCallable)
	void StartBattle(APlayerController* Initiator, EBattleType Type, AActor* Opponent);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void SpawnBattleCreatures();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void RecallCreature(EInfoSide Side);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	AActor* ReleaseCreature(EInfoSide Side, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	AActor* SpawnCreature(const FElfCreatureInstance& CreatureData, AActor* SpawnPoint);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void PlaySpawnAnimation(AActor* CreatureActor, float Duration = 0.4f);

	UElfBattleController* GetBattleController() const { return BattleController; }

	UFUNCTION(BlueprintPure)
	UElfTurnManager* GetTurnManager() const { return TurnManager; }

	UFUNCTION(BlueprintPure)
	APlayerController* GetOwnerPC() const { return OwnerPC; }

	UFUNCTION(BlueprintCallable)
	void OnIntroComplete();

	UFUNCTION()
	void OnPlayerReadyStateChanged(bool bIsReady);

	UFUNCTION()
	void OnTurnSwitchRequested(EInfoSide Side, int32 NextSlotIndex);

	UFUNCTION()
	void OnTurnBattleEnded(EBattleResult Result);

	UFUNCTION(BlueprintCallable)
	void SkipToBattle();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> IntroWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> BattleWidgetClass;

protected:
	void ShowIntro();
	void EnterBattle();
	void CloseCurrentUI();

	UPROPERTY()
	TObjectPtr<UUIManager> UIManager;

	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPC;

	UPROPERTY()
	TObjectPtr<UElfBattleController> BattleController;

	UPROPERTY()
	TObjectPtr<UElfTurnManager> TurnManager;

	UPROPERTY()
	TObjectPtr<UUserWidget> IntroWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> BattleWidget;

	EBattlePhase CurrentPhase = EBattlePhase::None;
	EBattleType BattleType;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> BattleCreatures;

private:
	struct FScaleAnim
	{
		TWeakObjectPtr<AActor> Actor;
		float Elapsed = 0.0f;
		float Duration = 0.4f;
		FVector StartScale = FVector::ZeroVector;
		FVector TargetScale = FVector::OneVector;
	};

	void TickScaleAnimations();

	TArray<FScaleAnim> ActiveAnimations;

	FTimerHandle AnimTimerHandle;
};
