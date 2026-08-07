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
class UUserWidget;

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

	// ==== GM 调试 ====
	// 打开 GM 面板（进入战斗后 bGMDismissed=true，不再弹出）
	UFUNCTION(Exec, BlueprintCallable, Category = "GM")
	void OpenGM();

	// GM 面板关闭时回调（由 GM Widget 调用）
	void OnGMWidgetClosed();

	// 关闭 GM 面板并恢复输入模式
	void CloseGMWidget();

	// 用指定精灵替换玩家指定索引（SlotIndex）的精灵：等级继承、个体随机；技能无效/空缺时从该精灵可学技能随机补
	// 索引越界时按最后一只处理；队伍为空则新增；客户端调用会走 Server RPC，保证在服务器上执行并复制回各客户端
	UFUNCTION(BlueprintCallable, Category = "GM")
	bool GMReplaceElf(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex = 0);

	UFUNCTION(Server, Reliable)
	void Server_GMReplaceElf(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex);

	bool GMReplaceElf_Authority(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex);

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
	TObjectPtr<UUserWidget> GMWidget;

	// GM 面板类（可在 BP_PlayerController 配置为继承 UElfGMWidget 的 GMHUD）
	UPROPERTY(EditDefaultsOnly, Category = "GM")
	TSubclassOf<UUserWidget> GMWidgetClass;

	// 进入战斗后置 true，本次运行不再弹出 GM
	bool bGMDismissed = false;

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
