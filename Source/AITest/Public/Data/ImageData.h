#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ImageData.generated.h"

USTRUCT(BlueprintType)
struct FImageData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "图片")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "图片")
	TSoftObjectPtr<UTexture2D> Image;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "图片")
	FText Source;
};
