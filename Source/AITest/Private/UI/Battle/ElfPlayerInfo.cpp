#include "UI/Battle/ElfPlayerInfo.h"
#include "UI/Battle/ElfBattleController.h"

UElfBattleController* UElfPlayerInfo::GetBattleController() const
{
	if (!CachedBattleController)
	{
		CachedBattleController = Cast<UElfBattleController>(WidgetController);
	}
	return CachedBattleController;
}
