#include "Character/PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/ElfPlayerController.h"
#include "Game/ElfGameModeBase.h"
#include "GameFramework/GameModeBase.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	BattleTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("BattleTrigger"));
	BattleTrigger->SetupAttachment(RootComponent);
	BattleTrigger->SetSphereRadius(80.f);
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BattleTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	BattleTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	BattleTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &APlayerCharacter::EnableBattleTrigger);
}

void APlayerCharacter::EnableBattleTrigger()
{
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BattleTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &APlayerCharacter::OnBattleTriggerOverlap);
}

void APlayerCharacter::Jump()
{
	Super::Jump();

	if (JumpMontage && GetCharacterMovement()->IsMovingOnGround())
	{
		PlayAnimMontage(JumpMontage);
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::OnBattleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bBattleTriggered) return;

	APlayerCharacter* OtherPlayer = Cast<APlayerCharacter>(OtherActor);
	if (!OtherPlayer) return;

	AElfPlayerController* PC = GetController<AElfPlayerController>();
	AElfPlayerController* OtherPC = OtherPlayer->GetController<AElfPlayerController>();
	if (!PC || !OtherPC) return;
	if (PC->IsInBattle() || OtherPC->IsInBattle()) return;

	bBattleTriggered = true;

	AGameModeBase* GM = GetWorld()->GetAuthGameMode();
	AElfGameModeBase* ElfGM = Cast<AElfGameModeBase>(GM);
	if (ElfGM)
	{
		ElfGM->StartBattle(PC, EBattleType::PvP, OtherPlayer);
	}
}
