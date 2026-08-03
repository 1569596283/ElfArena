#include "Ability/ElfAbilityManager.h"
#include "Ability/ElfAbilityBase.h"
#include "Battle/ElfTurnManager.h"
#include "Battle/ElfBuffManager.h"
#include "Event/ElfEventManager.h"
#include "ElfGameplayTags.h"
#include "Data/ElfAbilityData.h"
#include "Data/ElfBaseData.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Game/ElfGameInstance.h"
#include "GameFramework/PlayerController.h"

void UElfAbilityManager::Init(UElfBattleController* InBC, UElfBattleModel* InBM)
{
	BattleController = InBC;
	BattleModel = InBM;
	TurnManager = InBC ? InBC->GetTurnManager() : nullptr;
	BuffManager = TurnManager ? TurnManager->GetBuffManager() : nullptr;

	UElfGameInstance* GI = InBC && InBC->GetOwnerPC() ? InBC->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	EventManager = GI ? GI->GetSubsystem<UElfEventManager>() : nullptr;

	// 特性只在战斗内有意义，实例在 AbilityManager 初始化时创建（BuffManager 已就绪）
	CreateAbilityInstances();

	if (EventManager)
	{
		EventManager->OnGameplayEvent.AddUObject(this, &UElfAbilityManager::HandleGameplayEvent);
	}
}

void UElfAbilityManager::CreateAbilityInstances()
{
	if (!BattleModel) return;

	// 需要 GI 来查数据表
	UElfGameInstance* GI = BattleController && BattleController->GetOwnerPC()
		? BattleController->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return;

	auto CreateForSide = [&](FBattleSideData& Side)
	{
		Side.AbilityInstances.SetNum(Side.Team.Num());
		for (int32 i = 0; i < Side.Team.Num(); i++)
		{
			const FName& CreatureRowName = Side.Team[i].CreatureRowName;

			FElfBaseData BaseData;
			if (!GI->GetElfBaseData(CreatureRowName, BaseData) || BaseData.AbilityID.IsNone())
				continue;

			FAbilityData AbilityData;
			if (!GI->GetAbilityData(BaseData.AbilityID, AbilityData) || !AbilityData.AbilityClass)
				continue;

			UElfAbilityBase* Instance = NewObject<UElfAbilityBase>(BattleModel, AbilityData.AbilityClass);
			Instance->Init(BaseData.AbilityID, AbilityData.Trigger, AbilityData.Effects);
			Instance->SetContext(BattleModel, BuffManager, TurnManager, BattleController);
			Side.AbilityInstances[i] = Instance;
		}
	};

	CreateForSide(BattleModel->PlayerSide);
	CreateForSide(BattleModel->EnemySide);
}

void UElfAbilityManager::TriggerEnter(EInfoSide Side)
{
	if (!BattleModel) return;
	FBattleSideData* SideData = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
	if (!SideData) return;
	TriggerCreatureEnter(SideData->GetActiveCreature());
}

void UElfAbilityManager::TriggerEnterBattle()
{
	if (!BattleModel) return;

	const FElfCreatureInstance* SelfCreature = BattleModel->PlayerSide.GetActiveCreature();
	const FElfCreatureInstance* EnemyCreature = BattleModel->EnemySide.GetActiveCreature();
	if (!SelfCreature && !EnemyCreature) return;

	// 按速度排序：快者先触发（没有的都排最后）
	int32 SelfSpeed = SelfCreature ? GetCreatureSpeed(SelfCreature) : -1;
	int32 EnemySpeed = EnemyCreature ? GetCreatureSpeed(EnemyCreature) : -1;

	if (!EnemyCreature || (SelfCreature && SelfSpeed >= EnemySpeed))
	{
		if (SelfCreature) TriggerCreatureEnter(SelfCreature);
		if (EnemyCreature) TriggerCreatureEnter(EnemyCreature);
	}
	else
	{
		if (EnemyCreature) TriggerCreatureEnter(EnemyCreature);
		if (SelfCreature) TriggerCreatureEnter(SelfCreature);
	}
}

void UElfAbilityManager::TriggerEnterForced(EInfoSide Side)
{
	if (!BattleModel) return;
	FBattleSideData* SideData = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
	if (!SideData) return;
	TriggerCreatureEnter(SideData->GetActiveCreature());
}

void UElfAbilityManager::TriggerCreatureEnter(const FElfCreatureInstance* Creature)
{
	if (!Creature || !BattleModel) return;

	// 定位精灵所在侧与索引
	EInfoSide Side = EInfoSide::Self;
	int32 Index = -1;
	for (int32 SideIdx = 0; SideIdx < 2; SideIdx++)
	{
		EInfoSide CheckSide = (SideIdx == 0) ? EInfoSide::Self : EInfoSide::Enemy;
		FBattleSideData* SideData = (CheckSide == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
		if (!SideData) continue;

		for (int32 i = 0; i < SideData->Team.Num(); i++)
		{
			if (&SideData->Team[i] == Creature)
			{
				Side = CheckSide;
				Index = i;
				break;
			}
		}
		if (Index >= 0) break;
	}

	if (Index < 0) return;

	FBattleSideData* TargetSide = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
	if (!TargetSide || !TargetSide->AbilityInstances.IsValidIndex(Index)) return;

	if (UElfAbilityBase* Ability = TargetSide->AbilityInstances[Index])
	{
		const FGameplayTag& EnterTag = FElfGameplayTags::Get().Battle_Trigger_EnterBattle;
		if (Ability->IsTriggerMatch(EnterTag) && Ability->CanTrigger(Creature))
		{
			Ability->TriggerAbility(Creature);
			if (BattleController)
				BattleController->OnCreatureAbility.Broadcast(Side, Ability->GetAbilityID());
		}
	}
}

int32 UElfAbilityManager::GetCreatureSpeed(const FElfCreatureInstance* Creature) const
{
	if (!Creature || !BattleModel) return 0;
	for (int32 SideIdx = 0; SideIdx < 2; SideIdx++)
	{
		EInfoSide CheckSide = (SideIdx == 0) ? EInfoSide::Self : EInfoSide::Enemy;
		FBattleSideData* SideData = (CheckSide == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
		if (!SideData) continue;
		for (int32 i = 0; i < SideData->CalculatedStats.Num(); i++)
		{
			if (&SideData->Team[i] == Creature && SideData->CalculatedStats.IsValidIndex(i))
				return SideData->CalculatedStats[i].SPD;
		}
	}
	return 0;
}

void UElfAbilityManager::HandleGameplayEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature)
{
	TriggerByEvent(EventTag, Creature);
}

void UElfAbilityManager::TriggerByEvent(const FGameplayTag& EventTag, const FElfCreatureInstance* Creature)
{
	if (!BattleModel) return;

	// 定位事件对应精灵所在侧与索引
	EInfoSide Side = EInfoSide::Self;
	int32 Index = -1;
	for (int32 SideIdx = 0; SideIdx < 2; SideIdx++)
	{
		EInfoSide CheckSide = (SideIdx == 0) ? EInfoSide::Self : EInfoSide::Enemy;
		FBattleSideData* SideData = (CheckSide == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
		if (!SideData) continue;

		for (int32 i = 0; i < SideData->Team.Num(); i++)
		{
			if (&SideData->Team[i] == Creature)
			{
				Side = CheckSide;
				Index = i;
				break;
			}
		}
		if (Index >= 0) break;
	}

	if (Index < 0)
	{
		// 未定位到精灵（如全局事件）→ 遍历两侧全部触发
		for (int32 SideIdx = 0; SideIdx < 2; SideIdx++)
		{
			EInfoSide CheckSide = (SideIdx == 0) ? EInfoSide::Self : EInfoSide::Enemy;
			FBattleSideData* SideData = (CheckSide == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
			if (!SideData) continue;
			for (UElfAbilityBase* Ability : SideData->AbilityInstances)
			{
				if (!Ability) continue;
				if (Ability->IsTriggerMatch(EventTag) && Ability->CanTrigger(Creature))
				{
					Ability->TriggerAbility(Creature);
					if (BattleController)
						BattleController->OnCreatureAbility.Broadcast(CheckSide, Ability->GetAbilityID());
				}
			}
		}
		return;
	}

	// 只触发该精灵的特性实例
	FBattleSideData* TargetSide = (Side == EInfoSide::Self) ? &BattleModel->PlayerSide : &BattleModel->EnemySide;
	if (!TargetSide || !TargetSide->AbilityInstances.IsValidIndex(Index)) return;
	if (UElfAbilityBase* Ability = TargetSide->AbilityInstances[Index])
	{
		if (Ability->IsTriggerMatch(EventTag) && Ability->CanTrigger(Creature))
		{
			Ability->TriggerAbility(Creature);
			if (BattleController)
				BattleController->OnCreatureAbility.Broadcast(Side, Ability->GetAbilityID());
		}
	}
}
