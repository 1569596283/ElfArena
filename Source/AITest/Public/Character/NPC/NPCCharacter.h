#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCCharacter.generated.h"

class USphereComponent;

UCLASS()
class AITEST_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANPCCharacter();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NPCDataID;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	void EnableBattleTrigger();

	UFUNCTION()
	void OnBattleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "战斗")
	TObjectPtr<USphereComponent> BattleTrigger;

	bool bBattleTriggered = false;
};
