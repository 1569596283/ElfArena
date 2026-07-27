#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "ElfEnum.h"
#include "ElfPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UElfInputConfig;
class UElfSaveGame;
class UElfManager;
class UUIManager;
class UElfBattleManager;

struct FInputActionValue;

UCLASS()
class AITEST_API AElfPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "存档")
	void SaveGame(const FString& SlotName = "Default");

	UFUNCTION(BlueprintCallable, Category = "存档")
	void LoadGame(const FString& SlotName = "Default");

	UFUNCTION(Client, Reliable)
	void Client_EnterBattleMode(AActor* Opponent, EBattleType Type);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void EnterBattleMode(AActor* Opponent, EBattleType Type = EBattleType::Wild);

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void ExitBattleMode();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void MoveCameraToBattle();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void MoveCameraBackToPlayer();

	UFUNCTION(BlueprintPure, Category = "战斗")
	bool IsInBattle() const { return bIsInBattle; }

	UFUNCTION(BlueprintCallable, Category = "精灵")
	void InitDefaultTeam();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗")
	TSubclassOf<UElfBattleManager> BattleManagerClass;

private:

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> WorldInputContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> BattleInputContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UElfInputConfig> InputConfig;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float LookYawSensitivity = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float LookPitchSensitivity = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bInvertPitch = true;

	UPROPERTY()
	TObjectPtr<UUIManager> UIManager;

	UPROPERTY()
	TObjectPtr<UElfBattleManager> BattleManager;

	UPROPERTY()
	TObjectPtr<AActor> SavedViewTarget;

	UPROPERTY(Replicated)
	bool bIsInBattle = false;
	bool bCameraAtBattle = false;

	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void JumpStarted();
	void JumpCompleted();

	void InputTagPressed(FGameplayTag InputTag);
	void InputTagReleased(FGameplayTag InputTag);
	void InputTagHeld(FGameplayTag InputTag);
};
