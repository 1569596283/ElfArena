#include "Battle/ElfTurnManager.h"
#include "Battle/ElfBattleAI.h"
#include "Battle/ElfBuffManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Skill/ElfSkillBase.h"
#include "Skill/Attack/AttackSkillBase.h"
#include "Data/ElfTypeChart.h"
#include "Game/ElfGameInstance.h"
#include "GameFramework/PlayerController.h"

void UElfTurnManager::Init(UElfBattleController* InBC, UElfBattleModel* InBM)
{
	BattleController = InBC;
	BattleModel = InBM;

	BuffManager = NewObject<UElfBuffManager>(this);
	BuffManager->Init(InBC, InBM);

	BattleAI = NewObject<UElfBattleAI>(this);

	if (BattleController)
	{
		BattleController->OnSkillSelected.Clear();
		BattleController->OnSkillSelected.AddDynamic(this, &UElfTurnManager::OnPlayerSkillSelected);
		BattleController->OnDefaultSkillSelected.Clear();
		BattleController->OnDefaultSkillSelected.AddDynamic(this, &UElfTurnManager::OnPlayerDefaultSkillSelected);
	}
}

void UElfTurnManager::StartTurn()
{
	PlayerChosenSlot = -1;
	EnemyChosenSlot = -1;
	PlayerDefaultSlotIndex = -1;
	EnemyDefaultSlotIndex = -1;
	bPlayerUsedDefault = false;
	bEnemyUsedDefault = false;
	bLocalActionChosen = false;
	bRemoteActionChosen = false;

	ChangePhase(ETurnPhase::PlayerCommand);
}

void UElfTurnManager::OnPlayerSkillSelected(int32 SlotIndex)
{
	if (CurrentPhase != ETurnPhase::PlayerCommand) return;

	FElfCreatureInstance* Creature = GetActiveCreature(EInfoSide::Self);
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return;

	UElfSkillBase* SkillInstance = GetActiveSkillInstance(EInfoSide::Self, SlotIndex);
	if (!SkillInstance) return;
	if (BuffManager->GetModifiedEnergyCost(EInfoSide::Self, SkillInstance->GetInstanceEnergyCost()) > Creature->CurrentEnergy) return;
	if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Defense && Creature->LastUsedSkillType == ESkillType::Defense) return;

	PlayerChosenSlot = SlotIndex;
	bLocalActionChosen = true;

	if (BattleModel && BattleModel->BattleType == EBattleType::PvP)
	{
		ChangePhase(ETurnPhase::WaitingForOpponent);
	}
	else
	{
		ChooseEnemyAction();
		OnPlayerActionReady();
	}
}

void UElfTurnManager::OnPlayerDefaultSkillSelected(int32 SlotIndex)
{
	if (CurrentPhase != ETurnPhase::PlayerCommand) return;

	FElfCreatureInstance* Creature = GetActiveCreature(EInfoSide::Self);
	if (!Creature) return;

	UElfSkillBase* SkillInstance = GetActiveDefaultSkillInstance(EInfoSide::Self, SlotIndex);
	if (!SkillInstance) return;
	if (BuffManager->GetModifiedEnergyCost(EInfoSide::Self, SkillInstance->GetInstanceEnergyCost()) > Creature->CurrentEnergy) return;
	if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Defense && Creature->LastUsedSkillType == ESkillType::Defense) return;

	PlayerDefaultSlotIndex = SlotIndex;
	bPlayerUsedDefault = true;
	bLocalActionChosen = true;

	if (BattleModel && BattleModel->BattleType == EBattleType::PvP)
	{
		ChangePhase(ETurnPhase::WaitingForOpponent);
	}
	else
	{
		ChooseEnemyAction();
		OnPlayerActionReady();
	}
}

void UElfTurnManager::OnRemoteActionReceived(int32 SlotIndex)
{
	if (CurrentPhase != ETurnPhase::WaitingForOpponent) return;

	EnemyChosenSlot = SlotIndex;
	bRemoteActionChosen = true;

	OnPlayerActionReady();
}

void UElfTurnManager::OnPlayerSwitchRequest(int32 SlotIndex)
{
	if (CurrentPhase != ETurnPhase::Switch && CurrentPhase != ETurnPhase::PlayerCommand) return;

	if (BuffManager->IsSwitchBlocked(EInfoSide::Self)) return;

	FElfCreatureInstance* Current = GetActiveCreature(EInfoSide::Self);
	if (Current)
	{
		Current->LastUsedSkillType = ESkillType::Attack;
	}

	ChangePhase(ETurnPhase::Switch);
	OnSwitchRequested.Broadcast(EInfoSide::Self, SlotIndex);

	if (BattleModel && BattleModel->BattleType == EBattleType::PvP)
	{
		ChangePhase(ETurnPhase::WaitingForOpponent);
	}
	else
	{
		StartTurn();
	}
}

void UElfTurnManager::ChooseEnemyAction()
{
	EnemyChosenSlot = -1;
	EnemyDefaultSlotIndex = -1;
	bEnemyUsedDefault = false;

	FElfCreatureInstance* Creature = GetActiveCreature(EInfoSide::Enemy);
	UElfGameInstance* GI = GetGameInstance();

	if (BattleModel && BattleModel->BattleType == EBattleType::Wild)
	{
		if (Creature)
		{
			TArray<int32> Valid;
			for (int32 i = 0; i < Creature->EquippedSkills.Num(); i++)
			{
				UElfSkillBase* Inst = GetActiveSkillInstance(EInfoSide::Enemy, i);
				if (Inst && Inst->GetInstanceEnergyCost() <= Creature->CurrentEnergy)
					Valid.Add(i);
			}
			if (!Valid.IsEmpty())
			{
				EnemyChosenSlot = Valid[FMath::RandRange(0, Valid.Num() - 1)];
				return;
			}
		}
	}
	else if (BattleAI && GI)
	{
		int32 Result = BattleAI->ChooseSkill(EInfoSide::Enemy, EInfoSide::Self,
			BattleController, BattleModel, GI);

		if (Result == -2)
		{
			int32 SwitchSlot = BattleAI->ChooseSwitch(EInfoSide::Enemy, EInfoSide::Self,
				BattleModel, GI);
			if (SwitchSlot >= 0)
				OnSwitchRequested.Broadcast(EInfoSide::Enemy, SwitchSlot);

			Result = BattleAI->ChooseSkill(EInfoSide::Enemy, EInfoSide::Self,
				BattleController, BattleModel, GI);
		}

		if (Result >= 0)
		{
			EnemyChosenSlot = Result;
			return;
		}
	}

	FBattleSideData* EnemySide = GetSide(EInfoSide::Enemy);
	if (EnemySide)
	{
		for (int32 i = 0; i < EnemySide->GetDefaultSkillCount(); i++)
		{
			UElfSkillBase* Inst = EnemySide->GetActiveDefaultSkillInstance(i);
			if (Inst)
			{
				bool bHasRestore = false;
				for (const FSkillEffect& Effect : Inst->GetSkillDataRef().Effects)
					if (Effect.Type == EEffectType::RestoreEnergy) bHasRestore = true;

				if (bHasRestore)
				{
					EnemyDefaultSlotIndex = i;
					bEnemyUsedDefault = true;
					return;
				}
			}
		}
	}

	if (Creature)
	{
		for (int32 i = 0; i < Creature->EquippedSkills.Num(); i++)
		{
			UElfSkillBase* Inst = GetActiveSkillInstance(EInfoSide::Enemy, i);
			if (Inst)
			{
				EnemyChosenSlot = i;
				return;
			}
		}
	}
}

void UElfTurnManager::ChangePhase(ETurnPhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnTurnPhaseChanged.Broadcast(NewPhase);
}

void UElfTurnManager::OnPlayerActionReady()
{
	ResolveActions();
}

void UElfTurnManager::ResolveActions()
{
	FTurnAction PlayerAction = bPlayerUsedDefault
		? FTurnAction{ EInfoSide::Self, PlayerDefaultSlotIndex, true }
		: FTurnAction{ EInfoSide::Self, PlayerChosenSlot, false };
	FTurnAction EnemyAction = bEnemyUsedDefault
		? FTurnAction{ EInfoSide::Enemy, EnemyDefaultSlotIndex, true }
		: FTurnAction{ EInfoSide::Enemy, EnemyChosenSlot, false };

	int32 PlayerPriority = PlayerChosenSlot >= 0 ? GetSkillPriorityFor(EInfoSide::Self, PlayerChosenSlot) : -99;
	int32 EnemyPriority = EnemyChosenSlot >= 0 ? GetSkillPriorityFor(EInfoSide::Enemy, EnemyChosenSlot) : -99;

	if (PlayerPriority > EnemyPriority)
	{
		ActionQueue = { PlayerAction, EnemyAction };
	}
	else if (EnemyPriority > PlayerPriority)
	{
		ActionQueue = { EnemyAction, PlayerAction };
	}
	else
	{
		int32 PlayerSpeed = GetEffectiveSpeed(EInfoSide::Self);
		int32 EnemySpeed = GetEffectiveSpeed(EInfoSide::Enemy);

		if (PlayerSpeed >= EnemySpeed)
		{
			ActionQueue = { PlayerAction, EnemyAction };
		}
		else
		{
			ActionQueue = { EnemyAction, PlayerAction };
		}
	}

	ChangePhase(ETurnPhase::Executing);

	if (BattleController)
	{
		BattleController->OnActionPhaseStarted.Broadcast();
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(ExecutionTimer,
			FTimerDelegate::CreateUObject(this, &UElfTurnManager::OnExecutionTimer),
			3.0f, false);
	}
}

void UElfTurnManager::OnExecutionTimer()
{
	if (ActionQueue.Num() < 2)
	{
		EndTurn();
		return;
	}

	FTurnAction& A = ActionQueue[0];
	FTurnAction& B = ActionQueue[1];

	ECounterState Counter = DetermineCounter(A, B);

	if (Counter == ECounterState::FirstCountersSecond)
	{
		Swap(ActionQueue[0], ActionQueue[1]);
	}

	for (int32 i = 0; i < ActionQueue.Num(); i++)
	{
		const FTurnAction& Action = ActionQueue[i];

		FElfCreatureInstance* Actor = GetActiveCreature(Action.Side);
		if (!Actor || Actor->CurrentHP <= 0 || Action.SlotIndex < 0) continue;
		if (!Action.bIsDefault && !Actor->EquippedSkills.IsValidIndex(Action.SlotIndex)) continue;

		UElfSkillBase* SkillInstance = Action.bIsDefault
			? GetActiveDefaultSkillInstance(Action.Side, Action.SlotIndex)
			: GetActiveSkillInstance(Action.Side, Action.SlotIndex);
		if (!SkillInstance) continue;

		int32 Cost = FMath::Max(0, BuffManager->GetModifiedEnergyCost(Action.Side, SkillInstance->GetInstanceEnergyCost()));
		Actor->CurrentEnergy = FMath::Max(0, Actor->CurrentEnergy - Cost);
		SkillInstance->OnSkillUsed();
		Actor->LastUsedSkillType = SkillInstance->GetSkillDataRef().SkillType;

		if (Action.Side == EInfoSide::Self)
		{
			BattleController->OnSelfCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
		}
		else
		{
			BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
		}

		float Modifier = 1.0f;
		if (Counter != ECounterState::None)
		{
			const FTurnAction& Other = ActionQueue[1 - i];
			if (IsCounteredBy(Action, Other))
			{
				Modifier = GetCounterModifier(Other);
			}
		}

		EInfoSide TargetSide = (Action.Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;

		if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Attack)
		{
			ApplyAttack(Action.Side, Action.SlotIndex, TargetSide, Modifier);
		}
		else if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Status)
		{
			ApplyStatusEffects(Action, SkillInstance);
		}

		EInfoSide DeathSide = (Action.Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;
		CheckDeath(DeathSide);
		if (CurrentPhase == ETurnPhase::BattleEnd) return;
	}

	if (BattleController)
	{
		BattleController->OnActionPhaseEnded.Broadcast();
	}

	ActionQueue.Empty();
	EndTurn();
}

UElfTurnManager::ECounterState UElfTurnManager::DetermineCounter(const FTurnAction& A, const FTurnAction& B) const
{
	if (IsCounteredBy(A, B)) return ECounterState::SecondCountersFirst;
	if (IsCounteredBy(B, A)) return ECounterState::FirstCountersSecond;
	return ECounterState::None;
}

bool UElfTurnManager::IsCounteredBy(const FTurnAction& Target, const FTurnAction& Counter) const
{
	UElfSkillBase* TargetSkill = GetActiveSkillInstance(Target.Side, Target.SlotIndex);
	UElfSkillBase* CounterSkill = GetActiveSkillInstance(Counter.Side, Counter.SlotIndex);
	if (!TargetSkill || !CounterSkill) return false;
	if (!CounterSkill->GetSkillDataRef().Counter) return false;

	ESkillType TargetType = TargetSkill->GetSkillDataRef().SkillType;
	ESkillType CounterType = CounterSkill->GetSkillDataRef().SkillType;

	switch (CounterType)
	{
	case ESkillType::Defense: return TargetType == ESkillType::Attack;
	case ESkillType::Attack:  return TargetType == ESkillType::Status;
	case ESkillType::Status:  return TargetType == ESkillType::Defense;
	default: return false;
	}
}

float UElfTurnManager::GetCounterModifier(const FTurnAction& Counter) const
{
	UElfSkillBase* Skill = GetActiveSkillInstance(Counter.Side, Counter.SlotIndex);
	if (!Skill || !Skill->GetSkillDataRef().Counter) return 1.0f;

	ESkillType Type = Skill->GetSkillDataRef().SkillType;

	if (Type == ESkillType::Defense)
	{
		float Reduction = 0.3f;
		for (const FSkillEffect& Effect : Skill->GetSkillDataRef().CounterEffects)
		{
			if (Effect.Type == EEffectType::Power)
			{
				Reduction = Effect.Value / 100.0f;
				break;
			}
		}
		return 1.0f - FMath::Clamp(Reduction, 0.0f, 1.0f);
	}

	if (Type == ESkillType::Attack)
	{
		float Bonus = 0.5f;
		for (const FSkillEffect& Effect : Skill->GetSkillDataRef().CounterEffects)
		{
			if (Effect.Type == EEffectType::Power)
			{
				Bonus = Effect.Value / 100.0f;
				break;
			}
		}
		return 1.0f + Bonus;
	}

	return 1.0f;
}

void UElfTurnManager::ExecuteSingleAction(const FTurnAction& Action, float DamageModifier)
{
	ApplyAttack(Action.Side, Action.SlotIndex,
		(Action.Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self,
		DamageModifier);
}

void UElfTurnManager::ApplyAttack(EInfoSide AttackerSide, int32 SlotIndex, EInfoSide TargetSide, float DamageModifier)
{
	FElfCreatureInstance* Attacker = GetActiveCreature(AttackerSide);
	FElfCalculatedStats* AttackerStats = GetActiveStats(AttackerSide);
	FElfCreatureInstance* Target = GetActiveCreature(TargetSide);
	FElfCalculatedStats* TargetStats = GetActiveStats(TargetSide);
	if (!Attacker || !Target || !AttackerStats || !TargetStats) return;

	UElfSkillBase* SkillInstance = GetActiveSkillInstance(AttackerSide, SlotIndex);
	UAttackSkillBase* AttackSkill = SkillInstance ? Cast<UAttackSkillBase>(SkillInstance) : nullptr;
	if (!AttackSkill) return;

	FElfCalculatedStats ModifiedAttacker = *AttackerStats;
	FElfCalculatedStats ModifiedTarget = *TargetStats;
	BuffManager->GetModifiedStats(AttackerSide, ModifiedAttacker);
	BuffManager->GetModifiedStats(TargetSide, ModifiedTarget);

	int32 Damage = AttackSkill->CalculateInstanceDamage(ModifiedAttacker, ModifiedTarget);
	if (Damage <= 0) return;

	float PowerMod = 1.0f;
	TArray<const FActiveBuff*> AttackBuffs;
	BuffManager->CollectActiveBuffs(AttackerSide, AttackBuffs);
	for (const FActiveBuff* Buff : AttackBuffs)
	{
		if (Buff->EffectID == EEffectID::ModifyEnergyCostAndPower && Buff->Params.IsValidIndex(1))
		{
			PowerMod += Buff->Params[1] * Buff->StackCount;
		}
	}
	Damage = FMath::Max(1, FMath::RoundToInt(Damage * PowerMod));

	UElfGameInstance* GI = GetGameInstance();
	if (GI)
	{
		FElfBaseData TargetBaseData;
		if (GI->GetElfBaseData(Target->CreatureRowName, TargetBaseData))
		{
			float TypeMult = ElfTypeChart::GetMultiplier(
				SkillInstance->GetSkillDataRef().ElementType,
				TargetBaseData.Type1,
				TargetBaseData.Type2,
				GI->TypeChartTable);
			Damage = FMath::Max(1, FMath::RoundToInt(Damage * TypeMult));
		}
	}

	Damage = FMath::Max(1, FMath::RoundToInt(Damage * DamageModifier));

	Target->CurrentHP = FMath::Max(0, Target->CurrentHP - Damage);

	if (TargetSide == EInfoSide::Self)
	{
		BattleController->OnSelfCreatureHPChanged.Broadcast(Target->CurrentHP, TargetStats->MaxHP);
	}
	else
	{
		BattleController->OnEnemyCreatureHPChanged.Broadcast(Target->CurrentHP, TargetStats->MaxHP);
	}
}

void UElfTurnManager::ApplyStatusEffects(const FTurnAction& Action, UElfSkillBase* SkillInstance)
{
	FElfCreatureInstance* Actor = GetActiveCreature(Action.Side);
	if (!Actor) return;

	EInfoSide TargetSide = (Action.Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;

	for (const FSkillEffect& Effect : SkillInstance->GetSkillDataRef().Effects)
	{
		switch (Effect.Type)
		{
		case EEffectType::HealHPPercent:
		{
			FElfCalculatedStats* ActorStats = GetActiveStats(Action.Side);
			if (ActorStats)
			{
				int32 Heal = FMath::RoundToInt(ActorStats->MaxHP * Effect.Value / 100.0f);
				Actor->CurrentHP = FMath::Min(ActorStats->MaxHP, Actor->CurrentHP + Heal);
				if (Action.Side == EInfoSide::Self)
					BattleController->OnSelfCreatureHPChanged.Broadcast(Actor->CurrentHP, ActorStats->MaxHP);
				else
					BattleController->OnEnemyCreatureHPChanged.Broadcast(Actor->CurrentHP, ActorStats->MaxHP);
			}
			break;
		}
		case EEffectType::RestoreEnergy:
		{
			int32 Restore = FMath::RoundToInt(Effect.Value);
			Actor->CurrentEnergy = FMath::Min(10, Actor->CurrentEnergy + Restore);
			if (Action.Side == EInfoSide::Self)
				BattleController->OnSelfCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
			else
				BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
			break;
		}
		case EEffectType::AddBuff:
		case EEffectType::AddDebuff:
		{
			if (!Effect.BuffRowName.IsNone())
			{
				const FEffectDef* Def = BuffManager->GetBuffDefCached(Effect.BuffRowName);
				if (Def)
				{
					EInfoSide BuffTarget = (Effect.EffectTarget == EEffectTarget::Caster) ? Action.Side : TargetSide;
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

void UElfTurnManager::EndTurn()
{
	BuffManager->ProcessTurnEndEffects(EInfoSide::Self);
	CheckDeath(EInfoSide::Self);
	BuffManager->ProcessTurnEndEffects(EInfoSide::Enemy);
	CheckDeath(EInfoSide::Enemy);

	BuffManager->TickBuffs(EInfoSide::Self);
	BuffManager->TickBuffs(EInfoSide::Enemy);

	ChangePhase(ETurnPhase::None);
	StartTurn();
}

void UElfTurnManager::CheckDeath(EInfoSide Side)
{
	FElfCreatureInstance* Creature = GetActiveCreature(Side);
	if (!Creature || Creature->CurrentHP > 0) return;

	if (Side == EInfoSide::Self) PlayerFaintCount++;
	else EnemyFaintCount++;

	int32 MaxFaints = 3;

	if (PlayerFaintCount >= MaxFaints)
	{
		EndBattle(EBattleResult::PlayerLose);
		return;
	}

	if (EnemyFaintCount >= MaxFaints)
	{
		EndBattle(EBattleResult::PlayerWin);
		return;
	}

	if (HasAliveCreatures(Side))
	{
		EnterSwitchPhase(Side);
	}
	else
	{
		EndBattle(Side == EInfoSide::Self ? EBattleResult::PlayerLose : EBattleResult::PlayerWin);
	}
}

void UElfTurnManager::EnterSwitchPhase(EInfoSide Side)
{
	ChangePhase(ETurnPhase::Switch);

	int32 NextAlive = -1;
	FBattleSideData* SideData = GetSide(Side);
	if (SideData)
	{
		for (int32 i = 0; i < SideData->Team.Num(); i++)
		{
			if (SideData->Team[i].CurrentHP > 0)
			{
				NextAlive = i;
				break;
			}
		}
	}

	if (NextAlive >= 0)
	{
		OnSwitchRequested.Broadcast(Side, NextAlive);
		StartTurn();
	}
	else
	{
		EndBattle(Side == EInfoSide::Self ? EBattleResult::PlayerLose : EBattleResult::PlayerWin);
	}
}

void UElfTurnManager::EndBattle(EBattleResult Result)
{
	ChangePhase(ETurnPhase::BattleEnd);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExecutionTimer);
	}

	OnBattleEnded.Broadcast(Result);
}

void UElfTurnManager::OnCreatureEnteredField(EInfoSide Side)
{
	if (BuffManager)
		BuffManager->OnCreatureEnteredField(Side);
}

void UElfTurnManager::ApplyBuffToTarget(EInfoSide TargetSide, FName BuffDefRowName, const FEffectDef& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
{
	if (BuffManager)
		BuffManager->ApplyBuffToTarget(TargetSide, BuffDefRowName, Def, OverrideStack, OverrideDuration, bIsBuff);
}

void UElfTurnManager::ApplyBuffToSide(EInfoSide Side, FName BuffDefRowName, const FEffectDef& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
{
	if (BuffManager)
		BuffManager->ApplyBuffToSide(Side, BuffDefRowName, Def, OverrideStack, OverrideDuration, bIsBuff);
}

FBattleSideData* UElfTurnManager::GetSide(EInfoSide Side)
{
	if (!BattleModel) return nullptr;
	return (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
}

FElfCreatureInstance* UElfTurnManager::GetActiveCreature(EInfoSide Side)
{
	FBattleSideData* SideData = GetSide(Side);
	return SideData ? SideData->GetActiveCreature() : nullptr;
}

FElfCalculatedStats* UElfTurnManager::GetActiveStats(EInfoSide Side)
{
	FBattleSideData* SideData = GetSide(Side);
	return SideData ? SideData->GetActiveStats() : nullptr;
}

UElfSkillBase* UElfTurnManager::GetActiveSkillInstance(EInfoSide Side, int32 SlotIndex) const
{
	if (!BattleModel) return nullptr;
	FBattleSideData* SideData = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
	return SideData ? SideData->GetActiveSkillInstance(SlotIndex) : nullptr;
}

UElfSkillBase* UElfTurnManager::GetActiveDefaultSkillInstance(EInfoSide Side, int32 SlotIndex) const
{
	if (!BattleModel) return nullptr;
	FBattleSideData* SideData = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
	return SideData ? SideData->GetActiveDefaultSkillInstance(SlotIndex) : nullptr;
}

bool UElfTurnManager::HasAliveCreatures(EInfoSide Side) const
{
	if (!BattleModel) return false;

	const FBattleSideData& SideData = (Side == EInfoSide::Self) ? BattleModel->PlayerSide : BattleModel->EnemySide;
	for (const FElfCreatureInstance& C : SideData.Team)
	{
		if (C.CurrentHP > 0) return true;
	}
	return false;
}

int32 UElfTurnManager::GetEffectiveSpeed(EInfoSide Side)
{
	if (!BattleModel) return 0;

	FBattleSideData& SideData = (Side == EInfoSide::Self) ? BattleModel->PlayerSide : BattleModel->EnemySide;
	if (SideData.CalculatedStats.IsValidIndex(SideData.ActiveIndex))
	{
		int32 BaseSpeed = SideData.CalculatedStats[SideData.ActiveIndex].SPD;
		return BuffManager ? BuffManager->GetModifiedSpeed(Side, BaseSpeed) : BaseSpeed;
	}
	return 0;
}

int32 UElfTurnManager::GetSkillPriorityFor(EInfoSide Side, int32 SlotIndex) const
{
	UElfSkillBase* Inst = GetActiveSkillInstance(Side, SlotIndex);
	return Inst ? Inst->GetSkillDataRef().Priority : -99;
}

UElfGameInstance* UElfTurnManager::GetGameInstance() const
{
	if (!BattleController) return nullptr;
	APlayerController* PC = BattleController->GetOwnerPC();
	return PC ? PC->GetGameInstance<UElfGameInstance>() : nullptr;
}
