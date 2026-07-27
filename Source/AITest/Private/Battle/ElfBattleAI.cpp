#include "Battle/ElfBattleAI.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Data/ElfTypeChart.h"
#include "Data/ElfBaseData.h"
#include "Data/ElfSkillData.h"
#include "Skill/ElfSkillBase.h"
#include "Game/ElfGameInstance.h"

int32 UElfBattleAI::ChooseSkill(EInfoSide SelfSide, EInfoSide EnemySide,
	UElfBattleController* BC, UElfBattleModel* BM, UElfGameInstance* GI)
{
	if (!BC || !BM) return -1;

	FBattleSideData& SelfData = (SelfSide == EInfoSide::Self) ? BM->PlayerSide : BM->EnemySide;
	FBattleSideData& EnemyData = (EnemySide == EInfoSide::Self) ? BM->PlayerSide : BM->EnemySide;
	FElfCreatureInstance* Self = SelfData.GetActiveCreature();
	FElfCreatureInstance* Enemy = EnemyData.GetActiveCreature();
	if (!Self || !Enemy) return -1;

	FElfBaseData EnemyBase, SelfBase;
	EElfType EnemyT1 = EElfType::None, EnemyT2 = EElfType::None;
	EElfType SelfT1 = EElfType::None, SelfT2 = EElfType::None;
	if (GI)
	{
		if (GI->GetElfBaseData(Enemy->CreatureRowName, EnemyBase))
		{
			EnemyT1 = EnemyBase.Type1;
			EnemyT2 = EnemyBase.Type2;
		}
		if (GI->GetElfBaseData(Self->CreatureRowName, SelfBase))
		{
			SelfT1 = SelfBase.Type1;
			SelfT2 = SelfBase.Type2;
		}
	}

	// Build valid skill list
	struct FSlotInfo { int32 Index; UElfSkillBase* Inst; };
	TArray<FSlotInfo> All;
	for (int32 i = 0; i < Self->EquippedSkills.Num(); i++)
	{
		UElfSkillBase* Inst = SelfData.GetActiveSkillInstance(i);
		if (!Inst) continue;
		if (Inst->GetInstanceEnergyCost() > Self->CurrentEnergy) continue;
		if (Inst->GetSkillDataRef().SkillType == ESkillType::Defense && Self->LastUsedSkillType == ESkillType::Defense) continue;
		All.Add({ i, Inst });
	}

	if (All.IsEmpty()) return -1;

	auto GetMult = [&](EElfType AtkType) {
		return ElfTypeChart::GetMultiplier(AtkType, EnemyT1, EnemyT2, GI ? GI->TypeChartTable : nullptr);
	};

	auto GetSelfMult = [&](EElfType AtkType) {
		return ElfTypeChart::GetMultiplier(AtkType, SelfT1, SelfT2, GI ? GI->TypeChartTable : nullptr);
	};

	// 1. Super-effective attack
	if (FMath::FRand() < SuperEffectiveWeight)
	{
		TArray<int32> Candidates;
		for (const auto& Slot : All)
		{
			if (Slot.Inst->GetSkillDataRef().SkillType != ESkillType::Attack) continue;
			if (GetMult(Slot.Inst->GetSkillDataRef().ElementType) > 1.0f)
				Candidates.Add(Slot.Index);
		}
		if (!Candidates.IsEmpty())
		{
			Candidates.Sort([&](int32 A, int32 B) {
				float MA = 0, MB = 0;
				for (auto& S : All) { if (S.Index == A) for (auto& E : S.Inst->GetSkillDataRef().Effects) if (E.Type == EEffectType::Power) MA = E.Value; }
				for (auto& S : All) { if (S.Index == B) for (auto& E : S.Inst->GetSkillDataRef().Effects) if (E.Type == EEffectType::Power) MB = E.Value; }
				return MA > MB;
			});
			return Candidates[0];
		}
	}

	// 2. Buff / restore energy
	bool bEnergyLow = Self->CurrentEnergy <= 3;
	if (FMath::FRand() < BuffOrEnergyWeight)
	{
		TArray<int32> Candidates;
		for (const auto& Slot : All)
		{
			if (Slot.Inst->GetSkillDataRef().SkillType != ESkillType::Status) continue;
			bool bUseful = false;
			for (const FSkillEffect& E : Slot.Inst->GetSkillDataRef().Effects)
			{
				if (E.Type == EEffectType::RestoreEnergy) bUseful = true;
				if (E.Type == EEffectType::AddBuff) bUseful = true;
				if (E.Type == EEffectType::HealHPPercent && Self->CurrentHP < 100) bUseful = true;
			}
			if (bUseful) Candidates.Add(Slot.Index);
		}
		if (!Candidates.IsEmpty()) return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	}

	// 3. Check if self is weak to enemy
	bool bWeak = false;
	float WorstMult = 0;
	for (auto Atk : { EnemyT1, EnemyT2 })
	{
		if (Atk == EElfType::None) continue;
		float M = GetSelfMult(Atk);
		if (M > WorstMult) WorstMult = M;
		if (M > 1.0f) bWeak = true;
	}

	if (bWeak)
	{
		if (FMath::FRand() < DefenseWhenWeakWeight)
		{
			TArray<int32> DefSlots;
			for (const auto& Slot : All)
				if (Slot.Inst->GetSkillDataRef().SkillType == ESkillType::Defense)
					DefSlots.Add(Slot.Index);
			if (!DefSlots.IsEmpty()) return DefSlots[FMath::RandRange(0, DefSlots.Num() - 1)];
		}

		if (FMath::FRand() < SwitchWhenWeakWeight)
		{
			return -2;
		}
	}

	// 4. Random
	return All[FMath::RandRange(0, All.Num() - 1)].Index;
}

int32 UElfBattleAI::ChooseSwitch(EInfoSide SelfSide, EInfoSide EnemySide,
	UElfBattleModel* BM, UElfGameInstance* GI)
{
	if (!BM) return -1;

	FBattleSideData& SelfData = (SelfSide == EInfoSide::Self) ? BM->PlayerSide : BM->EnemySide;
	FBattleSideData& EnemyData = (EnemySide == EInfoSide::Self) ? BM->PlayerSide : BM->EnemySide;
	FElfCreatureInstance* Enemy = EnemyData.GetActiveCreature();
	if (!Enemy) return -1;

	FElfBaseData EnemyBase;
	EElfType EnemyT1 = EElfType::None, EnemyT2 = EElfType::None;
	if (GI && GI->GetElfBaseData(Enemy->CreatureRowName, EnemyBase))
	{
		EnemyT1 = EnemyBase.Type1;
		EnemyT2 = EnemyBase.Type2;
	}

	int32 BestSlot = -1;
	float BestMult = 999.0f;

	for (int32 i = 0; i < SelfData.Team.Num(); i++)
	{
		if (SelfData.Team[i].CurrentHP <= 0) continue;
		if (i == SelfData.ActiveIndex) continue;

		FElfBaseData Base;
		if (GI && GI->GetElfBaseData(SelfData.Team[i].CreatureRowName, Base))
		{
			float M1 = ElfTypeChart::GetMultiplier(EnemyT1, Base.Type1, Base.Type2, GI->TypeChartTable);
			float M2 = ElfTypeChart::GetMultiplier(EnemyT2, Base.Type1, Base.Type2, GI->TypeChartTable);
			float Avg = (M1 + M2) / 2.0f;
			if (Avg < BestMult)
			{
				BestMult = Avg;
				BestSlot = i;
			}
		}
	}

	return BestSlot;
}
