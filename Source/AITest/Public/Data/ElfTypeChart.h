#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ElfBaseData.h"
#include "ElfTypeChart.generated.h"

USTRUCT(BlueprintType)
struct FElfTypeEffectivenessRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EElfType AttackType = EElfType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EElfType> StrongAgainst;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EElfType> ResistedBy;
};

struct AITEST_API ElfTypeChart
{
	static float GetMultiplier(EElfType AttackType, EElfType DefendType1, EElfType DefendType2, UDataTable* TypeTable = nullptr)
	{
		if (AttackType == EElfType::None) return 1.0f;

		TSet<EElfType> StrongSet;
		TSet<EElfType> ResistSet;

		if (TypeTable)
		{
			FElfTypeEffectivenessRow* Row = nullptr;

			TArray<FName> RowNames = TypeTable->GetRowNames();
			for (const FName& Name : RowNames)
			{
				FElfTypeEffectivenessRow* R = TypeTable->FindRow<FElfTypeEffectivenessRow>(Name, "");
				if (R && R->AttackType == AttackType)
				{
					Row = R;
					break;
				}
			}

			if (Row)
			{
				StrongSet.Append(Row->StrongAgainst);
				ResistSet.Append(Row->ResistedBy);
			}
		}

		if (StrongSet.IsEmpty() && ResistSet.IsEmpty())
		{
			return 1.0f;
		}

		int32 Strong = 0, Resist = 0;

		if (DefendType1 != EElfType::None)
		{
			if (StrongSet.Contains(DefendType1)) Strong++;
			if (ResistSet.Contains(DefendType1)) Resist++;
		}
		if (DefendType2 != EElfType::None && DefendType2 != DefendType1)
		{
			if (StrongSet.Contains(DefendType2)) Strong++;
			if (ResistSet.Contains(DefendType2)) Resist++;
		}

		return (1.0f + Strong) / (1.0f + Resist);
	}
};
