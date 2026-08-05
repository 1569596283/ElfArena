#include "Ability/ElfAbilityBase.h"
#include "Battle/ElfBuffManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Elf/ElfManager.h"
#include "ElfGameplayTags.h"

void UElfAbilityBase::Init(const FName& InAbilityID, const FGameplayTag& InTrigger, const TArray<FSkillEffect>& InEffects, float InTriggerDelay)
{
	AbilityID = InAbilityID;
	Trigger = InTrigger;
	Effects = InEffects;
	TriggerDelay = InTriggerDelay;
}

void UElfAbilityBase::SetTriggerConditions(float InTriggerChance, float InHPThreshold, EElfType InTargetElement, int32 InEnergyCostCondition)
{
	TriggerChance = InTriggerChance;
	HPThreshold = InHPThreshold;
	TargetElement = InTargetElement;
	EnergyCostCondition = InEnergyCostCondition;
}

float UElfAbilityBase::ModifySkillPower(EInfoSide Side, const FSkillData& SkillData) const
{
	// 能耗条件：>=0 时仅匹配该能耗的技能
	if (EnergyCostCondition >= 0 && SkillData.EnergyCost != EnergyCostCondition)
		return 1.0f;

	// 增幅从 Effects 的 Power 效果读取（如 50=+50%）
	float Bonus = 0.0f;
	for (const FSkillEffect& Effect : Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			Bonus = Effect.Value;
			break;
		}
	}
	if (Bonus <= 0.0f) return 1.0f;
	return 1.0f + Bonus / 100.0f;
}

void UElfAbilityBase::ModifyEnergyCost(EInfoSide Side, const FSkillData& SkillData, int32& InOutCost) const
{
	// 默认不改，子类重写（如 水系：防御技能能耗-2）
}

void UElfAbilityBase::SetContext(UElfBattleModel* InModel, UElfBuffManager* InBuffManager, UElfTurnManager* InTurnManager, UElfBattleController* InBattleController)
{
	BattleModel = InModel;
	BuffManager = InBuffManager;
	TurnManager = InTurnManager;
	BattleController = InBattleController;
}

bool UElfAbilityBase::CanTrigger(const FElfCreatureInstance* Creature) const
{
	// UseElementSkill：技能属性必须匹配 TargetElement（未指定则不限制）
	if (Trigger == FElfGameplayTags::Get().Battle_Trigger_UseElementSkill)
	{
		if (TargetElement != EElfType::None)
		{
			if (!Creature || Creature->LastSkillElement != TargetElement)
				return false;
		}
	}
	return true;
}

int32 UElfAbilityBase::GetCreatureSide(const FElfCreatureInstance* Creature) const
{
	if (!Creature || !BattleModel) return -1;

	for (int32 SideIdx = 0; SideIdx < 2; SideIdx++)
	{
		EInfoSide Side = (SideIdx == 0) ? EInfoSide::Self : EInfoSide::Enemy;
		FBattleSideData* SideData = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
		if (!SideData) continue;

		for (const FElfCreatureInstance& C : SideData->Team)
		{
			if (&C == Creature)
				return SideIdx;
		}
	}
	return -1;
}

void UElfAbilityBase::TriggerAbility(const FElfCreatureInstance* Creature)
{
	if (!Creature || !BattleModel) return;

	int32 SideIdx = GetCreatureSide(Creature);
	if (SideIdx < 0) return;

	EInfoSide Side = (SideIdx == 0) ? EInfoSide::Self : EInfoSide::Enemy;
	EInfoSide TargetSide = (Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;

	FBattleSideData* SelfSide = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
	if (!SelfSide) return;

	FElfCreatureInstance* Actor = SelfSide->GetActiveCreature();
	FElfCalculatedStats* ActorStats = SelfSide->GetActiveStats();
	if (!Actor) return;

	// 遍历 Effects 执行通用效果
	for (const FSkillEffect& Effect : Effects)
	{
		switch (Effect.Type)
		{
		case EEffectType::HealHPPercent:
		{
			if (ActorStats)
			{
				int32 Heal = FMath::RoundToInt(ActorStats->MaxHP * Effect.Value / 100.0f);
				Actor->CurrentHP = FMath::Min(ActorStats->MaxHP, Actor->CurrentHP + Heal);
				if (BattleController)
				{
					if (Side == EInfoSide::Self)
						BattleController->OnSelfCreatureHPChanged.Broadcast(Actor->CurrentHP, ActorStats->MaxHP);
					else
						BattleController->OnEnemyCreatureHPChanged.Broadcast(Actor->CurrentHP, ActorStats->MaxHP);
				}
			}
			break;
		}
		case EEffectType::RestoreEnergy:
		{
			int32 Restore = FMath::RoundToInt(Effect.Value);
			Actor->CurrentEnergy = FMath::Min(10, Actor->CurrentEnergy + Restore);
			if (BattleController)
			{
				if (Side == EInfoSide::Self)
					BattleController->OnSelfCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
				else
					BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
			}
			break;
		}
		case EEffectType::AddBuff:
		case EEffectType::AddDebuff:
		{
			if (!Effect.BuffRowName.IsNone() && BuffManager)
			{
				const FEffectData* Def = BuffManager->GetBuffDataCached(Effect.BuffRowName);
				if (Def)
				{
					EInfoSide BuffTarget = (Effect.EffectTarget == EEffectTarget::Caster) ? Side : TargetSide;
					bool bIsBuff = (Effect.Type == EEffectType::AddBuff);
					int32 StackOverride = (Effect.Value > 0) ? FMath::RoundToInt(Effect.Value) : -1;
					if (Def->TargetType == EBuffTargetType::Side)
						BuffManager->ApplyBuffToSide(BuffTarget, Effect.BuffRowName, *Def, StackOverride, -1, bIsBuff);
					else
						BuffManager->ApplyBuffToTarget(BuffTarget, Effect.BuffRowName, *Def, StackOverride, -1, bIsBuff);
				}
			}
			break;
		}
		case EEffectType::DealDamage:
		{
			// 对目标造成物理威力伤害：威力 × 己方物攻 / 目标物防（带 buff 修正），无属性、不触发受击类效果
			EInfoSide DmgTarget = (Effect.EffectTarget == EEffectTarget::Caster) ? Side : TargetSide;
			FElfCreatureInstance* Target = (DmgTarget == EInfoSide::Self)
				? BattleModel->PlayerSide.GetActiveCreature() : BattleModel->EnemySide.GetActiveCreature();
			FElfCalculatedStats* TargetStats = (DmgTarget == EInfoSide::Self)
				? BattleModel->PlayerSide.GetActiveStats() : BattleModel->EnemySide.GetActiveStats();
			if (Actor && ActorStats && Target && TargetStats && BuffManager)
			{
				FElfCalculatedStats ModOwner = *ActorStats;
				FElfCalculatedStats ModTarget = *TargetStats;
				BuffManager->GetModifiedStats(Side, ModOwner);
				BuffManager->GetModifiedStats(DmgTarget, ModTarget);
				int32 Dmg = FMath::Max(1, FMath::RoundToInt(Effect.Value * ModOwner.ATK / FMath::Max(1, ModTarget.DEF)));
				Dmg = FMath::Max(1, FMath::RoundToInt(Dmg * BuffManager->GetDirectDamageMultiplier(Side, DmgTarget)));
				Target->CurrentHP = FMath::Max(0, Target->CurrentHP - Dmg);
				if (BattleController)
				{
					if (DmgTarget == EInfoSide::Self)
						BattleController->OnSelfCreatureHPChanged.Broadcast(Target->CurrentHP, TargetStats->MaxHP);
					else
						BattleController->OnEnemyCreatureHPChanged.Broadcast(Target->CurrentHP, TargetStats->MaxHP);
				}
			}
			break;
		}
		case EEffectType::DrainEnemyEnergy:
		{
			// 目标失去固定能量
			EInfoSide DrainTarget = (Effect.EffectTarget == EEffectTarget::Caster) ? Side : TargetSide;
			FElfCreatureInstance* Target = (DrainTarget == EInfoSide::Self)
				? BattleModel->PlayerSide.GetActiveCreature() : BattleModel->EnemySide.GetActiveCreature();
			if (Target)
			{
				Target->CurrentEnergy = FMath::Max(0, Target->CurrentEnergy - FMath::RoundToInt(Effect.Value));
				if (BattleController)
				{
					if (DrainTarget == EInfoSide::Self)
						BattleController->OnSelfCreatureEnergyChanged.Broadcast(Target->CurrentEnergy);
					else
						BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Target->CurrentEnergy);
				}
			}
			break;
		}
		case EEffectType::StealEnergy:
		{
			// 偷取：目标失去最多N能量，己方获得实际偷取量（目标没得扣则不加）
			EInfoSide StealTarget = (Effect.EffectTarget == EEffectTarget::Caster) ? Side : TargetSide;
			FElfCreatureInstance* Target = (StealTarget == EInfoSide::Self)
				? BattleModel->PlayerSide.GetActiveCreature() : BattleModel->EnemySide.GetActiveCreature();
			if (Target && Actor)
			{
				int32 Amount = FMath::RoundToInt(Effect.Value);
				int32 Drained = FMath::Min(Amount, Target->CurrentEnergy);
				Target->CurrentEnergy = FMath::Max(0, Target->CurrentEnergy - Drained);
				Actor->CurrentEnergy = FMath::Min(10, Actor->CurrentEnergy + Drained);
				if (BattleController)
				{
					if (StealTarget == EInfoSide::Self)
						BattleController->OnSelfCreatureEnergyChanged.Broadcast(Target->CurrentEnergy);
					else
						BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Target->CurrentEnergy);
					if (Side == EInfoSide::Self)
						BattleController->OnSelfCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
					else
						BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
				}
			}
			break;
		}
		default:
			break;
		}
	}
}
