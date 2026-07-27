#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "Elf/ElfManager.h"
#include "ElfCharacterBase.generated.h"

UCLASS()
class AITEST_API AElfCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AElfCharacterBase();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "精灵数据")
	FDataTableRowHandle CreatureRowHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "精灵数据")
	FElfCreatureInstance CreatureData;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
