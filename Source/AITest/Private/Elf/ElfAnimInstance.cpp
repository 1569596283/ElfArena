#include "Elf/ElfAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Elf/ElfCharacterBase.h"
#include "KismetAnimationLibrary.h"

void UElfAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AElfCharacterBase* Character = Cast<AElfCharacterBase>(TryGetPawnOwner());
	if (!Character) return;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	GroundSpeed = Movement->Velocity.Length();
	bIsInAir = Movement->IsFalling();
	Direction = UKismetAnimationLibrary::CalculateDirection(Movement->Velocity, Character->GetActorRotation());
}

