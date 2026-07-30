#include "UI/Battle/ElfBattleInfo.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"

void UElfBattleInfo::Init(EInfoSide InSide)
{
	Side = InSide;

	if (UElfBattleController* BC = GetBattleController())
	{
		if (Side == EInfoSide::Self)
		{
			BC->OnSelfCreatureHPChanged.AddDynamic(this, &UElfBattleInfo::OnHPChanged);
			BC->OnSelfCreatureEnergyChanged.AddDynamic(this, &UElfBattleInfo::OnEnergyChanged);
		}
		else
		{
			BC->OnEnemyCreatureHPChanged.AddDynamic(this, &UElfBattleInfo::OnHPChanged);
			BC->OnEnemyCreatureEnergyChanged.AddDynamic(this, &UElfBattleInfo::OnEnergyChanged);
		}
		BC->OnCreatureSwitched.AddDynamic(this, &UElfBattleInfo::OnCreatureSwitched);
	}

	OnInit(InSide);
}

void UElfBattleInfo::OnHPChanged(int32 NewHP, int32 MaxHP)
{
	BP_OnHPChanged(NewHP, MaxHP);
}

void UElfBattleInfo::OnEnergyChanged(int32 NewEnergy)
{
	BP_OnEnergyChanged(NewEnergy);
}

void UElfBattleInfo::OnCreatureSwitched(EInfoSide InSide)
{
	if (InSide == Side)
	{
		int32 HP, MaxHP, Energy;
		GetCurrentStats(HP, MaxHP, Energy);
		BP_OnHPChanged(HP, MaxHP);
		BP_OnEnergyChanged(Energy);
		BP_OnCreatureSwitched();
	}
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
