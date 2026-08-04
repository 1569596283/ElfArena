# 特性系统 — 当前状态 + 续接说明

> 完整功能清单见 `PROGRESS.md`。特性框架（数据/触发/入场阶段）已完成，本文档是**下一阶段：扩展特性触发时机 + 完善效果**的续接说明。

## 已完成（可直接复用）

### 框架
- `FAbilityData`（`DT_Ability`）：特性定义表（Trigger 用 `FGameplayTag`，含 `TriggerChance/HPThreshold/TargetElement/BuffRowName/Effects/TriggerDelay`）
- `UElfAbilityBase`：基类，`TriggerAbility` 默认遍历 `Effects` 执行通用效果（回血/回能/加Buff/加Debuff），持有 `SetContext` 注入的本局引用
- `UElfAbilityManager`：创建特性实例 + 三种入场触发 + 事件触发 + 延迟完成（`OnAllAbilitiesTriggered`）
- `UElfEventManager`：全局事件总线（GameInstanceSubsystem），`BroadcastEvent(Tag, Creature)`
- 入场阶段 `ETurnPhase::EnterPhase` + 双方特性按速度/间隔/延迟触发
- 播报：`BattleController->OnCreatureAbility(Side, AbilityID)`（已广播，UI 蓝图绑 `WBP_BattleTips` 显示）

### 已配特性
- 威吓 `Intimidate`：入场使敌方物攻/魔攻 -60%（6 层 -10%），`TriggerDelay` 触发延迟已生效

## 下一阶段：拓展触发时机（核心）

当前只有 **入场触发** 接了。`UElfEventManager` 事件总线已有 15 个 `Battle.Trigger.*` Tag，但 **回合内触发点（TurnStart/TurnEnd/受击/死亡/回能等）还没在 TurnManager/BuffManager 里广播**。

### 要接的触发点（在对应钩子处调 `EventManager->BroadcastEvent(Tag, Creature)`）

| Tag | 插入位置 | 备注 |
|-----|---------|------|
| `Battle.Trigger.TurnStart` | `UElfTurnManager::StartTurn()` | 回合开始 |
| `Battle.Trigger.TurnEnd` | `UElfTurnManager::EndTurn()` | 回合结束 |
| `Battle.Trigger.TakeDamage` | `ApplyAttack` 受伤方 | 连击多次计算 |
| `Battle.Trigger.DealSuperEffective` | `ApplyAttack` 克制倍率>1 | 连击只算一次 |
| `Battle.Trigger.UseElementSkill` | `ExecuteTurnAction` 技能属性匹配 | `TargetElement` 指定 |
| `Battle.Trigger.FirstAttack` | 先手判定后 | |
| `Battle.Trigger.OnDeath` | `CheckDeath` | |
| `Battle.Trigger.EnemyLeftField` / `SelfLeftField` | 离场/死亡 | |
| `Battle.Trigger.RestoreEnergy` | 回能处（技能/Buff） | |
| `Battle.Trigger.OnField` / `OnBench` | 常驻被动，入场/离场施加 | |
| `Battle.Trigger.SelfHasBuff` / `EnemyHasBuffOrDebuff` | Buff 增删钩子（BuffManager） | 状态判定型 |

### 注意
- 回合内触发若配了 `TriggerDelay`，需等延迟完成再继续回合流程（复用 `OnAllAbilitiesTriggered` 模式）
- 部分触发是**状态判定型**（OnField/OnBench/持增益），不是"触发一次"，需想清楚触发语义（可能由 Buff 承担，或每次相关钩子广播后 `CanTrigger` 判断）
- PVP：回合顺序同步尚未实现（服务器仲裁方案已讨论，见 PLAN 17 或另行记录）

## 其他待完成

### 特性相关
- [ ] 接入回合内触发点（上表）
- [ ] 特性效果扩展（更多 `EEffectID`，或特殊特性 C++ 子类）
- [ ] `CanTrigger` 默认实现读取 `TriggerChance`/`HPThreshold`（当前恒真）
- [ ] UI 播报绑定 `WBP_BattleTips`（蓝图）

### 动画
- [ ] 技能释放动画（逐行动作显示阶段已留 1.5s 等待，可替换成等动画播完）
- [ ] 进化动画
- [ ] 捕捉投掷动画
- [ ] 伤害/回复数字动画
- [ ] 特性触发动画（`TriggerDelay` 已留窗口）

### 系统功能
- [ ] 精灵属性与等级/个体值/努力值/性格挂钩（当前 `ElfStatCalculator` 只算种族值）
- [ ] 逃跑确认弹窗（ESC 现在直接退出）
- [ ] 双打支持（后续讨论）
- [ ] 道具背包系统（后续）
- [ ] 精灵球制作（Crafting 模式）
- [ ] PVP 回合同步（服务器仲裁先后手，当前双端本地模拟）

### 架构遗留（PLAN 17 节）
- [ ] `UElfBattleController` 职责拆分第 2 步（UI 查询/事件/输入分层）
