#pragma once

#include "GameplayTagContainer.h"

struct FElfGameplayTags
{
public:
    static const FElfGameplayTags& Get() { return GameplayTags; }
    static void InitializeNativeGameplayTags();

    FGameplayTag Input_Jump;

	FGameplayTag Input_Slot1;
	FGameplayTag Input_Slot2;
	FGameplayTag Input_Slot3;
	FGameplayTag Input_Slot4;
	FGameplayTag Input_Slot5;
	FGameplayTag Input_Slot6;

	FGameplayTag Input_Space;
	FGameplayTag Input_X;
	FGameplayTag Input_Q;
	FGameplayTag Input_W;
	FGameplayTag Input_E;
	FGameplayTag Input_R;
	FGameplayTag Input_Escape;

private:
    static FElfGameplayTags GameplayTags;
};
