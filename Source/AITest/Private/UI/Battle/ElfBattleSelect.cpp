#include "UI/Battle/ElfBattleSelect.h"
#include "UI/Battle/ElfBattleController.h"
#include "Engine/World.h"

void UElfBattleSelect::Init(EInfoSide InSide, int32 InSlotIndex)
{
	Side = InSide;
	SlotIndex = InSlotIndex;
	bLongPressTriggered = false;
	OnInit(InSide, InSlotIndex);
}

void UElfBattleSelect::OnPress()
{
	UWorld* World = GetWorld();
	if (!World) return;

	bLongPressTriggered = false;
	World->GetTimerManager().SetTimer(LongPressTimerHandle, this, &UElfBattleSelect::OnLongPressTimer, LongPressDuration, false);
}

void UElfBattleSelect::OnRelease()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LongPressTimerHandle);
	}

	if (!bLongPressTriggered)
	{
		HandleShortPress();
	}
}

void UElfBattleSelect::OnLongPressTimer()
{
	bLongPressTriggered = true;
	HandleLongPress();
}

void UElfBattleSelect::HandleShortPress()
{
	OnCreatureSelected.Broadcast(Side, SlotIndex);

	if (Side == EInfoSide::Self)
	{
		UElfBattleController* Controller = GetBattleController();
		if (Controller)
		{
			Controller->SelectCreature(SlotIndex);
		}
	}
}

void UElfBattleSelect::HandleLongPress()
{
	OnCreatureDetailRequested.Broadcast(Side, SlotIndex);
}


