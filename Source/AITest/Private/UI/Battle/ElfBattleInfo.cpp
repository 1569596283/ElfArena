#include "UI/Battle/ElfBattleInfo.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"

void UElfBattleInfo::Init(EInfoSide InSide)
{
	Side = InSide;
	OnInit(InSide);
}

UElfBattleController* UElfBattleInfo::GetBattleController() const
{
	if (!CachedBattleController)
	{
		CachedBattleController = Cast<UElfBattleController>(WidgetController);
	}
	return CachedBattleController;
}

void UElfBattleInfo::GetCurrentStats(int32& OutHP, int32& OutMaxHP, int32& OutEnergy) const
{
	OutHP = 0;
	OutMaxHP = 0;
	OutEnergy = 0;

	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return;

	UElfBattleModel* Model = Controller->GetBattleModel();
	if (!Model) return;

	FBattleSideData* SideData = (Side == EInfoSide::Self) ? &Model->PlayerSide : &Model->EnemySide;
	if (!SideData) return;

	const FElfCreatureInstance* Creature = SideData->GetActiveCreature();
	const FElfCalculatedStats* Stats = SideData->GetActiveStats();
	if (Creature && Stats)
	{
		OutHP = Creature->CurrentHP;
		OutMaxHP = Stats->MaxHP;
		OutEnergy = Creature->CurrentEnergy;
	}
}
