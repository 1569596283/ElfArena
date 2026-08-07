#include "Battle/ElfTurnManager.h"
#include "Battle/ElfBattleAI.h"
#include "Battle/ElfBuffManager.h"
#include "Ability/ElfAbilityBase.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Skill/ElfSkillBase.h"
#include "Skill/Attack/AttackSkillBase.h"
#include "Data/ElfTypeChart.h"
#include "Data/ElfStatCalculator.h"
#include "Game/ElfGameInstance.h"
#include "Event/ElfEventManager.h"
#include "ElfGameplayTags.h"
#include "Elf/ElfManager.h"
#include "GameFramework/PlayerController.h"
#include "Player/ElfPlayerState.h"

UElfEventManager* UElfTurnManager::GetEventManager() const
{
	UElfGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UElfEventManager>() : nullptr;
}

void UElfTurnManager::Init(UElfBattleController* InBC, UElfBattleModel* InBM)
{
	BattleController = InBC;
	BattleModel = InBM;

	BuffManager = NewObject<UElfBuffManager>(this);
	BuffManager->Init(InBC, InBM);

	BattleAI = NewObject<UElfBattleAI>(this);

	InitCaptureItemQuantities();

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
		BattleController->OnRunRequested.Clear();
		BattleController->OnRunRequested.AddDynamic(this, &UElfTurnManager::OnPlayerRunRequest);
	}
}

void UElfTurnManager::StartTurn()
{
	if (BattleController)
	{
		ResetBattleItemState();
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

	// 特性触发：回合开始
	if (UElfEventManager* EventManager = GetEventManager())
	{
		const FElfGameplayTags& Tags = FElfGameplayTags::Get();
		EventManager->BroadcastEvent(Tags.Battle_Trigger_TurnStart, GetActiveCreature(EInfoSide::Self));
		EventManager->BroadcastEvent(Tags.Battle_Trigger_TurnStart, GetActiveCreature(EInfoSide::Enemy));
	}
}

void UElfTurnManager::OnPlayerSkillSelected(int32 SlotIndex)
{
	if (CurrentPhase != ETurnPhase::PlayerDecision) return;

	FElfCreatureInstance* Creature = GetActiveCreature(EInfoSide::Self);
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return;

	UElfSkillBase* SkillInstance = GetActiveSkillInstance(EInfoSide::Self, SlotIndex);
	if (!SkillInstance) return;
	if (GetSkillEnergyCost(EInfoSide::Self, SkillInstance) > Creature->CurrentEnergy) return;
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
	if (GetSkillEnergyCost(EInfoSide::Self, SkillInstance) > Creature->CurrentEnergy) return;
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

void UElfTurnManager::OnPlayerRunRequest()
{
	UE_LOG(LogTemp, Warning, TEXT("OnPlayerRunRequest: CurrentPhase=%d"), (int32)CurrentPhase);

	if (CurrentPhase != ETurnPhase::PlayerDecision) return;

	// 野生对战逃跑无惩罚，训练家/玩家对战逃跑视为战败
	if (BattleModel && BattleModel->BattleType == EBattleType::Wild)
		EndBattle(EBattleResult::Run);
	else
		EndBattle(EBattleResult::PlayerLose);
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

	// 优先级按行动取技能实例（默认技能用默认实例），避免默认技能读错装备槽位
	int32 PlayerPriority = -99;
	if (UElfSkillBase* Inst = GetActionSkillInstance(PlayerAction))
		PlayerPriority = Inst->GetSkillDataRef().Priority;

	int32 EnemyPriority = -99;
	if (UElfSkillBase* Inst = GetActionSkillInstance(EnemyAction))
		EnemyPriority = Inst->GetSkillDataRef().Priority;

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

		if (PlayerSpeed == EnemySpeed)
		{
			// 速度相同 → 随机决定先后
			if (FMath::FRand() < 0.5f)
				ActionQueue = { PlayerAction, EnemyAction };
			else
				ActionQueue = { EnemyAction, PlayerAction };
		}
		else if (PlayerSpeed > EnemySpeed)
		{
			ActionQueue = { PlayerAction, EnemyAction };
		}
		else
		{
			ActionQueue = { EnemyAction, PlayerAction };
		}
	}

	bSwiftDone = false;
	bActionSetupDone = false;

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
			1.0f, false);
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
		// 应对：提示顺序 被应对→应对（A,B），生效顺序 应对→被应对（B,A）
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
		ConsumePendingItem();
	}

	if (BattleController && IsCapturePending())
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

	// 逐行动作：提示顺序用 DisplayActions（应对时先弹被应对），生效顺序用 ExecuteActions（应对先生效）
	if (bInDisplayPhase)
	{
		if (CurrentActionIndex < DisplayActions.Num())
		{
			const FTurnAction& Action = DisplayActions[CurrentActionIndex];
			FName SkillRowName = GetActionSkillRowName(Action);
			bool bIsCounter = false;
			if (DisplayActions.Num() > 1)
			{
				const FTurnAction& Other = DisplayActions[1 - CurrentActionIndex];
				bIsCounter = IsCounteredBy(Other, Action);
			}
			if (BattleController)
				BattleController->OnSkillDisplayStarted.Broadcast(Action.Side, SkillRowName, bIsCounter);

			// 先手攻击特性：基于首个生效行动（应对时是应对方）
			if (CurrentActionIndex == 0 && ExecuteActions.Num() > 0)
			{
				const FTurnAction& FirstExecute = ExecuteActions[0];
				UElfSkillBase* FirstSkill = FirstExecute.bIsDefault
					? GetActiveDefaultSkillInstance(FirstExecute.Side, FirstExecute.SlotIndex)
					: GetActiveSkillInstance(FirstExecute.Side, FirstExecute.SlotIndex);
				if (FirstSkill && FirstSkill->GetSkillDataRef().SkillType == ESkillType::Attack)
				{
					if (UElfEventManager* EventManager = GetEventManager())
					{
						EventManager->BroadcastEvent(FElfGameplayTags::Get().Battle_Trigger_FirstAttack, GetActiveCreature(FirstExecute.Side));
					}
				}
			}

			bInDisplayPhase = false;
			bInExecutePhase = true;
			World->GetTimerManager().SetTimer(ExecutionTimer, this, &UElfTurnManager::ProcessNextAction, 1.5f, false);
		}
		else
		{
			if (BattleController)
			{
				BattleController->OnAllSkillsDisplayed.Broadcast();
				BattleController->OnActionPhaseEnded.Broadcast();
			}
			ActionQueue.Empty();
			EndTurn();
		}
		return;
	}

	if (bInExecutePhase)
	{
		const FTurnAction& Action = ExecuteActions[CurrentActionIndex];
		CurrentActionIndex++;
		ExecuteTurnAction(Action);
		if (CurrentPhase == ETurnPhase::BattleEnd) return;

		if (bForceSwitchPending)
		{
			// 强制换将：暂停，换将完成后回到显示阶段展示下一个技能
			bInExecutePhase = false;
			bInDisplayPhase = true;
			return;
		}

		bInExecutePhase = false;
		bInDisplayPhase = true;
		World->GetTimerManager().SetTimer(ExecutionTimer, this, &UElfTurnManager::ProcessNextAction, 1.0f, false);
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

	int32 Cost = GetSkillEnergyCost(Action.Side, SkillInstance);
	Actor->CurrentEnergy = FMath::Max(0, Actor->CurrentEnergy - Cost);
	SkillInstance->OnSkillUsed();
	Actor->LastUsedSkillType = SkillInstance->GetSkillDataRef().SkillType;

	// 记录技能属性（供 UseElementSkill 特性匹配）
	Actor->LastSkillElement = SkillInstance->GetSkillDataRef().ElementType;

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

	// 使用元素技能特性：技能结算完成后触发（提示晚于技能生效，如 回春/烈焰双攻/水脉节能）
	if (UElfEventManager* EventManager = GetEventManager())
	{
		EventManager->BroadcastEvent(FElfGameplayTags::Get().Battle_Trigger_UseElementSkill, Actor);
	}

	EInfoSide DeathSide = (Action.Side == EInfoSide::Self) ? EInfoSide::Enemy : EInfoSide::Self;
	CheckDeath(DeathSide);
	if (CurrentPhase == ETurnPhase::BattleEnd) return;

	// 受击方反击/反伤可能打死的攻击方
	CheckDeath(Action.Side);
	if (CurrentPhase == ETurnPhase::BattleEnd) return;

	if (Action.Side == EInfoSide::Self && Actor->bWishActive)
		CancelWish();

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
	UElfSkillBase* TargetSkill = GetActionSkillInstance(Target);
	UElfSkillBase* CounterSkill = GetActionSkillInstance(Counter);
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
	UElfSkillBase* Skill = GetActionSkillInstance(Counter);
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

	int32 BaseDamage = AttackSkill->CalculateInstanceDamage(ModifiedAttacker, ModifiedTarget);
	if (BaseDamage <= 0) return;
	int32 Damage = BaseDamage;

	// 攻击方总伤害增幅（buff 威力 + 直接伤害乘区 + 攻击方特性增伤）
	Damage = FMath::Max(1, FMath::RoundToInt(Damage * GetAttackerDamageMultiplier(AttackerSide, TargetSide, SkillInstance->GetSkillDataRef())));

	// 防守方特性减伤（如 属性亲和：受到自身携带技能系别攻击 -40%）
	Damage = FMath::Max(1, FMath::RoundToInt(Damage * GetDefenderDamageMultiplier(TargetSide, SkillInstance->GetSkillDataRef())));

	UElfGameInstance* GI = GetGameInstance();
	float TypeMult = 1.0f;
	if (GI)
	{
		FElfBaseData TargetBaseData;
		if (GI->GetElfBaseData(Target->CreatureRowName, TargetBaseData))
		{
			TypeMult = ElfTypeChart::GetMultiplier(
				SkillInstance->GetSkillDataRef().ElementType,
				TargetBaseData.Type1,
				TargetBaseData.Type2,
				GI->TypeChartTable);
			Damage = FMath::Max(1, FMath::RoundToInt(Damage * TypeMult));
		}
	}

	Damage = FMath::Max(1, FMath::RoundToInt(Damage * DamageModifier));

	Target->CurrentHP = FMath::Max(0, Target->CurrentHP - Damage);

	// 吸血：攻击方有吸血buff时，按实际造成伤害的比例回复生命（封顶最大HP）
	if (BuffManager)
	{
		int32 LifestealPercent = BuffManager->GetLifestealPercent(AttackerSide);
		if (LifestealPercent > 0)
		{
			int32 Heal = FMath::RoundToInt(Damage * LifestealPercent / 100.0f);
			if (Heal > 0)
			{
				Attacker->CurrentHP = FMath::Min(AttackerStats->MaxHP, Attacker->CurrentHP + Heal);
				if (AttackerSide == EInfoSide::Self)
					BattleController->OnSelfCreatureHPChanged.Broadcast(Attacker->CurrentHP, AttackerStats->MaxHP);
				else
					BattleController->OnEnemyCreatureHPChanged.Broadcast(Attacker->CurrentHP, AttackerStats->MaxHP);
			}
		}
	}

	// 特性触发：受到伤害（每段）+ 克制伤害（每段一次）
	if (UElfEventManager* EventManager = GetEventManager())
	{
		const FElfGameplayTags& Tags = FElfGameplayTags::Get();
		EventManager->BroadcastEvent(Tags.Battle_Trigger_TakeDamage, Target);
		if (TypeMult > 1.0f)
			EventManager->BroadcastEvent(Tags.Battle_Trigger_DealSuperEffective, Attacker);
	}

	if (TargetSide == EInfoSide::Self)
	{
		BattleController->OnSelfCreatureHPChanged.Broadcast(Target->CurrentHP, TargetStats->MaxHP);
	}
	else
	{
		BattleController->OnEnemyCreatureHPChanged.Broadcast(Target->CurrentHP, TargetStats->MaxHP);
	}
}

float UElfTurnManager::GetAttackerDamageMultiplier(EInfoSide AttackerSide, EInfoSide TargetSide, const FSkillData& SkillData) const
{
	float Multiplier = 1.0f;

	// 1. buff 威力修正（ModifyEnergyCostAndPower）
	TArray<const FActiveBuff*> AttackBuffs;
	if (BuffManager)
		BuffManager->CollectActiveBuffs(AttackerSide, AttackBuffs);
	float PowerMod = 1.0f;
	for (const FActiveBuff* Buff : AttackBuffs)
	{
		if (Buff->EffectID == EEffectID::ModifyEnergyCostAndPower && Buff->Params.IsValidIndex(0))
		{
			PowerMod += Buff->Params[0] / 100.0f * Buff->StackCount;
		}
	}
	Multiplier *= PowerMod;

	// 2. 直接伤害乘区（增益 × 减免，含条件数量）
	if (BuffManager)
		Multiplier *= BuffManager->GetDirectDamageMultiplier(AttackerSide, TargetSide);

	// 3. 攻击方特性增伤（如 能耗1技能威力+50%）
	if (BattleModel)
	{
		FBattleSideData& SideData = (AttackerSide == EInfoSide::Self) ? BattleModel->PlayerSide : BattleModel->EnemySide;
		if (SideData.AbilityInstances.IsValidIndex(SideData.ActiveIndex))
		{
			if (UElfAbilityBase* Ability = SideData.AbilityInstances[SideData.ActiveIndex])
				Multiplier *= Ability->ModifySkillPower(AttackerSide, SkillData);
		}
	}

	return Multiplier;
}

float UElfTurnManager::GetDefenderDamageMultiplier(EInfoSide DefenderSide, const FSkillData& SkillData) const
{
	float Multiplier = 1.0f;

	// 防守方在场精灵特性减伤（如 属性亲和：受到自身携带技能系别攻击 -40%）
	if (BattleModel)
	{
		FBattleSideData& SideData = (DefenderSide == EInfoSide::Self) ? BattleModel->PlayerSide : BattleModel->EnemySide;
		if (SideData.AbilityInstances.IsValidIndex(SideData.ActiveIndex))
		{
			if (UElfAbilityBase* Ability = SideData.AbilityInstances[SideData.ActiveIndex])
				Multiplier *= Ability->ModifyIncomingDamage(DefenderSide, SkillData);
		}
	}

	return Multiplier;
}

int32 UElfTurnManager::GetSkillEnergyCost(EInfoSide Side, UElfSkillBase* SkillInstance) const
{
	if (!SkillInstance) return 0;
	const FSkillData& SkillData = SkillInstance->GetSkillDataRef();
	int32 Cost = BuffManager ? BuffManager->GetModifiedEnergyCost(Side, SkillInstance->GetInstanceEnergyCost()) : SkillInstance->GetInstanceEnergyCost();

	// 在场精灵特性修正能耗（如 水系：防御技能能耗-2）
	if (BattleModel)
	{
		FBattleSideData& SideData = (Side == EInfoSide::Self) ? BattleModel->PlayerSide : BattleModel->EnemySide;
		if (SideData.AbilityInstances.IsValidIndex(SideData.ActiveIndex))
		{
			if (UElfAbilityBase* Ability = SideData.AbilityInstances[SideData.ActiveIndex])
				Ability->ModifyEnergyCost(Side, SkillData, Cost);
		}
	}
	return FMath::Max(0, Cost);
}

int32 UElfTurnManager::GetSkillEnergyCost(EInfoSide Side, int32 SlotIndex, bool bIsDefault) const
{
	UElfSkillBase* Inst = bIsDefault
		? GetActiveDefaultSkillInstance(Side, SlotIndex)
		: GetActiveSkillInstance(Side, SlotIndex);
	return GetSkillEnergyCost(Side, Inst);
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

			// 特性触发：回复能量
			if (UElfEventManager* EventManager = GetEventManager())
			{
				EventManager->BroadcastEvent(FElfGameplayTags::Get().Battle_Trigger_RestoreEnergy, Actor);
			}
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
		OnForcedSwitchAllComplete.Broadcast();
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
	if (!BattleController || !IsCapturePending()) return;

	ChangePhase(ETurnPhase::CapturePhase);

	FElfCreatureInstance* Target = GetActiveCreature(EInfoSide::Enemy);
	if (!Target) { ClearCapturePending(); return; }

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) { ClearCapturePending(); return; }

	FElfBaseData BaseData;
	if (!GI->GetElfBaseData(Target->CreatureRowName, BaseData)) { ClearCapturePending(); return; }

	float Difficulty = FMath::Max(0.1f, BaseData.CaptureDifficulty);
	float BallRate = FMath::Max(0.1f, GetCaptureBallRate());
	float Chance = BallRate / Difficulty;

	bool bSuccess = FMath::FRandRange(0.0f, 1.0f) <= Chance;

	ClearCapturePending();
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
				int32 Cost = GetSkillEnergyCost(SideEnum, GetActiveSkillInstance(SideEnum, j));
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
	int32 Cost = GetSkillEnergyCost(Side, SkillInstance);
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
	if (CurrentPhase == ETurnPhase::BattleEnd) return;
	CheckDeath(Side); // 迅捷攻击的受击方反击可能打死的迅捷使用者
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
	// 特性触发：回合结束
	if (UElfEventManager* EventManager = GetEventManager())
	{
		const FElfGameplayTags& Tags = FElfGameplayTags::Get();
		EventManager->BroadcastEvent(Tags.Battle_Trigger_TurnEnd, GetActiveCreature(EInfoSide::Self));
		EventManager->BroadcastEvent(Tags.Battle_Trigger_TurnEnd, GetActiveCreature(EInfoSide::Enemy));
	}

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

	// 特性触发：死亡（在胜负结算/换将前）
	if (UElfEventManager* EventManager = GetEventManager())
	{
		EventManager->BroadcastEvent(FElfGameplayTags::Get().Battle_Trigger_OnDeath, Creature);
	}

	// 魔力值扣除：精灵被击倒扣 1，扣到 0 判负（初始 4，即最多可被击倒 4 只）
	// 特性 牺牲：死亡时不消耗魔力值
	bool bNoMagicCost = false;
	{
		FBattleSideData* SideData = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
		if (SideData && SideData->AbilityInstances.IsValidIndex(SideData->ActiveIndex))
		{
			if (UElfAbilityBase* Ability = SideData->AbilityInstances[SideData->ActiveIndex])
				bNoMagicCost = Ability->IsNoMagicCostOnDeath();
		}
	}
	if (!bNoMagicCost)
	{
		if (Side == EInfoSide::Self) PlayerMagicPoints--;
		else EnemyMagicPoints--;
	}

	if (PlayerMagicPoints <= 0)
	{
		EndBattle(EBattleResult::PlayerLose);
		return;
	}

	if (EnemyMagicPoints <= 0)
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

UElfSkillBase* UElfTurnManager::GetActionSkillInstance(const FTurnAction& Action) const
{
	return Action.bIsDefault
		? GetActiveDefaultSkillInstance(Action.Side, Action.SlotIndex)
		: GetActiveSkillInstance(Action.Side, Action.SlotIndex);
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

// ==================== 道具 / 捕捉（迁移自 BattleController） ====================

void UElfTurnManager::InitCaptureItemQuantities()
{
	UElfGameInstance* GI = GetGameInstance();
	if (!GI || !GI->ItemDataTable) return;

	CaptureItemQuantities.Empty();
	static const FString Context(TEXT("ReplenishCapture"));
	TArray<FName> RowNames = GI->ItemDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		const FItemData* Item = GI->ItemDataTable->FindRow<FItemData>(RowName, Context);
		if (Item && Item->ItemType == EItemType::Capture)
			CaptureItemQuantities.Add(RowName, 5);
	}
}

bool UElfTurnManager::UseItem(FName ItemRowName)
{
	if (ItemRowName.IsNone()) return false;

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) return false;

	FItemData ItemDef;
	if (!GI->GetItemData(ItemRowName, ItemDef)) return false;

	if (ItemDef.ItemType != EItemType::Battle) return false;

	// 重复点击 → 取消
	if (PendingItemRowName == ItemRowName)
	{
		if (ItemDef.EffectID == EEffectID::WishSkill)
			CancelWish();
		else
		{
			FElfCreatureInstance* Creature = BattleModel ? BattleModel->PlayerSide.GetActiveCreature() : nullptr;
			if (Creature) Creature->bPendingEvolution = false;
			PendingItemRowName = NAME_None;
			bItemUsedThisTurn = false;
		}
		return true;
	}

	int32* Remain = ItemRemainingUses.Find(ItemRowName);
	int32 UsesLeft = Remain ? *Remain : ItemDef.MaxBattleUses;
	if (UsesLeft <= 0) return false;

	FElfCreatureInstance* Creature = BattleModel ? BattleModel->PlayerSide.GetActiveCreature() : nullptr;
	if (!Creature) return false;

	switch (ItemDef.EffectID)
	{
	case EEffectID::Evolution:
	{
		FElfBaseData BaseData;
		if (GI->GetElfBaseData(Creature->CreatureRowName, BaseData) && BaseData.Type3 == EElfType::Leader && !BaseData.EvolutionTarget.RowName.IsNone())
		{
			Creature->bPendingEvolution = true;
		}
		break;
	}
	case EEffectID::WishSkill:
	{
		if (!ItemDef.TargetRowName.IsNone())
		{
			FElfBaseData BaseData;
			bool bHasBaseData = GI->GetElfBaseData(Creature->CreatureRowName, BaseData);
			if (bHasBaseData && BaseData.Type3 == EElfType::Leader) break;

			EElfType BloodlineType = bHasBaseData ? BaseData.Type3 : EElfType::Normal;
			int32 ActiveIdx = BattleModel->PlayerSide.ActiveIndex;

			// 存档原技能0，替换为愿力冲击
			Creature->BackupFirstSkill = Creature->EquippedSkills.IsValidIndex(0) ? Creature->EquippedSkills[0] : FName();
			Creature->EquippedSkills[0] = ItemDef.TargetRowName;
			Creature->bWishActive = true;

			// 创建愿力冲击技能实例
			FSkillData SkillData;
			if (GI->GetSkillData(ItemDef.TargetRowName, SkillData) && SkillData.SkillClass)
			{
				SkillData.ElementType = BloodlineType;
				UElfSkillBase* Instance = NewObject<UElfSkillBase>(BattleModel, SkillData.SkillClass);
				Instance->Init(SkillData);

				if (BattleModel->PlayerSide.SkillInstances.IsValidIndex(ActiveIdx))
				{
					if (BattleModel->PlayerSide.SkillInstances[ActiveIdx].Instances.IsValidIndex(0))
						BattleModel->PlayerSide.SkillInstances[ActiveIdx].Instances[0] = Instance;
					else
						BattleModel->PlayerSide.SkillInstances[ActiveIdx].Instances.Insert(Instance, 0);
				}
			}
		}
		break;
	}
	default:
		return false;
	}

	bItemUsedThisTurn = true;
	PendingItemRowName = ItemRowName;

	return true;
}

bool UElfTurnManager::CanUseBattleItem(FName ItemRowName) const
{
	if (bItemUsedThisTurn) return false;

	if (!BattleModel) return false;
	const FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature) return false;

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) return false;

	FItemData ItemDef;
	if (!GI->GetItemData(ItemRowName, ItemDef)) return false;

	FElfBaseData BaseData;
	bool bHasBase = GI->GetElfBaseData(Creature->CreatureRowName, BaseData);
	if (ItemDef.EffectID == EEffectID::WishSkill)
	{
		if (!bHasBase || BaseData.Type3 == EElfType::Leader) return false;
	}
	else if (ItemDef.EffectID == EEffectID::Evolution)
	{
		if (!bHasBase || BaseData.Type3 != EElfType::Leader) return false;
	}

	return true;
}

int32 UElfTurnManager::GetItemRemainingUses(FName ItemRowName) const
{
	// 初始化：从数据表读取最大使用次数
	UElfGameInstance* GI = GetGameInstance();

	if (GI)
	{
		FItemData ItemData;
		if (GI->GetItemData(ItemRowName, ItemData) && !ItemRemainingUses.Contains(ItemRowName))
		{
			const_cast<UElfTurnManager*>(this)->ItemRemainingUses.Add(ItemRowName, ItemData.MaxBattleUses);
		}
	}

	// 检查是否有其他战斗道具已被使用（每局只能用一个）
	if (GI && GI->ItemDataTable)
	{
		static const FString Context(TEXT("GetBattleItemUses"));
		TArray<FName> RowNames = GI->ItemDataTable->GetRowNames();
		for (const FName& Other : RowNames)
		{
			if (Other == ItemRowName) continue;
			const FItemData* OtherData = GI->ItemDataTable->FindRow<FItemData>(Other, Context);
			if (OtherData && OtherData->ItemType == EItemType::Battle)
			{
				const int32* Remain = ItemRemainingUses.Find(Other);
				if (Remain && *Remain < OtherData->MaxBattleUses)
					return 0;
			}
		}
	}

	const int32* Remain = ItemRemainingUses.Find(ItemRowName);
	return Remain ? *Remain : 0;
}

bool UElfTurnManager::IsItemCompatibleWithCreature(FName ItemRowName) const
{
	if (!BattleModel) return false;
	const FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature) return false;

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) return false;

	FItemData ItemDef;
	if (!GI->GetItemData(ItemRowName, ItemDef)) return false;

	FElfBaseData BaseData;
	bool bHasBase = GI->GetElfBaseData(Creature->CreatureRowName, BaseData);
	if (!bHasBase) return false;

	if (ItemDef.EffectID == EEffectID::WishSkill)
		return BaseData.Type3 != EElfType::Leader;
	if (ItemDef.EffectID == EEffectID::Evolution)
		return BaseData.Type3 == EElfType::Leader;

	return false;
}

FName UElfTurnManager::FindBattleItemRowName(EEffectID EffectID)
{
	// 缓存已查到的行名
	if (EffectID == EEffectID::WishSkill && !CachedWishRowName.IsNone())
		return CachedWishRowName;
	if (EffectID == EEffectID::Evolution && !CachedEvoRowName.IsNone())
		return CachedEvoRowName;

	UElfGameInstance* GI = GetGameInstance();
	if (!GI || !GI->ItemDataTable) return NAME_None;

	static const FString Context(TEXT("FindBattleItem"));
	TArray<FName> RowNames = GI->ItemDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		const FItemData* Item = GI->ItemDataTable->FindRow<FItemData>(RowName, Context);
		if (Item && Item->ItemType == EItemType::Battle && Item->EffectID == EffectID)
		{
			if (EffectID == EEffectID::WishSkill) CachedWishRowName = RowName;
			if (EffectID == EEffectID::Evolution) CachedEvoRowName = RowName;
			return RowName;
		}
	}
	return NAME_None;
}

void UElfTurnManager::UseBattleItem()
{
	FName WishRow = FindBattleItemRowName(EEffectID::WishSkill);
	FName EvoRow = FindBattleItemRowName(EEffectID::Evolution);
	FName ItemToUse = (!WishRow.IsNone() && IsItemCompatibleWithCreature(WishRow)) ? WishRow : EvoRow;
	if (!ItemToUse.IsNone())
	{
		UseItem(ItemToUse);
		if (BattleController)
			BattleController->OnBattleItemClicked.Broadcast(ItemToUse);
	}
}

FName UElfTurnManager::GetBattleItemRowName()
{
	FName WishRow = FindBattleItemRowName(EEffectID::WishSkill);
	FName EvoRow = FindBattleItemRowName(EEffectID::Evolution);

	if (IsItemCompatibleWithCreature(WishRow))
		return WishRow;
	if (IsItemCompatibleWithCreature(EvoRow))
		return EvoRow;
	return NAME_None;
}

int32 UElfTurnManager::GetBattleItemCount()
{
	return GetBattleItemList().Num();
}

FName UElfTurnManager::GetBattleItemAtSlot(int32 FlatIndex)
{
	return GetBattleItemList().IsValidIndex(FlatIndex) ? GetBattleItemList()[FlatIndex] : NAME_None;
}

const TArray<FName>& UElfTurnManager::GetBattleItemList()
{
	UElfGameInstance* GI = GetGameInstance();
	if (!GI || !GI->ItemDataTable) return CachedBattleItemList;

	// 检查精灵是否变更
	int32 CurrentActive = BattleModel ? BattleModel->PlayerSide.ActiveIndex : -1;
	if (CurrentActive == LastBattleItemActiveIndex && !CachedBattleItemList.IsEmpty())
		return CachedBattleItemList;

	LastBattleItemActiveIndex = CurrentActive;
	CachedBattleItemList.Empty();

	static const FString Context(TEXT("BuildBattleItemList"));
	for (const FName& RowName : GI->ItemDataTable->GetRowNames())
	{
		const FItemData* Item = GI->ItemDataTable->FindRow<FItemData>(RowName, Context);
		if (Item && Item->ItemType == EItemType::Battle && IsItemCompatibleWithCreature(RowName))
			CachedBattleItemList.Add(RowName);
	}
	return CachedBattleItemList;
}

void UElfTurnManager::UseCaptureItem(int32 FlatIndex)
{
	const TArray<FName>& List = GetCaptureItemList();
	if (!List.IsValidIndex(FlatIndex)) return;

	FName RowName = List[FlatIndex];
	int32* Qty = CaptureItemQuantities.Find(RowName);
	if (!Qty || *Qty <= 0) return;

	(*Qty)--;
	if (UElfGameInstance* GI = GetGameInstance())
	{
		if (int32* GIQty = GI->CaptureItemQuantities.Find(RowName))
			(*GIQty)--;
	}
	bCapturePending = true;
	PendingCaptureBallRate = 0.0f;

	UElfGameInstance* GI2 = GetGameInstance();
	FItemData ItemData;
	if (GI2 && GI2->GetItemData(RowName, ItemData) && ItemData.Params.IsValidIndex(0))
		PendingCaptureBallRate = ItemData.Params[0];

	bItemUsedThisTurn = true;
	if (BattleController)
		BattleController->OnCaptureConfirmed.Broadcast();
}

int32 UElfTurnManager::GetCaptureItemCount()
{
	return GetCaptureItemList().Num();
}

FName UElfTurnManager::GetCaptureItemAtSlot(int32 FlatIndex)
{
	GetCaptureItemList(); // 确保列表已构建
	return CachedCaptureItemList.IsValidIndex(FlatIndex) ? CachedCaptureItemList[FlatIndex] : NAME_None;
}

const TArray<FName>& UElfTurnManager::GetCaptureItemList()
{
	if (CachedCaptureItemList.IsEmpty())
	{
		CachedCaptureItemList.Empty();
		UElfGameInstance* GI = GetGameInstance();
		if (GI && GI->ItemDataTable)
		{
			static const FString Context(TEXT("BuildCaptureList"));
			for (const FName& RowName : GI->ItemDataTable->GetRowNames())
			{
				const FItemData* Item = GI->ItemDataTable->FindRow<FItemData>(RowName, Context);
				if (Item && Item->ItemType == EItemType::Capture)
					CachedCaptureItemList.Add(RowName);
			}
		}
	}
	return CachedCaptureItemList;
}

int32 UElfTurnManager::GetCaptureItemQuantity(FName ItemRowName) const
{
	return CaptureItemQuantities.FindRef(ItemRowName);
}

void UElfTurnManager::ConsumePendingItem()
{
	if (PendingItemRowName.IsNone()) return;

	UElfGameInstance* GI = GetGameInstance();
	if (!GI) return;

	FItemData ItemData;
	if (!GI->GetItemData(PendingItemRowName, ItemData)) return;

	int32* Remain = ItemRemainingUses.Find(PendingItemRowName);
	int32 Uses = Remain ? *Remain : ItemData.MaxBattleUses;
	if (Uses <= 0) return;

	Uses--;
	ItemRemainingUses.Add(PendingItemRowName, Uses);
	if (BattleController)
		BattleController->OnItemUsed.Broadcast(PendingItemRowName, Uses);

	PendingItemRowName = NAME_None;
}

void UElfTurnManager::CancelWish()
{
	if (!BattleModel) return;
	FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature || !Creature->bWishActive) return;

	// 恢复原技能
	Creature->EquippedSkills[0] = Creature->BackupFirstSkill;
	Creature->bWishActive = false;

	// 重新创建技能0的实例
	int32 ActiveIdx = BattleModel->PlayerSide.ActiveIndex;
	if (BattleModel->PlayerSide.SkillInstances.IsValidIndex(ActiveIdx))
	{
		if (!Creature->BackupFirstSkill.IsNone())
		{
			UElfGameInstance* GI = GetGameInstance();
			FSkillData SkillData;
			if (GI && GI->GetSkillData(Creature->BackupFirstSkill, SkillData) && SkillData.SkillClass)
			{
				UElfSkillBase* Instance = NewObject<UElfSkillBase>(BattleModel, SkillData.SkillClass);
				Instance->Init(SkillData);
				if (BattleModel->PlayerSide.SkillInstances[ActiveIdx].Instances.IsValidIndex(0))
					BattleModel->PlayerSide.SkillInstances[ActiveIdx].Instances[0] = Instance;
			}
		}
	}

	// 清除待定状态（未消耗次数，无需返还）
	PendingItemRowName = NAME_None;
	bItemUsedThisTurn = false;
}

void UElfTurnManager::RefundItem(FName ItemRowName)
{
	int32* Remain = ItemRemainingUses.Find(ItemRowName);
	if (Remain)
	{
		(*Remain)++;
		if (BattleController)
			BattleController->OnItemUsed.Broadcast(ItemRowName, *Remain);
	}
}

void UElfTurnManager::ResetBattleItemState()
{
	bItemUsedThisTurn = false;
	PendingItemRowName = NAME_None;
	bCapturePending = false;
	PendingCaptureBallRate = 0.0f;
}

void UElfTurnManager::ClearCapturePending()
{
	bCapturePending = false;
	PendingCaptureBallRate = 0.0f;
}
