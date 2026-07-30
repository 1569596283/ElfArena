#include "UI/Battle/ElfBattleItem.h"
#include "UI/Battle/ElfBattleController.h"
#include "Data/ElfItemData.h"
#include "Game/ElfGameInstance.h"
#include "GameFramework/PlayerController.h"

void UElfBattleItem::Init(int32 InSlotIndex, EBattleItemSlotType InSlotType)
{
	ItemIndex = InSlotIndex;
	SlotType = InSlotType;
	CachedItemRowName = ResolveItemRowName();
	OnInit(InSlotIndex, InSlotType);
}

void UElfBattleItem::OnClicked()
{
	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return;

	if (SlotType == EBattleItemSlotType::Capture)
		Controller->UseCaptureItem(ItemIndex);
	else
		Controller->UseBattleItem();
}

int32 UElfBattleItem::GetRemainingUses() const
{
	FName RowName = CachedItemRowName;
	if (RowName.IsNone()) return 0;

	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return 0;

	if (SlotType == EBattleItemSlotType::Capture)
		return Controller->GetCaptureItemQuantity(RowName);

	return Controller->GetItemRemainingUses(RowName);
}

bool UElfBattleItem::IsAvailable() const
{
	FName RowName = CachedItemRowName;
	if (RowName.IsNone()) return false;

	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return false;

	if (SlotType == EBattleItemSlotType::Capture)
		return Controller->GetCaptureItemQuantity(RowName) > 0;

	return Controller->GetItemRemainingUses(RowName) > 0 && Controller->CanUseBattleItem(RowName);
}

bool UElfBattleItem::IsSelectedThisTurn() const
{
	UElfBattleController* Controller = GetBattleController();
	return Controller ? Controller->IsBattleItemUsedThisTurn() : false;
}

FName UElfBattleItem::ResolveItemRowName() const
{
	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return NAME_None;

	if (SlotType == EBattleItemSlotType::Capture)
		return Controller->GetCaptureItemAtSlot(ItemIndex);

	return Controller->GetBattleItemAtSlot(ItemIndex);
}

bool UElfBattleItem::GetItemData(FItemData& OutData) const
{
	UElfBattleController* Controller = GetBattleController();
	if (!Controller) return false;

	APlayerController* PC = Controller->GetOwnerPC();
	if (!PC) return false;

	UElfGameInstance* GI = PC->GetGameInstance<UElfGameInstance>();
	if (!GI) return false;

	return GI->GetItemData(CachedItemRowName, OutData);
}

UElfBattleController* UElfBattleItem::GetBattleController() const
{
	if (!CachedBattleController)
	{
		CachedBattleController = Cast<UElfBattleController>(WidgetController);
	}
	return CachedBattleController;
}
