#include "Game/ElfGameModeBase.h"
#include "Elf/ElfWorldBase.h"
#include "Character/PlayerCharacter.h"
#include "Player/ElfPlayerController.h"
#include "Player/ElfPlayerState.h"

void AElfGameModeBase::StartBattle(APlayerController* Player, EBattleType Type, AActor* Opponent)
{
	if (!Player || !Opponent) return;

	AElfPlayerController* PC = Cast<AElfPlayerController>(Player);
	if (!PC || PC->IsInBattle()) return;

	switch (Type)
	{
	case EBattleType::Wild:
		StartWildBattle(PC, Cast<AElfWorldBase>(Opponent));
		break;
	case EBattleType::Trainer:
		StartTrainerBattle(PC, Opponent);
		break;
	case EBattleType::PvP:
		StartPvPBattle(PC, Opponent);
		break;
	default:
		break;
	}
}

void AElfGameModeBase::StartTrainerBattle(AElfPlayerController* PC, AActor* NPCActor)
{
	if (!PC || !NPCActor) return;

	PC->Client_EnterBattleMode(NPCActor, EBattleType::Trainer);
}

void AElfGameModeBase::StartPvPBattle(AElfPlayerController* PC, AActor* Opponent)
{
	if (!PC || !Opponent) return;

	APlayerCharacter* OtherPlayer = Cast<APlayerCharacter>(Opponent);
	if (!OtherPlayer) return;

	AElfPlayerController* OtherPC = Cast<AElfPlayerController>(OtherPlayer->GetController());
	if (OtherPC && !OtherPC->IsInBattle())
	{
		PC->Client_EnterBattleMode(OtherPC->PlayerState, EBattleType::PvP);
		OtherPC->Client_EnterBattleMode(PC->PlayerState, EBattleType::PvP);
	}
}

void AElfGameModeBase::StartWildBattle(AElfPlayerController* PC, AElfWorldBase* WildCreature)
{
	if (!PC || !WildCreature) return;

	PC->Client_EnterBattleMode(WildCreature, EBattleType::Wild);
	WildCreature->OnBattleStart();
}

