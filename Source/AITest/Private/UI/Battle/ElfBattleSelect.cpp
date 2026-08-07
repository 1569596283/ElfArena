#include "UI/Battle/ElfBattleSelect.h"
#include "UI/Battle/ElfBattleController.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"

void UElfBattleSelect::NativeConstruct()
{
	Super::NativeConstruct();
}

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

	if (!bEnableLongPress) return; // 长按禁用：release 一律走短按（高亮+待切换）

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

	// 点击后延迟一帧把键盘焦点还给游戏视口：按钮事件处理期间立即清焦点会被按钮重新抢回，
	// 导致后续空格/回车被焦点按钮吞掉（切换确认应走游戏输入）
	if (FSlateApplication::IsInitialized())
	{
		FTimerHandle FocusTimer;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(FocusTimer, []()
			{
				if (FSlateApplication::IsInitialized())
				{
					FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
					FSlateApplication::Get().SetAllUserFocusToGameViewport();
				}
			}, 0.05f, false);
		}
	}
}

void UElfBattleSelect::OnLongPressTimer()
{
	bLongPressTriggered = true;
	HandleLongPress();
}

void UElfBattleSelect::HandleShortPress()
{
	// 短按统一广播 OnCreatureSelected：
	//   - 开场选择界面绑定它 → 选择首发
	//   - 局内切换精灵界面绑定它 → 调用 BattleController->RequestPlayerSwitch(SlotIndex) 请求切换（等同按数字键）
	OnCreatureSelected.Broadcast(Side, SlotIndex);

	if (Side == EInfoSide::Self)
	{
		UElfBattleController* Controller = GetBattleController();
		if (Controller)
		{
			Controller->SelectCreature(SlotIndex);
			// 短按 = 按数字键的效果：高亮并设为待确认切换槽位（之后由空格/确认触发切换）
			Controller->HighlightSwitchSlot(SlotIndex);
		}
	}
}

void UElfBattleSelect::HandleLongPress()
{
	OnCreatureDetailRequested.Broadcast(Side, SlotIndex);
}


