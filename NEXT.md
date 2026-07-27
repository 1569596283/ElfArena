# 增益减益系统 — 接续说明

## 已有基础

### 数据层
- `EElfType` / `ESkillType` / `EEffectType` — `Data/ElfSkillData.h`
- `FSkillEffect` 已含 `AddBuff` / `AddDebuff` 类型，以及 `StatName` / `StackCount` / `Duration` 字段
- `FElfCreatureInstance` (`Elf/ElfManager.h`) 已有 `TArray<FActiveBuff> ActiveBuffs`
- `FActiveBuff` 结构：`StatName` / `ValuePerStack` / `StackCount` / `RemainingTurns`

### 执行层
- `UElfTurnManager::ApplyStatusEffects()` — 处理 Status 技能效果，需要在 `EEffectType::AddBuff` / `AddDebuff` 分支里写入 `ActiveBuffs`
- `UElfTurnManager::OnExecutionTimer()` — 每个行动后有 `CheckDeath()`，可以加 buff tick
- `UAttackSkillBase::CalculateInstanceDamage()` — 伤害计算入口，后续需要在这里读取 buff 修正属性

### 需注意
- `DT_ElfSkill` 因 `FSkillData` 结构变更（删除 Param1/Param2），旧配置已失效，需重新配表
- 属性克制表通过 `DT_TypeChart` + `ElfTypeChart` 读取
- 记得把 `FElfCreatureInstance` 的 `ActiveBuffs` 清空时机加上（战斗开始、全恢复等）
