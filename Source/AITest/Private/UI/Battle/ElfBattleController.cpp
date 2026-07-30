#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Data/ElfSkillData.h"
#include "Data/ElfItemData.h"
#include "Data/ElfBaseData.h"
#include "Skill/ElfSkillBase.h"
#include "Skill/Attack/AttackSkillBase.h"
#include "Game/ElfGameInstance.h"
#include "Player/ElfPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "ElfGameplayTags.h"

void UElfBattleController::InitCaptureItemQuantities()
{
	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return;

	CaptureItemQuantities = GI->CaptureItemQuantities;
}

void UElfBattleController::Init(APlayerController* InOwner, EBattleType Type, AActor* Opponent)
{
	OwnerPC = InOwner;
	BattleModel = NewObject<UElfBattleModel>(this);
	BattleModel->Init(InOwner, Type, Opponent);
	InitCaptureItemQuantities();
	BroadcastHP();
}

void UElfBattleController::CompleteIntro()
{
	OnIntroComplete.Broadcast();
}

void UElfBattleController::SwitchToBattleCamera()
{
	OnCameraRequested.Broadcast();
}

void UElfBattleController::SelectCreature(int32 Index)
{
	if (bLocalPlayerReady) return;
	if (!BattleModel) return;

	if (Index >= 0 && Index < BattleModel->PlayerSide.Team.Num())
	{
		SelectedSlotIndex = Index;
		OnCreatureSelected.Broadcast(Index);
	}
}

void UElfBattleController::ConfirmReady()
{
	if (bLocalPlayerReady) return;
	bLocalPlayerReady = true;
	OnPlayerReadyStateChanged.Broadcast(true);
}

void UElfBattleController::CancelReady()
{
	if (!bLocalPlayerReady) return;
	bLocalPlayerReady = false;
	OnPlayerReadyStateChanged.Broadcast(false);
}

int32 UElfBattleController::CalculateSkillPower(int32 SlotIndex)
{
	if (!BattleModel) return 0;
	FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature->EquippedSkills[SlotIndex], SkillData)) return 0;

	int32 BasePower = 0;
	for (const FSkillEffect& Effect : SkillData.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			BasePower = FMath::RoundToInt(Effect.Value);
			break;
		}
	}

	if (SkillData.SkillClass)
	{
		UAttackSkillBase* Attack = Cast<UAttackSkillBase>(SkillData.SkillClass.GetDefaultObject());
		if (Attack)
		{
			return Attack->CalculateDamage(this, SlotIndex);
		}
	}

	return BasePower;
}

int32 UElfBattleController::GetDefaultSkillPower(int32 SlotIndex) const
{
	if (!BattleModel) return 0;
	FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature->EquippedSkills[SlotIndex], SkillData)) return 0;

	for (const FSkillEffect& Effect : SkillData.Effects)
	{
		if (Effect.Type == EEffectType::Power)
		{
			return FMath::RoundToInt(Effect.Value);
		}
	}
	return 0;
}

int32 UElfBattleController::GetDefaultEnergyCost(int32 SlotIndex) const
{
	if (!BattleModel) return 0;
	FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature->EquippedSkills[SlotIndex], SkillData)) return 0;

	return FMath::Max(0, SkillData.EnergyCost);
}

void UElfBattleController::ApplyCounterEffect(int32 SlotIndex)
{
}

int32 UElfBattleController::GetSkillEnergyCost(int32 SlotIndex) const
{
	if (!BattleModel) return 0;
	FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature->EquippedSkills[SlotIndex], SkillData)) return 0;

	return SkillData.EnergyCost;
}

EElfType UElfBattleController::GetActiveCreatureBloodline() const
{
	if (!BattleModel) return EElfType::None;
	const FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature) return EElfType::None;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return EElfType::None;

	FElfBaseData BaseData;
	if (!GI->GetElfBaseData(Creature->CreatureRowName, BaseData)) return EElfType::None;

	return BaseData.Type3;
}

EElfType UElfBattleController::GetSkillElementType(int32 SlotIndex) const
{
	if (!BattleModel) return EElfType::None;
	FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return EElfType::None;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return EElfType::None;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature->EquippedSkills[SlotIndex], SkillData)) return EElfType::None;

	return SkillData.ElementType;
}

FName UElfBattleController::GetSkillRowName(int32 SlotIndex) const
{
	if (!BattleModel) return NAME_None;
	FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature || !Creature->EquippedSkills.IsValidIndex(SlotIndex)) return NAME_None;

	return Creature->EquippedSkills[SlotIndex];
}



int32 UElfBattleController::GetSelfTeamCount() const
{
	return BattleModel ? BattleModel->PlayerSide.Team.Num() : 0;
}

int32 UElfBattleController::GetEnemyTeamCount() const
{
	return BattleModel ? BattleModel->EnemySide.Team.Num() : 0;
}

int32 UElfBattleController::GetSelfAliveCount() const
{
	if (!BattleModel) return 0;
	int32 Count = 0;
	for (const FElfCreatureInstance& C : BattleModel->PlayerSide.Team)
	{
		if (C.CurrentHP > 0) Count++;
	}
	return Count;
}

int32 UElfBattleController::GetEnemyAliveCount() const
{
	if (!BattleModel) return 0;
	int32 Count = 0;
	for (const FElfCreatureInstance& C : BattleModel->EnemySide.Team)
	{
		if (C.CurrentHP > 0) Count++;
	}
	return Count;
}

FElfCreatureInstance UElfBattleController::GetSelfCreature(int32 Index) const
{
	if (BattleModel && BattleModel->PlayerSide.Team.IsValidIndex(Index))
	{
		return BattleModel->PlayerSide.Team[Index];
	}
	return FElfCreatureInstance();
}

FElfCreatureInstance UElfBattleController::GetEnemyCreature(int32 Index) const
{
	if (BattleModel && BattleModel->EnemySide.Team.IsValidIndex(Index))
	{
		return BattleModel->EnemySide.Team[Index];
	}
	return FElfCreatureInstance();
}

bool UElfBattleController::GetElfBaseData(FName RowName, FElfBaseData& OutData) const
{
	if (!OwnerPC) return false;
	UElfGameInstance* GI = OwnerPC->GetGameInstance<UElfGameInstance>();
	return GI ? GI->GetElfBaseData(RowName, OutData) : false;
}

EBattleType UElfBattleController::GetBattleType() const
{
	return BattleModel ? BattleModel->BattleType : EBattleType::Wild;
}

FString UElfBattleController::GetOpponentName() const
{
	return BattleModel ? BattleModel->OpponentName : FString();
}

FName UElfBattleController::GetOpponentAvatarID() const
{
	return BattleModel ? BattleModel->OpponentAvatarID : FName();
}

FString UElfBattleController::GetSelfPlayerName() const
{
	if (!OwnerPC) return FString();
	APlayerState* PS = OwnerPC->PlayerState;
	return PS ? PS->GetPlayerName() : FString();
}

FName UElfBattleController::GetSelfAvatarID() const
{
	if (!OwnerPC) return FName("Default");
	AElfPlayerState* PS = OwnerPC->GetPlayerState<AElfPlayerState>();
	return PS ? PS->AvatarID : FName("Default");
}

FName UElfBattleController::GetSelfCardID() const
{
	if (!OwnerPC) return FName("Default");
	AElfPlayerState* PS = OwnerPC->GetPlayerState<AElfPlayerState>();
	return PS ? PS->CardID : FName("Default");
}

void UElfBattleController::SetInputMode(EBattleInputMode NewMode)
{
	if (CurrentInputMode == NewMode) return;
	CurrentInputMode = NewMode;

	if (NewMode == EBattleInputMode::Capture)
	{
		if (PendingCaptureSlot < 0)
		{
			PendingCaptureSlot = GetCaptureItemCount() > 0 ? 0 : -1;
			OnCaptureSlotHighlighted.Broadcast(PendingCaptureSlot);
		}
		else
		{
			OnCaptureSlotHighlighted.Broadcast(PendingCaptureSlot);
		}
	}

	if (NewMode == EBattleInputMode::Switch)
	{
		if (PendingSwitchSlot < 0)
		{
			PendingSwitchSlot = 1;
			OnSwitchSlotHighlighted.Broadcast(1);
		}
		else
		{
			OnSwitchSlotHighlighted.Broadcast(PendingSwitchSlot);
		}
	}

	OnInputModeChanged.Broadcast(NewMode);
}

void UElfBattleController::HandleInput(const FGameplayTag& InputTag)
{
	UE_LOG(LogTemp, Warning, TEXT("HandleInput: %s"), *InputTag.ToString());
	const FElfGameplayTags& Tags = FElfGameplayTags::Get();

	// QWER 仅在玩家决策阶段可用
	if (CurrentTurnPhase == ETurnPhase::PlayerDecision && !bInputModeLocked)
	{
		if (InputTag == Tags.Input_Q) { SetInputMode(EBattleInputMode::Item); return; }
		if (InputTag == Tags.Input_W) { SetInputMode(EBattleInputMode::Capture); return; }
		if (InputTag == Tags.Input_E) { SetInputMode(EBattleInputMode::Switch); return; }
		if (InputTag == Tags.Input_R)
		{
			SetInputMode(CurrentInputMode == EBattleInputMode::Crafting
				? EBattleInputMode::Capture : EBattleInputMode::Command);
			return;
		}
	}
	switch (CurrentInputMode)
	{
	case EBattleInputMode::Command:
	{
		if (InputTag == Tags.Input_Slot1) { UseSkill(0); return; }
		if (InputTag == Tags.Input_Slot2) { UseSkill(1); return; }
		if (InputTag == Tags.Input_Slot3) { UseSkill(2); return; }
		if (InputTag == Tags.Input_Slot4) { UseSkill(3); return; }
		if (InputTag == Tags.Input_Slot5) { UseSkill(4); return; }
		if (InputTag == Tags.Input_Slot6) { UseSkill(5); return; }
		if (InputTag == Tags.Input_X) { UseDefaultSkill(0); return; }
		break;
	}
	case EBattleInputMode::Item:
	{
		if (InputTag == Tags.Input_Slot1 || InputTag == Tags.Input_Slot2 || InputTag == Tags.Input_Slot3 ||
			InputTag == Tags.Input_Slot4 || InputTag == Tags.Input_Slot5 || InputTag == Tags.Input_Slot6)
		{
			UseBattleItem();
			return;
		}
		break;
	}
	case EBattleInputMode::Switch:
	{
		if (InputTag == Tags.Input_Slot1) { PendingSwitchSlot = 1; OnSwitchSlotHighlighted.Broadcast(1); return; }
		if (InputTag == Tags.Input_Slot2) { PendingSwitchSlot = 2; OnSwitchSlotHighlighted.Broadcast(2); return; }
		if (InputTag == Tags.Input_Slot3) { PendingSwitchSlot = 3; OnSwitchSlotHighlighted.Broadcast(3); return; }
		if (InputTag == Tags.Input_Slot4) { PendingSwitchSlot = 4; OnSwitchSlotHighlighted.Broadcast(4); return; }
		if (InputTag == Tags.Input_Slot5) { PendingSwitchSlot = 5; OnSwitchSlotHighlighted.Broadcast(5); return; }
		if (InputTag == Tags.Input_Slot6) { PendingSwitchSlot = 6; OnSwitchSlotHighlighted.Broadcast(6); return; }
		if (InputTag == Tags.Input_Space && PendingSwitchSlot >= 0)
		{
			OnSwitchSlotSelected.Broadcast(PendingSwitchSlot);
			PendingSwitchSlot = -1;
			OnSwitchSlotHighlighted.Broadcast(-1);
			return;
		}
		break;
	}
	case EBattleInputMode::Capture:
	{
		if (InputTag == Tags.Input_Slot1) { PendingCaptureSlot = 0; OnCaptureSlotHighlighted.Broadcast(0); return; }
		if (InputTag == Tags.Input_Slot2) { PendingCaptureSlot = 1; OnCaptureSlotHighlighted.Broadcast(1); return; }
		if (InputTag == Tags.Input_Slot3) { PendingCaptureSlot = 2; OnCaptureSlotHighlighted.Broadcast(2); return; }
		if (InputTag == Tags.Input_Slot4) { PendingCaptureSlot = 3; OnCaptureSlotHighlighted.Broadcast(3); return; }
		if (InputTag == Tags.Input_Slot5) { PendingCaptureSlot = 4; OnCaptureSlotHighlighted.Broadcast(4); return; }
		if (InputTag == Tags.Input_Slot6) { PendingCaptureSlot = 5; OnCaptureSlotHighlighted.Broadcast(5); return; }
		if (InputTag == Tags.Input_Space)
		{
			UE_LOG(LogTemp, Warning, TEXT("Space in Capture mode, PendingCaptureSlot=%d, CaptureItemCount=%d"), PendingCaptureSlot, GetCaptureItemCount());
			if (PendingCaptureSlot >= 0)
			{
				UseCaptureItem(PendingCaptureSlot);
			}
			return;
		}
		if (InputTag == Tags.Input_X) { SetInputMode(EBattleInputMode::Crafting); return; }
		break;
	}
	case EBattleInputMode::Crafting:
	{
		if (InputTag == Tags.Input_Slot1) { OnCraftingSlotSelected.Broadcast(0); return; }
		if (InputTag == Tags.Input_Slot2) { OnCraftingSlotSelected.Broadcast(1); return; }
		if (InputTag == Tags.Input_Slot3) { OnCraftingSlotSelected.Broadcast(2); return; }
		if (InputTag == Tags.Input_Slot4) { OnCraftingSlotSelected.Broadcast(3); return; }
		if (InputTag == Tags.Input_Slot5) { OnCraftingSlotSelected.Broadcast(4); return; }
		if (InputTag == Tags.Input_Slot6) { OnCraftingSlotSelected.Broadcast(5); return; }
		break;
	}
	}
}

void UElfBattleController::UseSkill(int32 SlotIndex)
{
	OnSkillSelected.Broadcast(SlotIndex);
}

void UElfBattleController::UseDefaultSkill(int32 SlotIndex)
{
	OnDefaultSkillSelected.Broadcast(SlotIndex);
}

int32 UElfBattleController::GetBattleItemCount()
{
	return GetBattleItemList().Num();
}

FName UElfBattleController::GetBattleItemAtSlot(int32 FlatIndex)
{
	return GetBattleItemList().IsValidIndex(FlatIndex) ? GetBattleItemList()[FlatIndex] : NAME_None;
}

const TArray<FName>& UElfBattleController::GetBattleItemList()
{
	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
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

FName UElfBattleController::GetBattleItemRowName()
{
	FName WishRow = FindBattleItemRowName(EEffectID::WishSkill);
	FName EvoRow = FindBattleItemRowName(EEffectID::Evolution);

	if (IsItemCompatibleWithCreature(WishRow))
		return WishRow;
	if (IsItemCompatibleWithCreature(EvoRow))
		return EvoRow;
	return NAME_None;
}

void UElfBattleController::UseBattleItem()
{
	FName WishRow = FindBattleItemRowName(EEffectID::WishSkill);
	FName EvoRow = FindBattleItemRowName(EEffectID::Evolution);
	FName ItemToUse = (!WishRow.IsNone() && IsItemCompatibleWithCreature(WishRow)) ? WishRow : EvoRow;
	if (!ItemToUse.IsNone())
	{
		UseItem(ItemToUse);
		OnBattleItemClicked.Broadcast(ItemToUse);
	}
}

void UElfBattleController::UseCaptureItem(int32 FlatIndex)
{
	const TArray<FName>& List = GetCaptureItemList();
	UE_LOG(LogTemp, Warning, TEXT("UseCaptureItem: FlatIndex=%d, List.Num=%d"), FlatIndex, List.Num());
	if (!List.IsValidIndex(FlatIndex)) { UE_LOG(LogTemp, Warning, TEXT("→ Invalid index")); return; }

	FName RowName = List[FlatIndex];
	UE_LOG(LogTemp, Warning, TEXT("→ RowName=%s, List[0]=%s, List[1]=%s, List[2]=%s"), *RowName.ToString(),
		List.IsValidIndex(0) ? *List[0].ToString() : TEXT("?"),
		List.IsValidIndex(1) ? *List[1].ToString() : TEXT("?"),
		List.IsValidIndex(2) ? *List[2].ToString() : TEXT("?"));
	UE_LOG(LogTemp, Warning, TEXT("→ CaptureItemQuantities keys:"));
	for (const auto& Pair : CaptureItemQuantities)
		UE_LOG(LogTemp, Warning, TEXT("    %s = %d"), *Pair.Key.ToString(), Pair.Value);
	int32* Qty = CaptureItemQuantities.Find(RowName);
	if (!Qty) { UE_LOG(LogTemp, Warning, TEXT("→ Not found in quantities")); return; }
	if (*Qty <= 0) { UE_LOG(LogTemp, Warning, TEXT("→ Quantity is 0")); return; }

	(*Qty)--;
	if (UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr)
	{
		if (int32* GIQty = GI->CaptureItemQuantities.Find(RowName))
			(*GIQty)--;
	}
	bCapturePending = true;
	PendingCaptureBallRate = 0.0f;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	FItemData ItemData;
	if (GI && GI->GetItemData(RowName, ItemData) && ItemData.Params.IsValidIndex(0))
		PendingCaptureBallRate = ItemData.Params[0];

	bItemUsedThisTurn = true;
	OnCaptureConfirmed.Broadcast();
}

int32 UElfBattleController::GetCaptureItemCount()
{
	return GetCaptureItemList().Num();
}

FName UElfBattleController::GetCaptureItemAtSlot(int32 FlatIndex)
{
	GetCaptureItemList(); // 确保列表已构建
	return CachedCaptureItemList.IsValidIndex(FlatIndex) ? CachedCaptureItemList[FlatIndex] : NAME_None;
}

const TArray<FName>& UElfBattleController::GetCaptureItemList()
{
	if (CachedCaptureItemList.IsEmpty())
	{
		CachedCaptureItemList.Empty();
		UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
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

bool UElfBattleController::UseItem(FName ItemRowName)
{
	if (ItemRowName.IsNone()) return false;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
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

bool UElfBattleController::CanUseBattleItem(FName ItemRowName) const
{
	if (bItemUsedThisTurn) return false;

	if (!BattleModel) return false;
	const FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature) return false;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
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

int32 UElfBattleController::GetItemRemainingUses(FName ItemRowName) const
{
	// 初始化：从数据表读取最大使用次数
	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;

	if (GI)
	{
		FItemData ItemData;
		if (GI->GetItemData(ItemRowName, ItemData) && !ItemRemainingUses.Contains(ItemRowName))
		{
			const_cast<UElfBattleController*>(this)->ItemRemainingUses.Add(ItemRowName, ItemData.MaxBattleUses);
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

bool UElfBattleController::IsItemCompatibleWithCreature(FName ItemRowName) const
{
	if (!BattleModel) return false;
	const FElfCreatureInstance* Creature = BattleModel->PlayerSide.GetActiveCreature();
	if (!Creature) return false;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
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

FName UElfBattleController::FindBattleItemRowName(EEffectID EffectID)
{
	// 缓存已查到的行名
	if (EffectID == EEffectID::WishSkill && !CachedWishRowName.IsNone())
		return CachedWishRowName;
	if (EffectID == EEffectID::Evolution && !CachedEvoRowName.IsNone())
		return CachedEvoRowName;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
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

void UElfBattleController::ConsumePendingItem()
{
	if (PendingItemRowName.IsNone()) return;

	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return;

	FItemData ItemData;
	if (!GI->GetItemData(PendingItemRowName, ItemData)) return;

	int32* Remain = ItemRemainingUses.Find(PendingItemRowName);
	int32 Uses = Remain ? *Remain : ItemData.MaxBattleUses;
	if (Uses <= 0) return;

	Uses--;
	ItemRemainingUses.Add(PendingItemRowName, Uses);
	OnItemUsed.Broadcast(PendingItemRowName, Uses);

	PendingItemRowName = NAME_None;
}

void UElfBattleController::CancelWish()
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
			UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;
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

void UElfBattleController::RefundItem(FName ItemRowName)
{
	int32* Remain = ItemRemainingUses.Find(ItemRowName);
	if (Remain)
	{
		(*Remain)++;
		OnItemUsed.Broadcast(ItemRowName, *Remain);
	}
}

void UElfBattleController::BroadcastHP()
{
	if (!BattleModel) return;

	FElfCreatureInstance* PlayerCreature = BattleModel->PlayerSide.GetActiveCreature();
	FElfCalculatedStats* PlayerStats = BattleModel->PlayerSide.GetActiveStats();
	if (PlayerCreature && PlayerStats)
	{
		PlayerCreature->CurrentHP = FMath::Max<int32>(0, PlayerCreature->CurrentHP);
		PlayerCreature->CurrentEnergy = FMath::Clamp<int32>(PlayerCreature->CurrentEnergy, 0, 10);
		OnSelfCreatureHPChanged.Broadcast(PlayerCreature->CurrentHP, PlayerStats->MaxHP);
		OnSelfCreatureEnergyChanged.Broadcast(PlayerCreature->CurrentEnergy);
	}

	FElfCreatureInstance* EnemyCreature = BattleModel->EnemySide.GetActiveCreature();
	FElfCalculatedStats* EnemyStats = BattleModel->EnemySide.GetActiveStats();
	if (EnemyCreature && EnemyStats)
	{
		EnemyCreature->CurrentHP = FMath::Max<int32>(0, EnemyCreature->CurrentHP);
		EnemyCreature->CurrentEnergy = FMath::Clamp<int32>(EnemyCreature->CurrentEnergy, 0, 10);
		OnEnemyCreatureHPChanged.Broadcast(EnemyCreature->CurrentHP, EnemyStats->MaxHP);
		OnEnemyCreatureEnergyChanged.Broadcast(EnemyCreature->CurrentEnergy);
	}
}


