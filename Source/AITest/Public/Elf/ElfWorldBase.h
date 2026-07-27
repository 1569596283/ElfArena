#pragma once

#include "CoreMinimal.h"
#include "Elf/ElfCharacterBase.h"
#include "ElfWorldBase.generated.h"

class AElfSpawner;
class USphereComponent;

UENUM(BlueprintType)
enum class EElfWorldRole : uint8
{
	Wild UMETA(DisplayName = "野生"),
	Follow UMETA(DisplayName = "跟随")
};

UCLASS()
class AITEST_API AElfWorldBase : public AElfCharacterBase
{
	GENERATED_BODY()

public:
	AElfWorldBase();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SpawnElfController();

	UFUNCTION(BlueprintCallable, Category = "战斗")
	void OnBattleStart();

	UPROPERTY(EditAnywhere, Category = "AI")
	TSubclassOf<class AAIController> WildAIControllerClass;

	UPROPERTY(EditAnywhere, Category = "AI")
	TSubclassOf<class AAIController> FollowAIControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EElfWorldRole RoleType = EElfWorldRole::Wild;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "活动范围")
	FVector SpawnOrigin;

	UPROPERTY(EditAnywhere, Category = "活动范围")
	float WanderRadius = 300.f;

	UPROPERTY(EditAnywhere, Category = "活动范围", meta = (ClampMin = "1.0"))
	float MaxWanderDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = "活动范围")
	float WanderZOffset = 200.f;

	UPROPERTY()
	TObjectPtr<AElfSpawner> OwningSpawner;

	int32 SpawnTargetIndex = -1;
	int32 SpawnEntryIndex = -1;

protected:
	virtual void BeginPlay() override;

	void EnableBattleTrigger();

	UFUNCTION()
	void OnBattleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "战斗")
	TObjectPtr<USphereComponent> BattleTrigger;

	UPROPERTY(EditAnywhere, Category = "战斗")
	float BattleTriggerRadius = 80.f;

	bool bBattleTriggered = false;
};
