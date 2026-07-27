#include "Battle/ElfBuffManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Game/ElfGameInstance.h"

void UElfBuffManager::Init(UElfBattleController* InBC, UElfBattleModel* InBM)
{
	BattleController = InBC;
	BattleModel = InBM;
	BuffDefCache.Empty();
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

const FEffectDef* UElfBuffManager::GetBuffDefCached(FName RowName) const
{
	if (RowName.IsNone()) return nullptr;
	if (const FEffectDef* Cached = BuffDefCache.Find(RowName))
		return Cached;
	UElfGameInstance* GI = GetGameInstance();
	if (!GI) return nullptr;
	FEffectDef Def;
	if (GI->GetBuffDef(RowName, Def))
		return &BuffDefCache.Add(RowName, Def);
	return nullptr;
}

static TArray<float> BuildParamsFromDef(const FEffectDef& Def)
{
	TArray<float> P;
	if (Def.EffectID == EEffectID::StatModPercent)
	{
		P.Add(static_cast<float>(Def.TargetStat));
		P.Add(Def.Value);
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
	return P;
}

void UElfBuffManager::ApplyBuffToTarget(EInfoSide TargetSide, FName BuffDefRowName, const FEffectDef& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
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

	if (bIsBuff)
		OnBeforeAddBuff(TargetSide, NewBuff);

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

void UElfBuffManager::ApplyBuffToSide(EInfoSide Side, FName BuffDefRowName, const FEffectDef& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
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

	if (bIsBuff)
		OnBeforeAddBuff(Side, NewBuff);

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
		if (Buff->EffectID == EEffectID::ModifyEnergyCostAndPower && Buff->Params.IsValidIndex(0))
			Cost = FMath::Max(0, Cost + FMath::RoundToInt(Buff->Params[0] * Buff->StackCount));
	}
	return Cost;
}

int32 UElfBuffManager::GetModifiedSpeed(EInfoSide Side, int32 BaseSpeed)
{
	int32 Speed = BaseSpeed;
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

void UElfBuffManager::GetModifiedStats(EInfoSide Side, FElfCalculatedStats& InOutStats)
{
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (!Creature) return;

	for (const FActiveBuff& Buff : Creature->ActiveBuffs)
	{
		if (Buff.EffectID == EEffectID::StatModPercent && Buff.Params.Num() >= 2)
		{
			int32 StatIdx = FMath::RoundToInt(Buff.Params[0]);
			float Percent = Buff.Params[1] * Buff.StackCount;
			switch (StatIdx)
			{
			case 0: InOutStats.ATK  = FMath::Max(1, FMath::RoundToInt(InOutStats.ATK  * (1.0f + Percent))); break;
			case 1: InOutStats.MATK = FMath::Max(1, FMath::RoundToInt(InOutStats.MATK * (1.0f + Percent))); break;
			case 2: InOutStats.DEF  = FMath::Max(1, FMath::RoundToInt(InOutStats.DEF  * (1.0f + Percent))); break;
			case 3: InOutStats.MDEF = FMath::Max(1, FMath::RoundToInt(InOutStats.MDEF * (1.0f + Percent))); break;
			}
		}
	}
}

void UElfBuffManager::ProcessTurnEndEffects(EInfoSide Side)
{
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	FElfCalculatedStats* Stats = GetActiveStats(Side);
	if (!Creature || !Stats) return;

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
		}
		if (Buff->EffectID == EEffectID::TurnEndDamage && Buff->Params.IsValidIndex(0))
		{
			int32 Dmg = FMath::Max(1, FMath::RoundToInt(Stats->MaxHP * Buff->Params[0] * Buff->StackCount));
			Creature->CurrentHP = FMath::Max(0, Creature->CurrentHP - Dmg);
			bHPChanged = true;
		}
		if (Buff->EffectID == EEffectID::TurnEndElementDamage && Buff->Params.Num() >= 2)
		{
			float Pct = Buff->Params[1] * Buff->StackCount;
			int32 Dmg = FMath::Max(1, FMath::RoundToInt(Stats->MaxHP * Pct));
			Creature->CurrentHP = FMath::Max(0, Creature->CurrentHP - Dmg);
			bHPChanged = true;
		}
		if (Buff->EffectID == EEffectID::FreezeHP && Buff->Params.IsValidIndex(0))
		{
			float FreezePct = Buff->Params[0] * Buff->StackCount;
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
