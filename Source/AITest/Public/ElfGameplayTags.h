#pragma once

#include "GameplayTagContainer.h"

struct FElfGameplayTags
{
public:
    static const FElfGameplayTags& Get() { return GameplayTags; }
    static void InitializeNativeGameplayTags();

    FGameplayTag Input_Jump;

private:
    static FElfGameplayTags GameplayTags;
};
