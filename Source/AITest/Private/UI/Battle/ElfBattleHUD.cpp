#include "UI/Battle/ElfBattleHUD.h"
#include "UI/Battle/ElfBattleController.h"

UElfBattleController* UElfBattleHUD::GetBattleController() const
{
	if (!CachedBattleController)
	{
		CachedBattleController = Cast<UElfBattleController>(WidgetController);
	}
	return CachedBattleController;
}
