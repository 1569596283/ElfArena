#include "ElfBlueprintFunctionLibrary.h"
#include "Game/ElfGameInstance.h"

UElfGameInstance* UElfBlueprintFunctionLibrary::GetElfGameInstance(UObject* WorldContext)
{
	if (!WorldContext) return nullptr;
	UWorld* World = WorldContext->GetWorld();
	return World ? World->GetGameInstance<UElfGameInstance>() : nullptr;
}
