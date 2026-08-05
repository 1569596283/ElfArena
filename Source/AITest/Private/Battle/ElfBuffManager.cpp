#include "Battle/ElfBuffManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Game/ElfGameInstance.h"
#include "Event/ElfEventManager.h"
#include "ElfGameplayTags.h"

void UElfBuffManager::Init(UElfBattleController* InBC, UElfBattleModel* InBM)
{
	BattleController = InBC;
	BattleModel = InBM;
	BuffDataCache.Empty();
}

void UElfBuffManager::CollectActiveBuffs(EInfoSide Side, TArray<const FActiveBuff*>& OutBuffs)
{
	FBattleSideData* SideData = GetSide(Side);
	if (!SideData) return;

	for (const FActiveBuff& B : SideData->SideBuffs)
		OutBuffs.Add(&B);

	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (Creature)
	{
		for (const FActiveBuff& B : Creature->ActiveBuffs)
			OutBuffs.Add(&B);
	}
}

const FEffectData* UElfBuffManager::GetBuffDataCached(FName RowName) const
{
	if (RowName.IsNone()) return nullptr;
	if (const FEffectData* Cached = BuffDataCache.Find(RowName))
		return Cached;
	UElfGameInstance* GI = GetGameInstance();
	if (!GI) return nullptr;
	FEffectData Def;
	if (GI->GetBuffData(RowName, Def))
		return &BuffDataCache.Add(RowName, Def);
	return nullptr;
}

static TArray<float> BuildParamsFromDef(const FEffectData& Def)
{
	TArray<float> P;
	if (Def.EffectID == EEffectID::StatModPercent)
	{
		P.Add(static_cast<float>(Def.StatFlags));
		P.Add(Def.Value / 100.0f);
	}
	else if (Def.EffectID == EEffectID::ModifyFlat)
	{
		P.Add(static_cast<float>(Def.TargetStat));
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::ModifySpeed)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::ModifyEnergyCost)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::ModifyEnergyCostAndPower)
	{
		P.Add(Def.Value);
		P.Add(Def.SecondaryValue);
	}
	else if (Def.EffectID == EEffectID::ExtraBuffStack)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::TurnEndDamage)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::FreezeHP)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::TurnEndElementDamage)
	{
		P.Add(Def.SecondaryValue);
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::EnterDrainEnergy)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::TurnEndRestoreEnergy)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::ModifyHitCount)
	{
		P.Add(Def.Value);
	}
	else if (Def.EffectID == EEffectID::DirectDamageGain || Def.EffectID == EEffectID::DirectDamageReduce)
	{
		P.Add(static_cast<float>(Def.GainCondition));
		P.Add(Def.Value);
	}
	return P;
}

// 同属性百分比修正的反方向抵消（净额）：增益/减益按重叠属性的总百分比互相抵消层数
static void ApplyOppositeCancellation(TArray<FActiveBuff>& BuffList, FActiveBuff& NewBuff)
{
	if (NewBuff.EffectID != EEffectID::StatModPercent || NewBuff.Params.Num() < 2)
		return;

	// 从列表末尾向前抵消，直到新 Buff 抵消完或无反方向 Buff
	for (int32 i = BuffList.Num() - 1; i >= 0 && NewBuff.StackCount > 0; i--)
	{
		FActiveBuff& Existing = BuffList[i];
		if (Existing.EffectID != EEffectID::StatModPercent || Existing.Params.Num() < 2)
			continue;
		// 属性位掩码有重叠才抵消
		if ((FMath::RoundToInt(Existing.Params[0]) & FMath::RoundToInt(NewBuff.Params[0])) == 0)
			continue;
		if (Existing.bIsBuff == NewBuff.bIsBuff)
			continue;

		float NewPerUnit = FMath::Abs(NewBuff.Params[1]);
		float OldPerUnit = FMath::Abs(Existing.Params[1]);
		if (NewPerUnit <= KINDA_SMALL_NUMBER || OldPerUnit <= KINDA_SMALL_NUMBER)
			continue;

		// 按总百分比取较小者抵消
		float NewTotal = FMath::Abs(NewBuff.Params[1]) * NewBuff.StackCount;
		float OldTotal = FMath::Abs(Existing.Params[1]) * Existing.StackCount;
		float CancelAbs = FMath::Min(NewTotal, OldTotal);

		NewBuff.StackCount -= FMath::RoundToInt(CancelAbs / NewPerUnit);
		Existing.StackCount -= FMath::RoundToInt(CancelAbs / OldPerUnit);

		if (Existing.StackCount <= 0)
			BuffList.RemoveAt(i);
	}

	if (NewBuff.StackCount < 0)
		NewBuff.StackCount = 0;
}

void UElfBuffManager::ApplyBuffToTarget(EInfoSide TargetSide, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
{
	FElfCreatureInstance* Target = GetActiveCreature(TargetSide);
	if (!Target) return;

	FActiveBuff NewBuff;
	NewBuff.BuffDefRowName = BuffDefRowName;
	NewBuff.bPersistent = Def.bPersistent;
	NewBuff.EffectID = Def.EffectID;
	NewBuff.Params = BuildParamsFromDef(Def);
	NewBuff.StackCount = (OverrideStack > 0) ? OverrideStack : 1;
	NewBuff.RemainingTurns = (OverrideDuration >= 0) ? OverrideDuration : Def.Duration;
	NewBuff.bIsBuff = bIsBuff;
	NewBuff.bIsTraitBuff = Def.bIsTraitBuff;

	if (bIsBuff)
		OnBeforeAddBuff(TargetSide, NewBuff);

	// 同属性反方向净额抵消
	ApplyOppositeCancellation(Target->ActiveBuffs, NewBuff);
	if (NewBuff.StackCount <= 0) return;

	for (FActiveBuff& Existing : Target->ActiveBuffs)
	{
		if (Existing.BuffDefRowName == NewBuff.BuffDefRowName && Existing.EffectID == NewBuff.EffectID)
		{
			Existing.StackCount += NewBuff.StackCount;
			if (Existing.RemainingTurns >= 0)
				Existing.RemainingTurns = FMath::Max(Existing.RemainingTurns, NewBuff.RemainingTurns);
			return;
		}
	}
	Target->ActiveBuffs.Add(NewBuff);
}

void UElfBuffManager::ApplyBuffToSide(EInfoSide Side, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
{
	FBattleSideData* SideData = GetSide(Side);
	if (!SideData) return;

	FActiveBuff NewBuff;
	NewBuff.BuffDefRowName = BuffDefRowName;
	NewBuff.bPersistent = Def.bPersistent;
	NewBuff.EffectID = Def.EffectID;
	NewBuff.Params = BuildParamsFromDef(Def);
	NewBuff.StackCount = (OverrideStack > 0) ? OverrideStack : 1;
	NewBuff.RemainingTurns = (OverrideDuration >= 0) ? OverrideDuration : Def.Duration;
	NewBuff.bIsBuff = bIsBuff;
	NewBuff.bIsTraitBuff = Def.bIsTraitBuff;

	if (bIsBuff)
		OnBeforeAddBuff(Side, NewBuff);

	// 同属性反方向净额抵消
	ApplyOppositeCancellation(SideData->SideBuffs, NewBuff);
	if (NewBuff.StackCount <= 0) return;

	for (FActiveBuff& Existing : SideData->SideBuffs)
	{
		if (Existing.BuffDefRowName == NewBuff.BuffDefRowName && Existing.EffectID == NewBuff.EffectID)
		{
			Existing.StackCount += NewBuff.StackCount;
			if (Existing.RemainingTurns >= 0)
				Existing.RemainingTurns = FMath::Max(Existing.RemainingTurns, NewBuff.RemainingTurns);
			return;
		}
	}
	SideData->SideBuffs.Add(NewBuff);
}

int32 UElfBuffManager::GetModifiedEnergyCost(EInfoSide Side, int32 BaseCost)
{
	int32 Cost = BaseCost;
	TArray<const FActiveBuff*> Buffs;
	CollectActiveBuffs(Side, Buffs);

	for (const FActiveBuff* Buff : Buffs)
	{
		if (Buff->EffectID == EEffectID::ModifyEnergyCost && Buff->Params.IsValidIndex(0))
			Cost = FMath::Max(0, Cost + FMath::RoundToInt(Buff->Params[0] * Buff->StackCount));
		if (Buff->EffectID == EEffectID::ModifyEnergyCostAndPower && Buff->Params.IsValidIndex(1))
			Cost = FMath::Max(0, Cost + FMath::RoundToInt(Buff->Params[1] * Buff->StackCount));
	}
	return Cost;
}

int32 UElfBuffManager::GetModifiedSpeed(EInfoSide Side, int32 BaseSpeed)
{
	// 百分比修正（StatModPercent，含 SPD 主/附加属性）
	FElfCalculatedStats Tmp;
	Tmp.MaxHP = Tmp.ATK = Tmp.MATK = Tmp.DEF = Tmp.MDEF = 0;
	Tmp.SPD = BaseSpeed;
	GetModifiedStats(Side, Tmp);
	int32 Speed = Tmp.SPD;

	// 固定修正（ModifySpeed / ModifyFlat 的 SPD）
	TArray<const FActiveBuff*> Buffs;
	CollectActiveBuffs(Side, Buffs);

	for (const FActiveBuff* Buff : Buffs)
	{
		if (Buff->EffectID == EEffectID::ModifySpeed && Buff->Params.IsValidIndex(0))
			Speed = FMath::Max(0, Speed + FMath::RoundToInt(Buff->Params[0] * Buff->StackCount));
		if (Buff->EffectID == EEffectID::ModifyFlat && Buff->Params.IsValidIndex(0) && FMath::RoundToInt(Buff->Params[0]) == 4)
			if (Buff->Params.IsValidIndex(1))
				Speed = FMath::Max(0, Speed + FMath::RoundToInt(Buff->Params[1] * Buff->StackCount));
	}
	return Speed;
}

int32 UElfBuffManager::GetModifiedHitCount(EInfoSide Side, int32 BaseCount)
{
	int32 Count = BaseCount;
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (!Creature) return Count;

	for (const FActiveBuff& Buff : Creature->ActiveBuffs)
	{
		if (Buff.EffectID == EEffectID::ModifyHitCount && Buff.Params.IsValidIndex(0))
			Count = FMath::Max(1, Count + FMath::RoundToInt(Buff.Params[0] * Buff.StackCount));
		if (Buff.EffectID == EEffectID::DoubleHitCount)
			Count *= 2;
	}
	return Count;
}

float UElfBuffManager::GetDirectDamageMultiplier(EInfoSide AttackerSide, EInfoSide DefenderSide)
{
	float Multiplier = 1.0f;

	// 攻击方直接伤害增益：倍率 = (1 + Value/100 × 单位数) ^ 层数，所有相乘
	TArray<const FActiveBuff*> AttackBuffs;
	CollectActiveBuffs(AttackerSide, AttackBuffs);
	for (const FActiveBuff* Buff : AttackBuffs)
	{
		if (Buff->EffectID == EEffectID::DirectDamageGain && Buff->Params.Num() >= 2)
		{
			int32 Units = GetGainConditionUnits(AttackerSide, DefenderSide, static_cast<EDirectGainCondition>(FMath::RoundToInt(Buff->Params[0])));
			float PerUnit = Buff->Params[1] / 100.0f;
			float Gain = 1.0f + PerUnit * Units * Buff->StackCount;
			Multiplier *= FMath::Max(0.0f, Gain);
		}
	}

	// 防守方直接伤害减免：倍率 = max(0, 1 - Value/100 × 单位数) ^ 层数，所有相乘
	TArray<const FActiveBuff*> DefenseBuffs;
	CollectActiveBuffs(DefenderSide, DefenseBuffs);
	for (const FActiveBuff* Buff : DefenseBuffs)
	{
		if (Buff->EffectID == EEffectID::DirectDamageReduce && Buff->Params.Num() >= 2)
		{
			int32 Units = GetGainConditionUnits(DefenderSide, AttackerSide, static_cast<EDirectGainCondition>(FMath::RoundToInt(Buff->Params[0])));
			float PerUnit = Buff->Params[1] / 100.0f;
			float Reduce = 1.0f - PerUnit * Units * Buff->StackCount;
			Multiplier *= FMath::Max(0.0f, Reduce);
		}
	}

	return Multiplier;
}

void UElfBuffManager::GetModifiedStats(EInfoSide Side, FElfCalculatedStats& InOutStats)
{
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (!Creature) return;

	for (const FActiveBuff& Buff : Creature->ActiveBuffs)
	{
		if (Buff.EffectID == EEffectID::StatModPercent && Buff.Params.Num() >= 2)
		{
			float Percent = Buff.Params[1] * Buff.StackCount;
			// Percent 为比例(0.4=+40%)。增益 ×(1+percent)；减益 ×100/(100+|percent|*100)
			// 即减益 -0.6 → ×100/(100+60)=0.625
			float Multiplier = (Percent >= 0.0f)
				? (1.0f + Percent)
				: 100.0f / (100.0f - Percent * 100.0f);

			auto ApplyToStat = [&](int32 Idx)
			{
				switch (Idx)
				{
				case 0: InOutStats.ATK  = FMath::Max(1, FMath::RoundToInt(InOutStats.ATK  * Multiplier)); break;
				case 1: InOutStats.MATK = FMath::Max(1, FMath::RoundToInt(InOutStats.MATK * Multiplier)); break;
				case 2: InOutStats.DEF  = FMath::Max(1, FMath::RoundToInt(InOutStats.DEF  * Multiplier)); break;
				case 3: InOutStats.MDEF = FMath::Max(1, FMath::RoundToInt(InOutStats.MDEF * Multiplier)); break;
				case 4: InOutStats.SPD  = FMath::Max(1, FMath::RoundToInt(InOutStats.SPD  * Multiplier)); break;
				}
			};

			// 属性位掩码：勾选的所有属性都修正
			uint8 Flags = static_cast<uint8>(FMath::RoundToInt(Buff.Params[0]));
			if (Flags & 1)  ApplyToStat(0); // ATK
			if (Flags & 2)  ApplyToStat(1); // MATK
			if (Flags & 4)  ApplyToStat(2); // DEF
			if (Flags & 8)  ApplyToStat(3); // MDEF
			if (Flags & 16) ApplyToStat(4); // SPD
		}
	}
}

void UElfBuffManager::ProcessTurnEndEffects(EInfoSide Side)
{
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	FElfCalculatedStats* Stats = GetActiveStats(Side);
	if (!Creature || !Stats) return;

	UElfGameInstance* GI = GetGameInstance();
	UElfEventManager* EventManager = GI ? GI->GetSubsystem<UElfEventManager>() : nullptr;

	TArray<const FActiveBuff*> Buffs;
	CollectActiveBuffs(Side, Buffs);

	bool bHPChanged = false;
	bool bEnergyChanged = false;

	for (const FActiveBuff* Buff : Buffs)
	{
		if (Buff->EffectID == EEffectID::TurnEndRestoreEnergy && Buff->Params.IsValidIndex(0))
		{
			int32 Restore = FMath::RoundToInt(Buff->Params[0] * Buff->StackCount);
			Creature->CurrentEnergy = FMath::Min(10, Creature->CurrentEnergy + Restore);
			bEnergyChanged = true;

			// 特性触发：回复能量
			if (EventManager)
				EventManager->BroadcastEvent(FElfGameplayTags::Get().Battle_Trigger_RestoreEnergy, Creature);
		}
		if (Buff->EffectID == EEffectID::TurnEndDamage && Buff->Params.IsValidIndex(0))
		{
			int32 Dmg = FMath::Max(1, FMath::RoundToInt(Stats->MaxHP * Buff->Params[0] / 100.0f * Buff->StackCount));
			Creature->CurrentHP = FMath::Max(0, Creature->CurrentHP - Dmg);
			bHPChanged = true;
		}
		if (Buff->EffectID == EEffectID::TurnEndElementDamage && Buff->Params.Num() >= 2)
		{
			float Pct = Buff->Params[1] / 100.0f * Buff->StackCount;
			int32 Dmg = FMath::Max(1, FMath::RoundToInt(Stats->MaxHP * Pct));
			Creature->CurrentHP = FMath::Max(0, Creature->CurrentHP - Dmg);
			bHPChanged = true;
		}
		if (Buff->EffectID == EEffectID::FreezeHP && Buff->Params.IsValidIndex(0))
		{
			float FreezePct = Buff->Params[0] / 100.0f * Buff->StackCount;
			int32 FreezeThreshold = FMath::RoundToInt(Stats->MaxHP * FreezePct);
			if (Creature->CurrentHP <= FreezeThreshold)
			{
				Creature->CurrentHP = FMath::Max(0, Creature->CurrentHP - FreezeThreshold);
				bHPChanged = true;
			}
		}
	}

	if (bHPChanged)
	{
		if (Side == EInfoSide::Self)
			BattleController->OnSelfCreatureHPChanged.Broadcast(Creature->CurrentHP, Stats->MaxHP);
		else
			BattleController->OnEnemyCreatureHPChanged.Broadcast(Creature->CurrentHP, Stats->MaxHP);
	}
	if (bEnergyChanged)
	{
		if (Side == EInfoSide::Self)
			BattleController->OnSelfCreatureEnergyChanged.Broadcast(Creature->CurrentEnergy);
		else
			BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Creature->CurrentEnergy);
	}
}

void UElfBuffManager::OnCreatureEnteredField(EInfoSide Side)
{
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (!Creature) return;

	FBattleSideData* SideData = GetSide(Side);
	if (!SideData) return;

	for (const FActiveBuff& Buff : SideData->SideBuffs)
	{
		if (Buff.EffectID == EEffectID::EnterDrainEnergy && Buff.Params.IsValidIndex(0))
		{
			int32 Cost = FMath::RoundToInt(Buff.Params[0] * Buff.StackCount);
			Creature->CurrentEnergy = FMath::Max(0, Creature->CurrentEnergy - Cost);
			if (Side == EInfoSide::Self)
				BattleController->OnSelfCreatureEnergyChanged.Broadcast(Creature->CurrentEnergy);
			else
				BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Creature->CurrentEnergy);
		}
	}
}

void UElfBuffManager::OnBeforeAddBuff(EInfoSide Side, FActiveBuff& NewBuff)
{
	FBattleSideData* SideData = GetSide(Side);
	if (!SideData || NewBuff.BuffDefRowName.IsNone()) return;

	for (const FActiveBuff& SideBuff : SideData->SideBuffs)
	{
		if (SideBuff.EffectID == EEffectID::ExtraBuffStack && SideBuff.Params.IsValidIndex(0))
		{
			int32 Extra = FMath::RoundToInt(SideBuff.Params[0] * SideBuff.StackCount);
			NewBuff.StackCount += Extra;
			break;
		}
	}
}

bool UElfBuffManager::IsSwitchBlocked(EInfoSide Side)
{
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (!Creature) return false;

	for (const FActiveBuff& Buff : Creature->ActiveBuffs)
		if (Buff.EffectID == EEffectID::BlockSwitch)
			return true;
	return false;
}

void UElfBuffManager::TickBuffs(EInfoSide Side)
{
	auto TickOne = [](TArray<FActiveBuff>& Buffs)
	{
		for (int32 i = Buffs.Num() - 1; i >= 0; i--)
		{
			if (Buffs[i].RemainingTurns > 0)
			{
				Buffs[i].RemainingTurns--;
				if (Buffs[i].RemainingTurns == 0)
				{
					Buffs.RemoveAt(i);
				}
			}
		}
	};

	FBattleSideData* SideData = GetSide(Side);
	if (SideData)
		TickOne(SideData->SideBuffs);

	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (Creature)
		TickOne(Creature->ActiveBuffs);
}

int32 UElfBuffManager::ClearGeneralBuffs(EInfoSide Side, bool bClearBuffs, bool bClearDebuffs)
{
	int32 Cleared = 0;

	auto Filter = [&](TArray<FActiveBuff>& Buffs)
	{
		for (int32 i = Buffs.Num() - 1; i >= 0; i--)
		{
			const FActiveBuff& Buff = Buffs[i];
			if (Buff.bIsTraitBuff) continue; // 特性buff 不清除
			if (bClearBuffs && Buff.bIsBuff)
			{
				Buffs.RemoveAt(i);
				Cleared++;
			}
			else if (bClearDebuffs && !Buff.bIsBuff)
			{
				Buffs.RemoveAt(i);
				Cleared++;
			}
		}
	};

	FBattleSideData* SideData = GetSide(Side);
	if (SideData)
		Filter(SideData->SideBuffs);

	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (Creature)
		Filter(Creature->ActiveBuffs);

	return Cleared;
}

FBattleSideData* UElfBuffManager::GetSide(EInfoSide Side)
{
	if (!BattleModel) return nullptr;
	return (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
}

FElfCreatureInstance* UElfBuffManager::GetActiveCreature(EInfoSide Side)
{
	FBattleSideData* SideData = GetSide(Side);
	return SideData ? SideData->GetActiveCreature() : nullptr;
}

FElfCalculatedStats* UElfBuffManager::GetActiveStats(EInfoSide Side)
{
	FBattleSideData* SideData = GetSide(Side);
	return SideData ? SideData->GetActiveStats() : nullptr;
}

UElfGameInstance* UElfBuffManager::GetGameInstance() const
{
	if (!BattleController) return nullptr;
	APlayerController* PC = BattleController->GetOwnerPC();
	return PC ? PC->GetGameInstance<UElfGameInstance>() : nullptr;
}

int32 UElfBuffManager::GetGainConditionUnits(EInfoSide OwnerSide, EInfoSide OtherSide, EDirectGainCondition Cond)
{
	switch (Cond)
	{
	case EDirectGainCondition::TargetBuffCount:
	{
		TArray<const FActiveBuff*> Buffs;
		CollectActiveBuffs(OtherSide, Buffs);
		int32 Count = 0;
		for (const FActiveBuff* Buff : Buffs)
			if (Buff->bIsBuff)
				Count++;
		return Count;
	}
	case EDirectGainCondition::SelfBuffCount:
	{
		TArray<const FActiveBuff*> Buffs;
		CollectActiveBuffs(OwnerSide, Buffs);
		int32 Count = 0;
		for (const FActiveBuff* Buff : Buffs)
			if (Buff->bIsBuff)
				Count++;
		return Count;
	}
	default:
		return 1;
	}
}
