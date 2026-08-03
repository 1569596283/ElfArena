# 战斗系统 — 当前状态 + 续接说明

> 完整功能清单见 `PROGRESS.md`。本文档重点是**下一个任务：特性（Ability）系统**。

## 下一个任务：特性系统（复杂度高）

### 现有基础（已就位，可直接复用）

- `FElfBaseData::AbilityID`（`FName`）— 精灵数据表里已有特性 ID 字段，尚未使用
- `UElfBattleController::OnCreatureAbility(EInfoSide Side, FName AbilityID)` — 事件已声明，**广播时机未接**（入场时不再自动广播，等特性触发时再弹提示）
- 提示 UI `WBP_BattleTips` 已可用（左己方/右敌方），特性触发时可复用来弹特性名
- **Buff 系统是最大可复用资产**：
  - `UElfBuffManager` — 效果应用、层叠、过期、钩子分发
  - `EEffectID` 已有 14 种效果：`StatModPercent / ModifyFlat / ModifySpeed / ModifyEnergyCost / ModifyEnergyCostAndPower / TurnEndRestoreEnergy / ExtraBuffStack / TurnEndDamage / FreezeHP / TurnEndElementDamage / EnterDrainEnergy / ModifyHitCount / DoubleHitCount / BlockSwitch`
  - 触发钩子已存在：能耗计算、速度计算、伤害计算、回合结束、上场、添加增益前、连击

### 设计要点（参考 `PLAN.md` 7.7 特性）

**触发时机**（特性是常驻被动，在特定时机触发一次）：
- 进入战斗 / 上场时
- 回合开始时 / 回合结束时
- 释放技能时 / 受到伤害时
- 死亡时
- 其他自定义时机

**效果类型**：增伤、减伤、回复、改变属性等 → 大概率能复用 `EEffectID` 的效果实现

**需要决策 / 设计**：
1. 特性数据表结构（如 `DT_Ability` / `FAbilityData`）：
   - 触发时机如何表达？（枚举 `EAbilityTrigger`？）
   - 效果如何表达？（复用 `FSkillEffect` / `FEffectData` 风格？）
2. 触发点在哪里挂？（在 TurnManager / BuffManager 的哪些钩子里查特性并触发）
3. 特性触发一次还是持续？与 Buff 的关系（特性是否就是"Duration=-1 + bPersistent"的 Buff？）
4. 触发时要广播 `OnCreatureAbility` 弹提示，提示文案如何定（特性名称？触发效果描述？）
5. 是否有触发条件（概率、血量阈值等）

### 建议思路

特性 ≈ 常驻被动 Buff + 自定义触发时机。可以：
- 参考 `DT_BuffDef`（`FEffectData`）的行结构扩展一个 `FAbilityData`
- 新增一个 `UElfAbilityManager`（或扩展 `UElfBuffManager`），在 TurnManager 的对应时机调用
- 触发时调用现有 `ApplyBuffToTarget` 等效果接口 + 广播 `OnCreatureAbility`

## 其他待完成

### 动画
- [ ] 技能释放动画（逐行动作显示阶段已留 1.5s 等待，可替换成等动画播完）
- [ ] 进化动画
- [ ] 捕捉投掷动画
- [ ] 伤害/回复数字动画

### 系统功能
- [ ] 精灵属性与等级/个体值/努力值/性格挂钩（当前 `ElfStatCalculator` 只算种族值）
- [ ] 逃跑确认弹窗（ESC 现在直接退出）
- [ ] 双打支持（后续讨论）
- [ ] 道具背包系统（后续）
- [ ] 精灵球制作（Crafting 模式）
