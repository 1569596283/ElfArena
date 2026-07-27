#include "UI/UIManager.h"
#include "UI/ElfUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void UUIManager::Init(APlayerController* InOwner)
{
	OwnerPC = InOwner;
}

UUserWidget* UUIManager::OpenUI(TSubclassOf<UUserWidget> WidgetClass, UObject* WidgetController)
{
	if (!WidgetClass || !OwnerPC) return nullptr;

	UUserWidget* Widget = CreateWidget<UUserWidget>(OwnerPC, WidgetClass);
	if (Widget)
	{
		UElfUserWidget* ElfWidget = Cast<UElfUserWidget>(Widget);
		if (ElfWidget && WidgetController)
		{
			ElfWidget->SetWidgetController(WidgetController);
		}
		Widget->AddToViewport();
	}
	return Widget;
}

void UUIManager::CloseUI(UUserWidget* Widget)
{
	if (Widget)
	{
		Widget->RemoveFromParent();
	}
}
