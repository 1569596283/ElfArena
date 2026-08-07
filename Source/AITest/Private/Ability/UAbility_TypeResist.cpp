#include "Ability/UAbility_TypeResist.h"
#include "UI/Battle/ElfBattleController.h"
#include "UI/Battle/ElfBattleModel.h"
#include "Game/ElfGameInstance.h"
#include "Elf/ElfManager.h"

float UAbility_TypeResist::ModifyIncomingDamage(EInfoSide DefenderSide, const FSkillData& SkillData) const
{
	// 攻击无属性 → 不触发
	if (SkillData.ElementType == EElfType::None)
		return 1.0f;

	// 定位持有者（在场精灵）
	int32 OwnerSideIdx = -1, OwnerTeamIdx = -1;
	FElfCreatureInstance* Owner = FindOwnerCreature(OwnerSideIdx, OwnerTeamIdx);
	if (!Owner) return 1.0f;

	UElfGameInstance* GI = BattleController ? BattleController->GetOwnerPC()->GetGameInstance<UElfGameInstance>() : nullptr;
	if (!GI) return 1.0f;

	// 携带技能系别：遍历装备技能，跳过默认技能（如 聚能），攻击属性命中任一装备技能系别 → -40%
	for (const FName& SkillID : Owner->EquippedSkills)
	{
		if (GI->DefaultSkillIDs.Contains(SkillID)) continue;

		FSkillData Equipped;
		if (GI->GetSkillData(SkillID, Equipped) && Equipped.ElementType == SkillData.ElementType)
		{
			// 特性触发条件满足 → 弹提示（UI 绑定 OnCreatureAbility 播报）
			if (BattleController)
				BattleController->OnCreatureAbility.Broadcast(DefenderSide, AbilityID);
			return 0.6f;
		}
	}
	return 1.0f;
}
