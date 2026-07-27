#include "ElfGameplayTags.h"
#include "GameplayTagsManager.h"

FElfGameplayTags FElfGameplayTags::GameplayTags;

void FElfGameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	GameplayTags.Input_Jump = Manager.AddNativeGameplayTag(FName("Input.Jump"));
}
