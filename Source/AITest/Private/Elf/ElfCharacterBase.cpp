#include "Elf/ElfCharacterBase.h"
#include "Net/UnrealNetwork.h"

AElfCharacterBase::AElfCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AElfCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AElfCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AElfCharacterBase, CreatureData);
}

void AElfCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

