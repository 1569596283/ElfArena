#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElfUserWidget.generated.h"

UCLASS()
class AITEST_API UElfUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetController(UObject* InWidgetController);
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> WidgetController;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void WidgetControllerSet();
};
