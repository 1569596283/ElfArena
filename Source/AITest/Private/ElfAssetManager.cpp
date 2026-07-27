#include "ElfAssetManager.h"
#include "ElfGameplayTags.h"

UElfAssetManager& UElfAssetManager::Get()
{
	check(GEngine);
	UElfAssetManager* ElfAssteManager = CastChecked<UElfAssetManager>(GEngine->AssetManager);
	return *ElfAssteManager;
}

void UElfAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	FElfGameplayTags::InitializeNativeGameplayTags();
}
