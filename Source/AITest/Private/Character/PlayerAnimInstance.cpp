#include "Character/PlayerAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/PlayerCharacter.h"

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APlayerCharacter* Character = Cast<APlayerCharacter>(TryGetPawnOwner());
	if (!Character) return;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	GroundSpeed = Movement->Velocity.Length();
	bIsInAir = Movement->IsFalling();
}
