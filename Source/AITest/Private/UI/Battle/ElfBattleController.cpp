#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Data/ElfSkillData.h"
#include "Skill/Attack/AttackSkillBase.h"
#include "Game/ElfGameInstance.h"
#include "Player/ElfPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void UElfBattleController::Init(APlayerController* InOwner, EBattleType Type, AActor* Opponent)
{
	OwnerPC = InOwner;
	BattleModel = NewObject<UElfBattleModel>(this);
	BattleModel->Init(InOwner, Type, Opponent);
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

void UElfBattleController::HandleInput(const FGameplayTag& InputTag)
{
}

void UElfBattleController::UseSkill(int32 SlotIndex)
{
	OnSkillSelected.Broadcast(SlotIndex);
}

void UElfBattleController::UseDefaultSkill(int32 SlotIndex)
{
	OnDefaultSkillSelected.Broadcast(SlotIndex);
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


