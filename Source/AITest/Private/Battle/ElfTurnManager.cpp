#include "Battle/ElfTurnManager.h"
#include "Battle/ElfBattleAI.h"
#include "Battle/ElfBuffManager.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Skill/ElfSkillBase.h"
#include "Skill/Attack/AttackSkillBase.h"
#include "Data/ElfTypeChart.h"
#include "Data/ElfStatCalculator.h"
#include "Game/ElfGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Player/ElfPlayerState.h"

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
		BattleController->OnCaptureConfirmed.Clear();
		BattleController->OnCaptureConfirmed.AddDynamic(this, &UElfTurnManager::OnCaptureConfirmed);
		BattleController->OnSwitchSlotSelected.Clear();
		BattleController->OnSwitchSlotSelected.AddDynamic(this, &UElfTurnManager::OnPlayerSwitchRequest);
	}
}

void UElfTurnManager::StartTurn()
{
	if (BattleController)
	{
		BattleController->ResetBattleItemState();
		BattleController->SetInputMode(EBattleInputMode::Command);
	}

	PlayerChosenSlot = -1;
	EnemyChosenSlot = -1;
	PlayerDefaultSlotIndex = -1;
	EnemyDefaultSlotIndex = -1;
	bPlayerUsedDefault = false;
	bEnemyUsedDefault = false;
	bLocalActionChosen = false;
	bRemoteActionChosen = false;

	ChangePhase(ETurnPhase::PlayerDecision);
}

void UElfTurnManager::OnPlayerSkillSelected(int32 SlotIndex)
{
	if (CurrentPhase != ETurnPhase::PlayerDecision) return;

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
		ChangePhase(ETurnPhase::WaitingForOpponent);
		ChooseEnemyAction();
		OnPlayerActionReady();
	}
}

void UElfTurnManager::OnPlayerDefaultSkillSelected(int32 SlotIndex)
{
	if (CurrentPhase != ETurnPhase::PlayerDecision) return;

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
		ChangePhase(ETurnPhase::WaitingForOpponent);
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

void UElfTurnManager::OnCaptureConfirmed()
{
	if (CurrentPhase != ETurnPhase::PlayerDecision) return;
	ChooseEnemyAction();
	OnPlayerActionReady();
}

void UElfTurnManager::OnPlayerSwitchRequest(int32 SlotIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("OnPlayerSwitchRequest: SlotIndex=%d, CurrentPhase=%d, bInputModeLocked=%d"), 
		SlotIndex, (int32)CurrentPhase, BattleController ? BattleController->bInputModeLocked : -1);

	if (CurrentPhase != ETurnPhase::ManualSwitch && CurrentPhase != ETurnPhase::PlayerDecision) return;

	if (BuffManager->IsSwitchBlocked(EInfoSide::Self)) return;

	FElfCreatureInstance* Current = GetActiveCreature(EInfoSide::Self);
	if (Current)
	{
		Current->LastUsedSkillType = ESkillType::Attack;
	}

	ChangePhase(ETurnPhase::ManualSwitch);
	OnSwitchRequested.Broadcast(EInfoSide::Self, SlotIndex);

	PlayerChosenSlot = -1;
	bLocalActionChosen = true;

	if (BattleModel && BattleModel->BattleType == EBattleType::PvP)
	{
		ChangePhase(ETurnPhase::WaitingForOpponent);
	}
	else
	{
		ChangePhase(ETurnPhase::WaitingForOpponent);
		ChooseEnemyAction();
		OnPlayerActionReady();
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
	if (BattleController)
	{
		BattleController->SetCurrentTurnPhase(NewPhase);
	}
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

	bSwiftDone = false;

	ChangePhase(ETurnPhase::SkillExecution);

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
	if (bActionSetupDone)
	{
		ProcessNextAction();
		return;
	}

	if (ActionQueue.Num() < 2)
	{
		EndTurn();
		return;
	}

	FTurnAction A = ActionQueue[0];
	FTurnAction B = ActionQueue[1];
	ECounterState Counter = DetermineCounter(A, B);
	if (Counter == ECounterState::FirstCountersSecond)
	{
		Swap(ActionQueue[0], ActionQueue[1]);
	}

	A = ActionQueue[0];
	B = ActionQueue[1];
	if (Counter != ECounterState::None)
	{
		DisplayActions = { A, B };
		ExecuteActions = { B, A };
	}
	else
	{
		DisplayActions = ActionQueue;
		ExecuteActions = ActionQueue;
	}

	if (BattleController)
	{
		BattleController->ConsumePendingItem();
	}

	if (BattleController && BattleController->IsCapturePending())
	{
		ProcessCapture();
		if (CurrentPhase == ETurnPhase::BattleEnd) return;
	}

	if (!bSwiftDone)
	{
		TryExecuteSwiftSkills();
		return;
	}

	ProcessPendingEvolutions();

	if (bCaptureAttempted)
	{
		bCaptureAttempted = false;
		ActionQueue.RemoveAll([](const FTurnAction& A) { return A.Side == EInfoSide::Self; });
		DisplayActions = ActionQueue;
		ExecuteActions = ActionQueue;
	}

	bActionSetupDone = true;
	BeginActionPipeline();
}

void UElfTurnManager::BeginActionPipeline()
{
	CurrentActionIndex = 0;
	bInDisplayPhase = true;
	bInExecutePhase = false;
	ProcessNextAction();
}

void UElfTurnManager::ProcessNextAction()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (bInDisplayPhase)
	{
		if (CurrentActionIndex < DisplayActions.Num())
		{
			const FTurnAction& Action = DisplayActions[CurrentActionIndex];
			FName SkillRowName = GetActionSkillRowName(Action);
			bool bIsCounter = false;
			if (DisplayActions.Num() > 1 && ExecuteActions.Num() > 1 && ExecuteActions[0].Side != DisplayActions[0].Side)
			{
				bIsCounter = (CurrentActionIndex == 1);
			}
			if (BattleController)
				BattleController->OnSkillDisplayStarted.Broadcast(Action.Side, SkillRowName, bIsCounter);

			CurrentActionIndex++;
			World->GetTimerManager().SetTimer(ExecutionTimer, this, &UElfTurnManager::ProcessNextAction, 1.5f, false);
		}
		else
		{
			if (BattleController)
				BattleController->OnAllSkillsDisplayed.Broadcast();

			bInDisplayPhase = false;
			CurrentActionIndex = 0;
			bInExecutePhase = true;
			World->GetTimerManager().SetTimer(ExecutionTimer, this, &UElfTurnManager::ProcessNextAction, 0.5f, false);
		}
		return;
	}

	if (bInExecutePhase)
	{
		if (CurrentActionIndex < ExecuteActions.Num())
		{
			const FTurnAction& Action = ExecuteActions[CurrentActionIndex];
			CurrentActionIndex++;
			ExecuteTurnAction(Action);
			if (CurrentPhase == ETurnPhase::BattleEnd) return;
			if (!bForceSwitchPending)
			{
				World->GetTimerManager().SetTimer(ExecutionTimer, this, &UElfTurnManager::ProcessNextAction, 1.0f, false);
			}
		}
		else
		{
			bInExecutePhase = false;
			if (BattleController)
				BattleController->OnActionPhaseEnded.Broadcast();
			ActionQueue.Empty();
			EndTurn();
		}
		return;
	}
}

FName UElfTurnManager::GetActionSkillRowName(const FTurnAction& Action)
{
	if (Action.bIsDefault)
	{
		UElfGameInstance* GI = GetGameInstance();
		return GI && GI->DefaultSkillIDs.IsValidIndex(Action.SlotIndex) ? GI->DefaultSkillIDs[Action.SlotIndex] : NAME_None;
	}
	FElfCreatureInstance* Creature = GetActiveCreature(Action.Side);
	return Creature && Creature->EquippedSkills.IsValidIndex(Action.SlotIndex) ? Creature->EquippedSkills[Action.SlotIndex] : NAME_None;
}

void UElfTurnManager::ExecuteTurnAction(const FTurnAction& Action)
{
	FElfCreatureInstance* Actor = GetActiveCreature(Action.Side);
	if (!Actor || Actor->CurrentHP <= 0 || Action.SlotIndex < 0) return;
	if (!Action.bIsDefault && !Actor->EquippedSkills.IsValidIndex(Action.SlotIndex)) return;

	UElfSkillBase* SkillInstance = Action.bIsDefault
		? GetActiveDefaultSkillInstance(Action.Side, Action.SlotIndex)
		: GetActiveSkillInstance(Action.Side, Action.SlotIndex);
	if (!SkillInstance) return;

	int32 Cost = FMath::Max(0, BuffManager->GetModifiedEnergyCost(Action.Side, SkillInstance->GetInstanceEnergyCost()));
	Actor->CurrentEnergy = FMath::Max(0, Actor->CurrentEnergy - Cost);
	SkillInstance->OnSkillUsed();
	Actor->LastUsedSkillType = SkillInstance->GetSkillDataRef().SkillType;

	if (Action.Side == EInfoSide::Self)
		BattleController->OnSelfCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
	else
		BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);

	float Modifier = 1.0f;
	for (int32 i = 0; i < ExecuteActions.Num(); i++)
	{
		if (ExecuteActions[i].Side == Action.Side && ExecuteActions[i].SlotIndex == Action.SlotIndex)
		{
			if (ExecuteActions.Num() > 1)
			{
				const FTurnAction& Other = ExecuteActions[1 - i];
				if (IsCounteredBy(Action, Other))
					Modifier = GetCounterModifier(Other);
			}
			break;
		}
	}

	EInfoSide TargetSide = (Action.Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;

	if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Attack)
		ApplyAttack(Action.Side, Action.SlotIndex, TargetSide, Modifier);
	else if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Status)
		ApplyStatusEffects(Action, SkillInstance);

	EInfoSide DeathSide = (Action.Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;
	CheckDeath(DeathSide);
	if (CurrentPhase == ETurnPhase::BattleEnd) return;

	if (Action.Side == EInfoSide::Self && Actor->bWishActive)
		BattleController->CancelWish();

	ForceSwitchSideCount = 0;
	ForceSwitchCompleted = 0;
	CheckSkillForcedSwitch(Action, SkillInstance);
	if (bForceSwitchPending)
	{
		ProcessForcedSwitches();
	}
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
				const FEffectData* Def = BuffManager->GetBuffDataCached(Effect.BuffRowName);
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

void UElfTurnManager::ProcessPendingEvolutions()
{
	// 检查是否有待进化
	auto HasPending = [&]() -> bool
	{
		for (EInfoSide S : { EInfoSide::Self, EInfoSide::Enemy })
		{
			FBattleSideData* Side = GetSide(S);
			if (!Side) continue;
			for (const FElfCreatureInstance& C : Side->Team)
				if (C.bPendingEvolution) return true;
		}
		return false;
	};

	if (HasPending())
	{
		ChangePhase(ETurnPhase::EvolutionPhase);
		// 进化完成后切回执行阶段
		ChangePhase(ETurnPhase::SkillExecution);
	}

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// 收集双方待进化精灵
	struct FEvolutionEntry
	{
		EInfoSide Side;
		int32 CreatureIndex;
		int32 Speed;
	};
	TArray<FEvolutionEntry> Pending;
	FBattleSideData* Sides[2] = { GetSide(EInfoSide::Self), GetSide(EInfoSide::Enemy) };
	EInfoSide SideLabels[2] = { EInfoSide::Self, EInfoSide::Enemy };

	for (int32 s = 0; s < 2; s++)
	{
		if (!Sides[s]) continue;
		for (int32 i = 0; i < Sides[s]->Team.Num(); i++)
		{
			FElfCreatureInstance& Creature = Sides[s]->Team[i];
			if (Creature.bPendingEvolution)
			{
				int32 Speed = Sides[s]->CalculatedStats.IsValidIndex(i) ? Sides[s]->CalculatedStats[i].SPD : 0;
				Pending.Add({ SideLabels[s], i, Speed });
			}
		}
	}

	// 按速度排序（快优先）
	Pending.Sort([](const FEvolutionEntry& A, const FEvolutionEntry& B) { return A.Speed > B.Speed; });

	// 执行进化
	for (const FEvolutionEntry& Entry : Pending)
	{
		FBattleSideData* SideData = Sides[Entry.Side == EInfoSide::Self ? 0 : 1];
		FElfCreatureInstance& Creature = SideData->Team[Entry.CreatureIndex];
		FElfBaseData BaseData;
		if (GI->GetElfBaseData(Creature.CreatureRowName, BaseData) && !BaseData.EvolutionTarget.RowName.IsNone())
		{
			Creature.CreatureRowName = BaseData.EvolutionTarget.RowName;
			Creature.bPendingEvolution = false;

			FElfBaseData EvolvedBase;
			if (GI->GetElfBaseData(Creature.CreatureRowName, EvolvedBase) && SideData->CalculatedStats.IsValidIndex(Entry.CreatureIndex))
			{
				FElfCalculatedStats& OldStats = SideData->CalculatedStats[Entry.CreatureIndex];
				float HPPct = OldStats.MaxHP > 0 ? static_cast<float>(Creature.CurrentHP) / OldStats.MaxHP : 1.0f;
				OldStats = UElfStatCalculator::CalculateStats(EvolvedBase);
				Creature.CurrentHP = FMath::Max(1, FMath::RoundToInt(OldStats.MaxHP * HPPct));
			}
		}
	}
}

void UElfTurnManager::CheckSkillForcedSwitch(const FTurnAction& Action, UElfSkillBase* SkillInstance)
{
	bForceSwitchPending = false;
	ForceSwitchSideCount = 0;

	EInfoSide SelfSide = Action.Side;
	EInfoSide EnemySide = (SelfSide == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;

	auto CheckOne = [&](const TArray<FSkillEffect>& EffectList)
	{
		for (const FSkillEffect& Effect : EffectList)
		{
			if (Effect.Type == EEffectType::ForceSwitchSelf)
			{
				if (HasAliveBackup(SelfSide)) ForceSwitchSides[ForceSwitchSideCount++] = SelfSide;
			}
			else if (Effect.Type == EEffectType::ForceSwitchEnemy)
			{
				if (HasAliveBackup(EnemySide)) ForceSwitchSides[ForceSwitchSideCount++] = EnemySide;
			}
			else if (Effect.Type == EEffectType::ForceSwitchBoth)
			{
				if (HasAliveBackup(SelfSide)) ForceSwitchSides[ForceSwitchSideCount++] = SelfSide;
				if (HasAliveBackup(EnemySide)) ForceSwitchSides[ForceSwitchSideCount++] = EnemySide;
			}
		}
	};

	CheckOne(SkillInstance->GetSkillDataRef().Effects);
	if (SkillInstance->GetSkillDataRef().Counter)
		CheckOne(SkillInstance->GetSkillDataRef().CounterEffects);

	bForceSwitchPending = (ForceSwitchSideCount > 0);
}

void UElfTurnManager::ProcessForcedSwitches()
{
	ChangePhase(ETurnPhase::ForcedSwitch);
	if (BattleController) BattleController->SetInputModeLocked(true);

	for (int32 i = 0; i < ForceSwitchSideCount; i++)
	{
		EInfoSide Side = ForceSwitchSides[i];

		if (Side == EInfoSide::Self)
		{
			// 玩家侧：广播等待选择
			FBattleSideData* SideData = GetSide(Side);
			if (!SideData) { ForceSwitchCompleted++; continue; }

			int32 NextAlive = -1;
			for (int32 j = 0; j < SideData->Team.Num(); j++)
			{
				if (SideData->Team[j].CurrentHP > 0 && j != SideData->ActiveIndex)
				{
					NextAlive = j;
					break;
				}
			}
			if (NextAlive >= 0)
				OnForcedSwitchRequested.Broadcast(Side, NextAlive);
			else
				ForceSwitchCompleted++;
		}
		else
		{
			// AI/敌方：随机选
			int32 NextAlive = PickRandomAliveCreature(Side);
			if (NextAlive >= 0)
			{
				OnForcedSwitchRequested.Broadcast(Side, NextAlive);
			}
			ForceSwitchCompleted++;
		}
	}

	// 如果不需要等待（全AI侧），直接恢复
	if (ForceSwitchCompleted >= ForceSwitchSideCount)
		ResumeAfterForcedSwitch();
}

void UElfTurnManager::OnForcedSwitchComplete()
{
	ForceSwitchCompleted++;
	if (ForceSwitchCompleted >= ForceSwitchSideCount)
	{
		ResumeAfterForcedSwitch();
	}
}

void UElfTurnManager::ResumeAfterForcedSwitch()
{
	bForceSwitchPending = false;
	if (BattleController) BattleController->SetInputModeLocked(false);

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(ExecutionTimer,
			FTimerDelegate::CreateUObject(this, &UElfTurnManager::ProcessNextAction),
			0.5f, false);
	}
}

bool UElfTurnManager::HasAliveBackup(EInfoSide Side) const
{
	FBattleSideData* SideData = (Side == EInfoSide::Self)
		? (BattleModel ? &BattleModel->PlayerSide : nullptr)
		: (BattleModel ? &BattleModel->EnemySide : nullptr);
	if (!SideData) return false;

	for (int32 i = 0; i < SideData->Team.Num(); i++)
	{
		if (i == SideData->ActiveIndex) continue;
		if (SideData->Team[i].CurrentHP > 0) return true;
	}
	return false;
}

int32 UElfTurnManager::PickRandomAliveCreature(EInfoSide Side) const
{
	FBattleSideData* SideData = (Side == EInfoSide::Self)
		? (BattleModel ? &BattleModel->PlayerSide : nullptr)
		: (BattleModel ? &BattleModel->EnemySide : nullptr);
	if (!SideData) return -1;

	TArray<int32> Alive;
	for (int32 i = 0; i < SideData->Team.Num(); i++)
	{
		if (i == SideData->ActiveIndex) continue;
		if (SideData->Team[i].CurrentHP > 0) Alive.Add(i);
	}
	return Alive.IsEmpty() ? -1 : Alive[FMath::RandRange(0, Alive.Num() - 1)];
}

// ==================== 捕捉 ====================

void UElfTurnManager::ProcessCapture()
{
	if (!BattleController || !BattleController->IsCapturePending()) return;

	ChangePhase(ETurnPhase::CapturePhase);

	FElfCreatureInstance* Target = GetActiveCreature(EInfoSide::Enemy);
	if (!Target) { BattleController->ClearCapturePending(); return; }

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) { BattleController->ClearCapturePending(); return; }

	FElfBaseData BaseData;
	if (!GI->GetElfBaseData(Target->CreatureRowName, BaseData)) { BattleController->ClearCapturePending(); return; }

	float Difficulty = FMath::Max(0.1f, BaseData.CaptureDifficulty);
	float BallRate = FMath::Max(0.1f, BattleController->GetCaptureBallRate());
	float Chance = BallRate / Difficulty;

	bool bSuccess = FMath::FRandRange(0.0f, 1.0f) <= Chance;

	BattleController->ClearCapturePending();
	bCaptureAttempted = true;

	if (bSuccess)
	{
		// 捕捉成功 → 精灵放入背包
		AElfPlayerState* PS = BattleController->GetOwnerPC() ? BattleController->GetOwnerPC()->GetPlayerState<AElfPlayerState>() : nullptr;
		if (PS)
		{
			TArray<FElfCreatureInstance>& Team = PS->GetTeamCreatures();
			if (Team.Num() < 6)
				Team.Add(*Target);
			else
				PS->GetWarehouseCreatures().Add(*Target);
		}

		// 敌方精灵移除
		FBattleSideData* EnemySide = GetSide(EInfoSide::Enemy);
		if (EnemySide)
		{
			Target->CurrentHP = 0;
			EnemySide->MoveActiveToEnd();
		}

		CheckDeath(EInfoSide::Enemy);
		if (CurrentPhase != ETurnPhase::BattleEnd)
			EndBattle(EBattleResult::PlayerWin);
	}

	// 失败：玩家行动被跳过
}

// ==================== 迅捷 ====================

struct FQueuedSwiftSkill
{
	EInfoSide Side;
	int32 SlotIndex;
	int32 Speed;
};

void UElfTurnManager::TryExecuteSwiftSkills()
{
	if (bSwiftDone) return;

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) { bSwiftDone = true; return; }

	TArray<FQueuedSwiftSkill> Queue;
	FBattleSideData* Sides[2] = { GetSide(EInfoSide::Self), GetSide(EInfoSide::Enemy) };

	for (int32 s = 0; s < 2; s++)
	{
		if (!Sides[s]) continue;
		FElfCreatureInstance* Creature = Sides[s]->GetActiveCreature();
		if (!Creature) continue;

		int32 Speed = Sides[s]->CalculatedStats.IsValidIndex(Sides[s]->ActiveIndex)
			? Sides[s]->CalculatedStats[Sides[s]->ActiveIndex].SPD : 0;

		for (int32 j = 0; j < Creature->EquippedSkills.Num(); j++)
		{
			FName SkillRowName = Creature->EquippedSkills[j];
			if (SkillRowName.IsNone()) continue;

			FSkillData SkillData;
			if (!GI->GetSkillData(SkillRowName, SkillData)) continue;

			for (const FSkillEffect& Effect : SkillData.Effects)
			{
				if (Effect.Type != EEffectType::Swift) continue;

				// 检查能量是否足够
				EInfoSide SideEnum = (s == 0) ? EInfoSide::Self : EInfoSide::Enemy;
				int32 Cost = SkillData.EnergyCost;
				if (BuffManager) Cost = BuffManager->GetModifiedEnergyCost(SideEnum, Cost);
				if (Creature->CurrentEnergy >= Cost)
				{
					// 记录 EffectTarget，但同一个技能只有一个 Swift 效果
					Queue.Add({ SideEnum, j, Speed });
				}
				break; // 一个技能只有一个 Swift 效果
			}
		}
	}

	if (Queue.IsEmpty())
	{
		bSwiftDone = true;
		OnSwiftSkillDone();
		return;
	}

	// 按速度排序
	Queue.Sort([](const FQueuedSwiftSkill& A, const FQueuedSwiftSkill& B) { return A.Speed > B.Speed; });

	// 链式执行
	for (int32 i = 0; i < Queue.Num(); i++)
	{
		ExecuteSwiftSkill(Queue[i].Side, Queue[i].SlotIndex);
	}

	bSwiftDone = true;
	OnSwiftSkillDone();
}

void UElfTurnManager::ExecuteSwiftSkill(EInfoSide Side, int32 SkillSlotIndex)
{
	FElfCreatureInstance* Actor = GetActiveCreature(Side);
	if (!Actor || !Actor->EquippedSkills.IsValidIndex(SkillSlotIndex)) return;

	UElfSkillBase* SkillInstance = GetActiveSkillInstance(Side, SkillSlotIndex);
	if (!SkillInstance) return;

	// 扣能量
	int32 Cost = SkillInstance->GetInstanceEnergyCost();
	if (BuffManager) Cost = BuffManager->GetModifiedEnergyCost(Side, Cost);
	Actor->CurrentEnergy = FMath::Max(0, Actor->CurrentEnergy - Cost);
	SkillInstance->OnSkillUsed();
	Actor->LastUsedSkillType = SkillInstance->GetSkillDataRef().SkillType;

	if (Side == EInfoSide::Self)
		BattleController->OnSelfCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);
	else
		BattleController->OnEnemyCreatureEnergyChanged.Broadcast(Actor->CurrentEnergy);

	// 按技能类型执行
	EInfoSide TargetSide = (Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;

	// 查找 Swift 效果的 EffectTarget
	EInfoSide SkillTarget = TargetSide; // 默认目标
	for (const FSkillEffect& Effect : SkillInstance->GetSkillDataRef().Effects)
	{
		if (Effect.Type == EEffectType::Swift)
		{
			SkillTarget = (Effect.EffectTarget == EEffectTarget::Caster) ? Side : TargetSide;
			break;
		}
	}

	if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Attack)
	{
		// 攻击迅捷：直接对目标造成伤害
		ApplyAttack(Side, SkillSlotIndex, SkillTarget, 1.0f);
	}
	else if (SkillInstance->GetSkillDataRef().SkillType == ESkillType::Status)
	{
		FTurnAction SwiftAction;
		SwiftAction.Side = Side;
		SwiftAction.SlotIndex = SkillSlotIndex;
		ApplyStatusEffects(SwiftAction, SkillInstance);
	}

	// 死亡判定
	CheckDeath(TargetSide);
}

void UElfTurnManager::OnSwiftSkillDone()
{
	bSwiftDone = true;
	// 重新进入 ExecutionTimer 继续主流程
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(ExecutionTimer,
			FTimerDelegate::CreateUObject(this, &UElfTurnManager::OnExecutionTimer),
			0.1f, false);
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
	ChangePhase(ETurnPhase::ManualSwitch);

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

void UElfTurnManager::ApplyBuffToTarget(EInfoSide TargetSide, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
{
	if (BuffManager)
		BuffManager->ApplyBuffToTarget(TargetSide, BuffDefRowName, Def, OverrideStack, OverrideDuration, bIsBuff);
}

void UElfTurnManager::ApplyBuffToSide(EInfoSide Side, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack, int32 OverrideDuration, bool bIsBuff)
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
