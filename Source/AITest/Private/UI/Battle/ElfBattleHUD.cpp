#include "UI/Battle/ElfBattleHUD.h"
#include "UI/Battle/ElfBattleController.h"

void UElfBattleHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (UElfBattleController* BC = GetBattleController())
	{
		BC->OnInputModeChanged.AddDynamic(this, &UElfBattleHUD::OnInputModeChanged);
		BC->OnBattlePhaseChanged.AddDynamic(this, &UElfBattleHUD::OnBattlePhaseChanged);
	}
}

void UElfBattleHUD::OnInputModeChanged(EBattleInputMode NewMode)
{
	BP_OnInputModeChanged(NewMode);
}

void UElfBattleHUD::OnBattlePhaseChanged(ETurnPhase NewPhase)
{
	BP_OnBattlePhaseChanged(NewPhase);
}


