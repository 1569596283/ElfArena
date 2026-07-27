#include "Elf/BTTask/BTT_Wild_SetTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

EBTNodeResult::Type UBTT_Wild_SetTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!BB || !Controller) return EBTNodeResult::Failed;

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	FVector SpawnOrigin = BB->GetValueAsVector("SpawnOrigin");
	float WanderRadius = BB->GetValueAsFloat("WanderRadius");
	float MaxDistance = BB->GetValueAsFloat("MaxWanderDistance");
	float ZOffsetRange = BB->GetValueAsFloat("WanderZOffset");

	FVector Origin = Pawn->GetActorLocation();
	FVector2D RandomDir = FMath::RandPointInCircle(1.0f);
	RandomDir.Normalize();
	float RandomDist = FMath::FRandRange(100.f, MaxDistance);

	FVector TargetLocation = Origin + FVector(RandomDir.X * RandomDist, RandomDir.Y * RandomDist, 0);

	TargetLocation.Z += FMath::FRandRange(-ZOffsetRange, ZOffsetRange);

	FVector ToSpawn = TargetLocation - SpawnOrigin;
	if (ToSpawn.Size2D() > WanderRadius)
	{
		ToSpawn.Z = 0;
		ToSpawn.Normalize();
		TargetLocation = SpawnOrigin + ToSpawn * WanderRadius;
		TargetLocation.Z = SpawnOrigin.Z + FMath::FRandRange(-ZOffsetRange, ZOffsetRange);
	}

	BB->SetValueAsVector(TargetLocationKey.SelectedKeyName, TargetLocation);

	return EBTNodeResult::Succeeded;
}

