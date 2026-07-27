#include "Character/NPC/NPCCharacter.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/PlayerCharacter.h"
#include "Player/ElfPlayerController.h"
#include "Game/ElfGameModeBase.h"
#include "GameFramework/GameModeBase.h"

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BattleTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("BattleTrigger"));
	BattleTrigger->SetupAttachment(RootComponent);
	BattleTrigger->SetSphereRadius(80.f);
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BattleTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	BattleTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	BattleTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ANPCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPCCharacter, NPCDataID);
}

void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ANPCCharacter::EnableBattleTrigger);
}

void ANPCCharacter::EnableBattleTrigger()
{
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BattleTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ANPCCharacter::OnBattleTriggerOverlap);
}

void ANPCCharacter::OnBattleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bBattleTriggered) return;

	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player) return;

	AElfPlayerController* PC = Player->GetController<AElfPlayerController>();
	if (!PC || PC->IsInBattle()) return;

	bBattleTriggered = true;

	AElfGameModeBase* ElfGM = Cast<AElfGameModeBase>(GetWorld()->GetAuthGameMode());
	if (ElfGM)
	{
		ElfGM->StartBattle(PC, EBattleType::Trainer, this);
	}
}
