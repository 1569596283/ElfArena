#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ElfAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;

UCLASS()
class AITEST_API AElfAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;

	UBlackboardComponent* GetElfBlackboard() const { return BlackboardComp; }

protected:
	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;
};
