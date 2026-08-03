#include "Event/ElfEventManager.h"

void UElfEventManager::BroadcastEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature)
{
	OnGameplayEvent.Broadcast(EventTag, Creature);
}
