#include "Elf/ElfWorldBase.h"
#include "Components/SphereComponent.h"
#include "AIController.h"
#include "Character/PlayerCharacter.h"
#include "Player/ElfPlayerController.h"
#include "Game/ElfGameModeBase.h"
#include "GameFramework/GameModeBase.h"

AElfWorldBase::AElfWorldBase()
{
	AutoPossessAI = EAutoPossessAI::Disabled;
	bReplicates = true;

	BattleTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("BattleTrigger"));
	BattleTrigger->SetupAttachment(RootComponent);
	BattleTrigger->SetSphereRadius(BattleTriggerRadius);
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BattleTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	BattleTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	BattleTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AElfWorldBase::BeginPlay()
{
	Super::BeginPlay();

	BattleTrigger->SetSphereRadius(BattleTriggerRadius);
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AElfWorldBase::EnableBattleTrigger);
}

void AElfWorldBase::EnableBattleTrigger()
{
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BattleTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &AElfWorldBase::OnBattleTriggerOverlap);
}

void AElfWorldBase::OnBattleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bBattleTriggered || RoleType != EElfWorldRole::Wild) return;

	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player) return;

	AElfPlayerController* PC = Player->GetController<AElfPlayerController>();
	if (!PC || PC->IsInBattle()) return;

	bBattleTriggered = true;

	AElfGameModeBase* ElfGM = Cast<AElfGameModeBase>(GetWorld()->GetAuthGameMode());
	if (ElfGM)
	{
		ElfGM->StartBattle(PC, EBattleType::Wild, this);
	}
}

void AElfWorldBase::SpawnElfController()
{
	TSubclassOf<AAIController> ControllerClass = (RoleType == EElfWorldRole::Follow) ? FollowAIControllerClass : WildAIControllerClass;
	if (ControllerClass && !GetController())
	{
		AIControllerClass = ControllerClass;
		SpawnDefaultController();
	}
}

void AElfWorldBase::OnBattleStart()
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (AIC)
	{
		AIC->StopMovement();
		AIC->UnPossess();
	}
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);
}

