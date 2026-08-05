#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/ElfSkillData.h"
#include "ElfManager.generated.h"

UENUM(BlueprintType)
enum class EElfSex : uint8
{
	None   UMETA(DisplayName = "无性别"),
	Male   UMETA(DisplayName = "雄性"),
	Female UMETA(DisplayName = "雌性")
};

USTRUCT(BlueprintType)
struct FActiveBuff
{
	GENERATED_BODY()

	UPROPERTY()
	FName BuffDefRowName;

	UPROPERTY()
	EEffectID EffectID = EEffectID::None;

	UPROPERTY()
	TArray<float> Params;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY()
	int32 RemainingTurns = -1;

	UPROPERTY()
	bool bPersistent = false;

	UPROPERTY()
	bool bIsBuff = true; // true=增益, false=减益

	// 是否特性buff（特性施加的 buff 打此标记，清除所有增益/减益的技能不会清除它）
	UPROPERTY()
	bool bIsTraitBuff = false;
};

USTRUCT(BlueprintType)
struct FElfCreatureInstance
{
	GENERATED_BODY()

	// 唯一标识，用于存档和查找
	UPROPERTY(VisibleAnywhere, Category = "精灵个体")
	FGuid CreatureID;

	// 对应 DataTable 中的行名
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	FName CreatureRowName;

	// 当前等级
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	int32 Level = 1;

	// 当前生命值
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	int32 CurrentHP = 0;

	// 当前能量
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	int32 CurrentEnergy = 0;

	// 当前经验值
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	int32 Exp = 0;

	// 性别
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	EElfSex Sex = EElfSex::None;

	// 性格 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	FName NatureID;

	// 是否闪光
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "精灵个体")
	bool bShiny = false;

	// 个体值 - 生命
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "个体值")
	int32 IV_HP = 0;

	// 个体值 - 物理攻击
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "个体值")
	int32 IV_ATK = 0;

	// 个体值 - 魔法攻击
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "个体值")
	int32 IV_MATK = 0;

	// 个体值 - 物理防御
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "个体值")
	int32 IV_DEF = 0;

	// 个体值 - 魔法防御
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "个体值")
	int32 IV_MDEF = 0;

	// 个体值 - 速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "个体值")
	int32 IV_SPD = 0;

	// 努力值 - 生命
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "努力值")
	int32 EV_HP = 0;

	// 努力值 - 物理攻击
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "努力值")
	int32 EV_ATK = 0;

	// 努力值 - 魔法攻击
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "努力值")
	int32 EV_MATK = 0;

	// 努力值 - 物理防御
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "努力值")
	int32 EV_DEF = 0;

	// 努力值 - 魔法防御
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "努力值")
	int32 EV_MDEF = 0;

	// 努力值 - 速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "努力值")
	int32 EV_SPD = 0;

	// 当前携带的技能 ID 列表（最多 4 个）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能")
	TArray<FName> EquippedSkills;

	UPROPERTY()
	TArray<FActiveBuff> ActiveBuffs;

	UPROPERTY()
	ESkillType LastUsedSkillType = ESkillType::Attack;

	// 战斗中最近使用技能的属性（瞬态，不入存档；供 UseElementSkill 特性匹配）
	EElfType LastSkillElement = EElfType::None;

	UPROPERTY()
	bool bWishActive = false;

	UPROPERTY()
	bool bPendingEvolution = false;

	UPROPERTY()
	FName BackupFirstSkill;

	FElfCreatureInstance()
	{
		CreatureID = FGuid::NewGuid();
	}
};

UCLASS(Blueprintable)
class AITEST_API UElfManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool GetCreatureByID(const FGuid& CreatureID, FElfCreatureInstance& OutInstance);

	UFUNCTION(BlueprintCallable, Category = "精灵管理")
	void AddCreature(const FElfCreatureInstance& Instance);

	UFUNCTION(BlueprintCallable, Category = "精灵管理")
	void RemoveCreature(const FGuid& CreatureID);

	UFUNCTION(BlueprintCallable, Category = "精灵管理")
	TArray<FElfCreatureInstance> GetAllCreatures() const;

	UFUNCTION(BlueprintCallable, Category = "精灵管理")
	void Clear();

	const TMap<FString, FElfCreatureInstance>& GetAllCreatureCache() const { return CreatureMap; }
	void RestoreCreatureCache(const TMap<FString, FElfCreatureInstance>& InCache) { CreatureMap = InCache; }

protected:
	UPROPERTY()
	TMap<FString, FElfCreatureInstance> CreatureMap;
};
