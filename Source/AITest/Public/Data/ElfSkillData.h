#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "ElfBaseData.h"
#include "ElfSkillData.generated.h"

UENUM(BlueprintType)
enum class EPowerState : uint8
{
	Default   UMETA(DisplayName = "默认"),
	Increased UMETA(DisplayName = "增加"),
	Reduced   UMETA(DisplayName = "减少")
};

UENUM(BlueprintType)
enum class EEnergyState : uint8
{
	Default    UMETA(DisplayName = "默认"),
	NotEnough  UMETA(DisplayName = "能量不足"),
	Reduced    UMETA(DisplayName = "能量减少"),
	Increased  UMETA(DisplayName = "能量增加")
};

UENUM(BlueprintType)
enum class ESkillType : uint8
{
	Attack  UMETA(DisplayName = "攻击"),
	Defense UMETA(DisplayName = "防御"),
	Status  UMETA(DisplayName = "状态")
};

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Physical UMETA(DisplayName = "物理"),
	Magical  UMETA(DisplayName = "魔法")
};

UENUM(BlueprintType)
enum class EEffectType : uint8
{
	None           UMETA(DisplayName = "无"),
	Power          UMETA(DisplayName = "威力"),
	HealHPPercent  UMETA(DisplayName = "回复生命百分比"),
	RestoreEnergy  UMETA(DisplayName = "回复能量固定值"),
	AddBuff        UMETA(DisplayName = "添加增益"),
	AddDebuff      UMETA(DisplayName = "添加减益"),
	ForceSwitchSelf   UMETA(DisplayName = "己方离场"),
	ForceSwitchEnemy  UMETA(DisplayName = "对方离场"),
	ForceSwitchBoth   UMETA(DisplayName = "双方离场"),
	Swift             UMETA(DisplayName = "迅捷")
};

UENUM(BlueprintType)
enum class EEffectTarget : uint8
{
	Caster   UMETA(DisplayName = "自己"),
	Opponent UMETA(DisplayName = "对方")
};

UENUM(BlueprintType)
enum class EBuffTargetType : uint8
{
	Individual UMETA(DisplayName = "增益/减益"),
	Side       UMETA(DisplayName = "印记")
};

UENUM(BlueprintType)
enum class EElfBuffStat : uint8
{
	ATK  UMETA(DisplayName = "物攻"),
	MATK UMETA(DisplayName = "魔攻"),
	DEF  UMETA(DisplayName = "物防"),
	MDEF UMETA(DisplayName = "魔防"),
	SPD  UMETA(DisplayName = "速度"),
	Power UMETA(DisplayName = "威力")
};

UENUM(BlueprintType)
enum class EEffectID : uint8
{
	None                  UMETA(DisplayName = "无"),
	StatModPercent        UMETA(DisplayName = "属性百分比修正"),
	ModifyFlat            UMETA(DisplayName = "属性固定值修正"),
	ModifySpeed           UMETA(DisplayName = "速度修正"),
	ModifyEnergyCost      UMETA(DisplayName = "能耗修正"),
	ModifyEnergyCostAndPower UMETA(DisplayName = "能耗+威力修正"),
	TurnEndRestoreEnergy  UMETA(DisplayName = "回合结束恢复能量"),
	ExtraBuffStack        UMETA(DisplayName = "额外增益层数"),
	TurnEndDamage         UMETA(DisplayName = "回合结束伤害"),
	FreezeHP              UMETA(DisplayName = "冻结生命"),
	TurnEndElementDamage  UMETA(DisplayName = "回合结束属性伤害"),
	EnterDrainEnergy      UMETA(DisplayName = "上场扣能"),
	ModifyHitCount        UMETA(DisplayName = "连击数修正"),
	DoubleHitCount        UMETA(DisplayName = "连击翻倍"),
	BlockSwitch           UMETA(DisplayName = "禁止替换"),

	Evolution             UMETA(DisplayName = "超进化"),
	WishSkill             UMETA(DisplayName = "愿力获得技能")
};

USTRUCT(BlueprintType)
struct FEffectData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "名称"))
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "描述"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "图标"))
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "效果ID"))
	EEffectID EffectID = EEffectID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "目标类型"))
	EBuffTargetType TargetType = EBuffTargetType::Individual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "持续回合", ToolTip = "-1=无限"))
	int32 Duration = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "效果", meta = (DisplayName = "退场保留"))
	bool bPersistent = false;

	// 三个通用参数，根据 EffectID 决定含义：
	//
	//   StatModPercent:          TargetStat=属性,  Value=百分比(0.1=+10%)
	//   ModifyFlat:              TargetStat=属性,  Value=数值
	//   ModifySpeed:             Value=速度变化值
	//   ModifyEnergyCost:        Value=能耗变化值
	//   ModifyEnergyCostAndPower: Value=威力倍率(0.2=+20%),  SecondaryValue=能耗变化值
	//   TurnEndRestoreEnergy:    Value=回复量
	//   ExtraBuffStack:          Value=额外层数
	//   TurnEndDamage:           Value=最大生命比例(0.03=3%)
	//   FreezeHP:                Value=冻结比例(0.05=5%)
	//   TurnEndElementDamage:    Value=最大生命比例,  SecondaryValue=元素类型(int)
	//   EnterDrainEnergy:        Value=扣除能量
	//   ModifyHitCount:          Value=连击变化数
	//   DoubleHitCount:          无参数
	//   BlockSwitch:             无参数

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "参数", meta = (DisplayName = "属性", EditCondition = "EffectID == EEffectID::StatModPercent || EffectID == EEffectID::ModifyFlat"))
	EElfBuffStat TargetStat = EElfBuffStat::ATK;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "参数", meta = (DisplayName = "数值"))
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "参数", meta = (DisplayName = "辅助数值", EditCondition = "EffectID == EEffectID::ModifyEnergyCostAndPower || EffectID == EEffectID::TurnEndElementDamage"))
	float SecondaryValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FSkillEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EEffectType Type = EEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "数值", ToolTip = "威力/百分比/固定值"))
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "目标", EditCondition = "Type == EEffectType::AddBuff || Type == EEffectType::AddDebuff"))
	EEffectTarget EffectTarget = EEffectTarget::Caster;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Buff行名", EditCondition = "Type == EEffectType::AddBuff || Type == EEffectType::AddDebuff"))
	FName BuffRowName;
};

USTRUCT(BlueprintType)
struct FSkillData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "技能类"))
	TSubclassOf<class UElfSkillBase> SkillClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "名称"))
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "描述"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "图标"))
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "技能类型"))
	ESkillType SkillType = ESkillType::Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "伤害类型"))
	EDamageType DamageType = EDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "系别"))
	EElfType ElementType = EElfType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "效果列表"))
	TArray<FSkillEffect> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "应对"))
	bool Counter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "应对效果列表"))
	TArray<FSkillEffect> CounterEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "先制度", ToolTip = "范围 -2~2，越大越先出手"))
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "技能", meta = (DisplayName = "能量消耗"))
	int32 EnergyCost = 1;
};
