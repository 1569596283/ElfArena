#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ElfAnimInstance.generated.h"

UCLASS()
class AITEST_API UElfAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 移动速度，用于 idle/walk/run 混合
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	float GroundSpeed = 0.0f;

	// 是否在空中
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	bool bIsInAir = false;

	// 移动方向，用于 BlendSpace 方向混合
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	float Direction = 0.0f;
};
