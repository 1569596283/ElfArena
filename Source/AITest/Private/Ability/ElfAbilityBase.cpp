#include "Ability/ElfAbilityBase.h"
#include "Battle/ElfBuffManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Elf/ElfManager.h"

void UElfAbilityBase::Init(const FName& InAbilityID, const FGameplayTag& InTrigger, const TArray<FSkillEffect>& InEffects, float InTriggerDelay)
{
	AbilityID = InAbilityID;
	Trigger = InTrigger;
	Effects = InEffects;
	TriggerDelay = InTriggerDelay;
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
		default:
			break;
		}
	}
}
