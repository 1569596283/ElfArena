// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/ElfInputConfig.h"
#include "InputAction.h"

const UInputAction* UElfInputConfig::FindInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FElfInputAction& Action : InputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}
	if (bLogNotFound) {
		UE_LOG(LogTemp, Error, TEXT("无法找到InputTag [%s]对应的InputAction ,InputConfig [%s]"), *InputTag.ToString(), *GetNameSafe(this));
	}
	return nullptr;
}
