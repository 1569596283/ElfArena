#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ElfEnum.h"
#include "UI/Battle/ElfPlayerInfo.h"
#include "Elf/ElfManager.h"
#include "Data/ElfSkillData.h"
#include "ElfBuffManager.generated.h"

class UElfBattleController;
class UElfBattleModel;
class UElfGameInstance;
struct FBattleSideData;

UCLASS()
class AITEST_API UElfBuffManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(UElfBattleController* InBC, UElfBattleModel* InBM);

	// 收集指定侧所有活跃 Buff（SideBuffs + ActiveBuffs）
	void CollectActiveBuffs(EInfoSide Side, TArray<const FActiveBuff*>& OutBuffs);

	// 添加 Buff/印记
	void ApplyBuffToTarget(EInfoSide TargetSide, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack = -1, int32 OverrideDuration = -1, bool bIsBuff = true);
	void ApplyBuffToSide(EInfoSide Side, FName BuffDefRowName, const FEffectData& Def, int32 OverrideStack = -1, int32 OverrideDuration = -1, bool bIsBuff = true);

	// 钩子
	int32 GetModifiedEnergyCost(EInfoSide Side, int32 BaseCost);
	int32 GetModifiedSpeed(EInfoSide Side, int32 BaseSpeed);
	int32 GetModifiedHitCount(EInfoSide Side, int32 BaseCount);
	void GetModifiedStats(EInfoSide Side, FElfCalculatedStats& InOutStats);

	void ProcessTurnEndEffects(EInfoSide Side);
	void OnCreatureEnteredField(EInfoSide Side);
	bool IsSwitchBlocked(EInfoSide Side);
	void OnBeforeAddBuff(EInfoSide Side, FActiveBuff& NewBuff);

	void TickBuffs(EInfoSide Side);

	// BuffDef 缓存
	const FEffectData* GetBuffDataCached(FName RowName) const;
	void ClearCache() { BuffDataCache.Empty(); }

protected:
	FBattleSideData* GetSide(EInfoSide Side);
	FElfCreatureInstance* GetActiveCreature(EInfoSide Side);
	FElfCalculatedStats* GetActiveStats(EInfoSide Side);
	UElfGameInstance* GetGameInstance() const;

	UPROPERTY()
	TObjectPtr<UElfBattleController> BattleController;

	UPROPERTY()
	TObjectPtr<UElfBattleModel> BattleModel;

	mutable TMap<FName, FEffectData> BuffDataCache;
};
