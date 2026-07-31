#include "ElfGameplayTags.h"
#include "GameplayTagsManager.h"

FElfGameplayTags FElfGameplayTags::GameplayTags;

void FElfGameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	GameplayTags.Input_Jump = Manager.AddNativeGameplayTag(FName("Input.Jump"));

	GameplayTags.Input_Space = Manager.AddNativeGameplayTag(FName("Input.Space"));
	GameplayTags.Input_Slot1 = Manager.AddNativeGameplayTag(FName("Input.Slot1"));
	GameplayTags.Input_Slot2 = Manager.AddNativeGameplayTag(FName("Input.Slot2"));
	GameplayTags.Input_Slot3 = Manager.AddNativeGameplayTag(FName("Input.Slot3"));
	GameplayTags.Input_Slot4 = Manager.AddNativeGameplayTag(FName("Input.Slot4"));
	GameplayTags.Input_Slot5 = Manager.AddNativeGameplayTag(FName("Input.Slot5"));
	GameplayTags.Input_Slot6 = Manager.AddNativeGameplayTag(FName("Input.Slot6"));

	GameplayTags.Input_X = Manager.AddNativeGameplayTag(FName("Input.X"));
	GameplayTags.Input_Q = Manager.AddNativeGameplayTag(FName("Input.Q"));
	GameplayTags.Input_W = Manager.AddNativeGameplayTag(FName("Input.W"));
	GameplayTags.Input_E = Manager.AddNativeGameplayTag(FName("Input.E"));
	GameplayTags.Input_R = Manager.AddNativeGameplayTag(FName("Input.R"));
	GameplayTags.Input_Escape = Manager.AddNativeGameplayTag(FName("Input.Escape"));
}
