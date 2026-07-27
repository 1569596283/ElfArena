#include "Elf/ElfAIController.h"
#include "Elf/ElfWorldBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

void AElfAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AElfWorldBase* Creature = Cast<AElfWorldBase>(InPawn);
	if (!Creature || !BehaviorTree) return;

	UseBlackboard(BehaviorTree->BlackboardAsset, BlackboardComp);
	if (!BlackboardComp) return;

	BlackboardComp->SetValueAsVector("SpawnOrigin", Creature->SpawnOrigin);
	BlackboardComp->SetValueAsFloat("WanderRadius", Creature->WanderRadius);
	BlackboardComp->SetValueAsFloat("MaxWanderDistance", Creature->MaxWanderDistance);
	BlackboardComp->SetValueAsFloat("WanderZOffset", Creature->WanderZOffset);

	RunBehaviorTree(BehaviorTree);
}

