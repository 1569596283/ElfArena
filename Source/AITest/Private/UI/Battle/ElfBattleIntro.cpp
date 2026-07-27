#include "UI/Battle/ElfBattleIntro.h"
#include "UI/Battle/ElfBattleController.h"

UElfBattleController* UElfBattleIntro::GetBattleController() const
{
	if (!CachedBattleController)
	{
		CachedBattleController = Cast<UElfBattleController>(WidgetController);
	}
	return CachedBattleController;
}
