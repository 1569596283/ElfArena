#include "UI/Battle/ElfBattleSkill.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Skill/ElfSkillBase.h"
#include "Data/ElfSkillData.h"
#include "Game/ElfGameInstance.h"
#include "Data/ElfSkillData.h"

void UElfBattleSkill::Init(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	UElfBattleController* Controller = GetBattleController();
	if (Controller)
	{
		Controller->OnSelfCreatureEnergyChanged.AddDynamic(this, &UElfBattleSkill::OnEnergyChanged);
	}

	OnInit(InSlotIndex);
}

void UElfBattleSkill::OnClicked()
{
	UElfBattleController* Controller = GetBattleController();
	if (Controller)
	{
		Controller->UseSkill(SlotIndex);
	}
}

int32 UElfBattleSkill::GetSkillEnergyCost() const
{
	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return 0;

	FElfCreatureInstance Creature = Controller->GetSelfCreature(0);
	if (!Creature.EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = Controller->GetOwnerPC() ? Controller->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature.EquippedSkills[SlotIndex], SkillData)) return 0;

	return SkillData.EnergyCost;
}

int32 UElfBattleSkill::GetCurrentPower() const
{
	UElfBattleController* Controller = GetBattleController();
	return Controller ? Controller->CalculateSkillPower(SlotIndex) : 0;
}

EPowerState UElfBattleSkill::GetPowerState() const
{
	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return EPowerState::Default;

	int32 Current = Controller->CalculateSkillPower(SlotIndex);
	int32 Default = Controller->GetDefaultSkillPower(SlotIndex);

	if (Current == Default) return EPowerState::Default;
	return Current > Default ? EPowerState::Increased : EPowerState::Reduced;
}

int32 UElfBattleSkill::GetCurrentEnergyCost() const
{
	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return 0;	

	FElfCreatureInstance Creature = Controller->GetSelfCreature(0);
	if (!Creature.EquippedSkills.IsValidIndex(SlotIndex)) return 0;

	UElfGameInstance* GI = Controller->GetOwnerPC() ? Controller->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 0;

	FSkillData SkillData;
	if (!GI->GetSkillData(Creature.EquippedSkills[SlotIndex], SkillData)) return 0;

	if (SkillData.SkillClass)
	{
		UElfSkillBase* SkillObj = Cast<UElfSkillBase>(SkillData.SkillClass.GetDefaultObject());
		if (SkillObj)
		{
			return FMath::Max(0, SkillObj->GetEnergyCost(Controller, SlotIndex));
		}
	}

	return FMath::Max(0, SkillData.EnergyCost);
}

EEnergyState UElfBattleSkill::GetEnergyState() const
{
	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return EEnergyState::Default;

	int32 Current = GetCurrentEnergyCost();
	int32 Default = Controller->GetDefaultEnergyCost(SlotIndex);

	FElfCreatureInstance Creature = Controller->GetSelfCreature(0);
	int32 CurrentEnergy = Creature.CurrentEnergy;

	if (CurrentEnergy < Current) return EEnergyState::NotEnough;
	if (Current < Default) return EEnergyState::Reduced;
	if (Current > Default) return EEnergyState::Increased;
	return EEnergyState::Default;
}

void UElfBattleSkill::OnEnergyChanged(int32 NewEnergy)
{
	// Blueprint 实现更新 UI（能量不足时变红等）
	ReceiveEnergyChanged(NewEnergy);
}
