#include "UI/Battle/ElfBattleModel.h"
#include "Skill/ElfSkillBase.h"
#include "Data/ElfStatCalculator.h"
#include "Data/NPCData.h"
#include "Game/ElfGameInstance.h"
#include "Player/ElfPlayerState.h"
#include "Elf/ElfWorldBase.h"
#include "Character/NPC/NPCCharacter.h"
#include "GameFramework/PlayerController.h"

static void InitSkillInstances(FBattleSideData& Side, UObject* Outer, UElfGameInstance* GI)
{
	Side.SkillInstances.SetNum(Side.Team.Num());
	Side.DefaultSkillInstances.SetNum(Side.Team.Num());
	for (int32 i = 0; i < Side.Team.Num(); i++)
	{
		const FElfCreatureInstance& Creature = Side.Team[i];

		for (int32 j = 0; j < Creature.EquippedSkills.Num(); j++)
		{
			FSkillData SkillData;
			if (GI && GI->GetSkillData(Creature.EquippedSkills[j], SkillData) && SkillData.SkillClass)
			{
				UElfSkillBase* Instance = NewObject<UElfSkillBase>(Outer, SkillData.SkillClass);
				Instance->Init(SkillData);
				Side.SkillInstances[i].Instances.Add(Instance);
			}
		}

		if (GI)
		{
			for (const FName& DefaultID : GI->DefaultSkillIDs)
			{
				FSkillData SkillData;
				if (GI->GetSkillData(DefaultID, SkillData) && SkillData.SkillClass)
				{
					UElfSkillBase* Instance = NewObject<UElfSkillBase>(Outer, SkillData.SkillClass);
					Instance->Init(SkillData);
					Side.DefaultSkillInstances[i].Instances.Add(Instance);
				}
			}
		}
	}
}

static void FillSideFromPlayerState(FBattleSideData& Side, AElfPlayerState* PS, UElfGameInstance* GI)
{
	if (!PS) return;
	Side.Team = PS->GetTeamCreatures();
	Side.ActiveIndex = Side.Team.IsEmpty() ? -1 : 0;

	for (const FElfCreatureInstance& Creature : Side.Team)
	{
		FElfBaseData BaseData;
		if (GI && GI->GetElfBaseData(Creature.CreatureRowName, BaseData))
		{
			Side.CalculatedStats.Add(UElfStatCalculator::CalculateStats(BaseData));
		}
	}
}

void UElfBattleModel::Init(APlayerController* OwnerPC, EBattleType Type, AActor* Opponent)
{
	BattleType = Type;
	UElfGameInstance* GI = OwnerPC ? OwnerPC->GetGameInstance<UElfGameInstance>() : nullptr;

	AElfPlayerState* OwnerPS = OwnerPC ? OwnerPC->GetPlayerState<AElfPlayerState>() : nullptr;
	FillSideFromPlayerState(PlayerSide, OwnerPS, GI);
	InitSkillInstances(PlayerSide, this, GI);

	switch (Type)
	{
	case EBattleType::Wild:
	{
		AElfWorldBase* Creature = Cast<AElfWorldBase>(Opponent);
		if (Creature)
		{
			EnemySide.Team.Add(Creature->CreatureData);

			FElfBaseData BaseData;
			if (GI && GI->GetElfBaseData(Creature->CreatureData.CreatureRowName, BaseData))
			{
				EnemySide.CalculatedStats.Add(UElfStatCalculator::CalculateStats(BaseData));
			}
		}
		EnemySide.ActiveIndex = 0;
		InitSkillInstances(EnemySide, this, GI);
		break;
	}
	case EBattleType::Trainer:
	{
		ANPCCharacter* NPC = Cast<ANPCCharacter>(Opponent);
		if (NPC && GI)
		{
			FNPCData NPCData;
			if (GI->GetNPCData(NPC->NPCDataID, NPCData))
			{
				OpponentName = NPCData.Name.ToString();
				OpponentAvatarID = NPCData.AvatarID;

				for (const FName& MemberID : NPCData.TeamMembers)
				{
					FElfMemberData Member;
					if (GI->GetElfMemberData(MemberID, Member))
					{
						FElfCreatureInstance Creature;
						Creature.CreatureRowName = Member.CreatureRowName;
						Creature.Level = Member.Level;
						Creature.Sex = Member.Sex;
						Creature.NatureID = Member.NatureID;
						Creature.EquippedSkills = Member.Skills;
						EnemySide.Team.Add(Creature);

						FElfBaseData BaseData;
						if (GI->GetElfBaseData(Member.CreatureRowName, BaseData))
						{
							EnemySide.CalculatedStats.Add(UElfStatCalculator::CalculateStats(BaseData));
						}
					}
				}
			}
		}
		EnemySide.ActiveIndex = EnemySide.Team.IsEmpty() ? -1 : 0;
		InitSkillInstances(EnemySide, this, GI);
		break;
	}
	case EBattleType::PvP:
	{
		AElfPlayerState* OtherPS = Cast<AElfPlayerState>(Opponent);
		FillSideFromPlayerState(EnemySide, OtherPS, GI);
		InitSkillInstances(EnemySide, this, GI);
		if (OtherPS)
		{
			OpponentName = OtherPS->GetPlayerName();
			OpponentAvatarID = OtherPS->AvatarID;
		}
		break;
	}
	default:
		break;
	}
}
