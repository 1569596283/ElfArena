#include "UI/Battle/ElfBattleUserWidget.h"
#include "UI/Battle/ElfBattleController.h"

UElfBattleController* UElfBattleUserWidget::GetBattleController() const
{
	if (!CachedBattleController)
	{
		CachedBattleController = Cast<UElfBattleController>(WidgetController);
	}
	return CachedBattleController;
}
