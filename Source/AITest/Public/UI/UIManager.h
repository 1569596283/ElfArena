#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UIManager.generated.h"

class UUserWidget;

UCLASS()
class AITEST_API UUIManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(APlayerController* InOwner);

	UFUNCTION(BlueprintCallable)
	UUserWidget* OpenUI(TSubclassOf<UUserWidget> WidgetClass, UObject* WidgetController = nullptr);

	UFUNCTION(BlueprintCallable)
	void CloseUI(UUserWidget* Widget);

protected:
	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPC;
};
