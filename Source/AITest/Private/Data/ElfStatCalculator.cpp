#include "Data/ElfStatCalculator.h"

FElfCalculatedStats UElfStatCalculator::CalculateStats(const FElfBaseData& BaseData)
{
	FElfCalculatedStats Stats;
	Stats.MaxHP = BaseData.BaseHP;
	Stats.ATK = BaseData.BaseATK;
	Stats.MATK = BaseData.BaseMATK;
	Stats.DEF = BaseData.BaseDEF;
	Stats.MDEF = BaseData.BaseMDEF;
	Stats.SPD = BaseData.BaseSPD;
	return Stats;
}
