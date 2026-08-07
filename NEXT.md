# 特性系统 — 当前状态 + 续接说明

> 完整功能清单见 `PROGRESS.md`，详细设计见 `PLAN.md`（7.7 特性、18 精灵家族、19 技能规则、20 GM）。
> 数据表导入源：`Content/Blueprint/Data/JSON|CSV|Scripts`（JSON/CSV 可改，uasset 需重新导入生效）。

## 已实现（可直接复用）

### 框架
- `FAbilityData`（`DT_Ability`）：`AbilityClass` / `Trigger`(FGameplayTag) / `TriggerChance` / `HPThreshold` / `TargetElement` / `BuffRowName` / `EnergyCostCondition` / `TotalCostThreshold` / `bNoPrompt` / `Effects` / `TriggerDelay` / `bTeamTrigger` / `bStartWithZeroEnergy` / `bNoMagicCostOnDeath` / `bEnergyDefense` / `bPoisonExtraTick`
- `UElfAbilityBase`：基类 `TriggerAbility` 遍历 Effects（回血/回能/加Buff/加Debuff/DealDamage/DrainEnemyEnergy/StealEnergy）；数值钩子 `ModifySkillPower` / `ModifyEnergyCost` / `ModifyIncomingDamage`（防守减伤）；`CanTrigger` 支持 UseElementSkill 属性匹配 + `TotalCostThreshold` 总能耗判定；`FindOwnerCreature` 按 CreatureID 定位持有者
- `UElfAbilityManager`：创建实例、三种入场触发、事件触发 `TriggerByEvent`（同侧遍历：非团队仅持有者、团队被动同侧触发）、延迟完成、`RefreshTotalCostTraits` / `RefreshEnergyDefenseTraits`
- `UElfEventManager`：全局事件总线；15 个 `Battle.Trigger.*` 已接 11 个（入场/回合开始/回合结束/受击/克制/属性技能/先手/死亡/左右离场/回能）
- 播报：`BattleController->OnCreatureAbility(Side, AbilityID)`；`bNoPrompt=true` 的被动特性不广播（TypeResist 减伤命中时主动广播弹提示）

### 效果类型（EEffectType）
- 通用：`HealHPPercent` / `RestoreEnergy` / `AddBuff` / `AddDebuff` / `Power`(威力增幅)
- 特性专用：`DealDamage`（物理威力伤害，不触发受击类效果）/ `DrainEnemyEnergy`（敌方失能）/ `StealEnergy`（偷取：目标失N、己方得实际偷取量）

### 数值修正钩子 + 集中计算
- `UElfAbilityBase::ModifySkillPower`：威力增幅（基类按 `EnergyCostCondition` + Effects 里 `Power` 值）
- `UElfAbilityBase::ModifyEnergyCost`：能耗修正（特殊特性用 C++ 子类重写，如 `UAbility_WaterDefense`）
- `UElfAbilityBase::ModifyIncomingDamage`：防守方特性减伤倍率（`UAbility_TypeResist` 属性亲和 -40%）
- `UElfTurnManager::GetAttackerDamageMultiplier` / `GetDefenderDamageMultiplier`：攻击/防守双方伤害统一乘
- `UElfTurnManager::GetSkillEnergyCost`：能耗统一算（buff 修正 + 在场精灵特性修正）
- UI 有效威力：`UElfBattleController::CalculateSkillPower`（基础威力 × 攻方攻/魔攻增幅 × 守方防/魔防倒数 × 攻击方增幅）；`GetDefaultSkillPower` 为原始威力

### 已配特性（20 个，PLAN 18 有分配表）
| 特性 | 行名 | 家族/精灵 | 效果 |
|------|------|----------|------|
| Intimidate 威吓 | Intimidate | 暗系 1-3 | 入场敌方攻-60% |
| FirstStrike 先手强化 | FirstStrike | 风系 11-12 | 先手攻击伤害+50% |
| FireBoost 烈焰双攻 | FireBoost | 火系 21-23 | 火系技能后双攻+10%×2层 |
| GrassHeal 回春 | GrassHeal | 草系 31-32 | 草系技能后回10%血 |
| WaterCost 水脉节能 | WaterCost | 水系 41 | 水系技能后能耗-1 |
| EnergyDrain 能量虹吸 | EnergyDrain | 暗系2 51-53 | 暗系技能后敌方-2能量 |
| Retaliate 反击 | Retaliate | 地系 61-63 | 受击对攻击者50物理伤害 |
| LowCostPower 低耗强化 | LowCostPower | 普通 71-73 | 能耗1技能威力+50% |
| WaterDefenseDiscount 水御 | WaterDefenseDiscount | 水系2 81-83 | 防御技能能耗-2（C++子类） |
| DuskDrain 暮色汲取 | DuskDrain | 暗草 91-93 | 回合结束偷敌方1能量 |
| 地脉充能 | EarthCharge | 岩灵 101 | 初始能量0，己方放地系技能回3能量（团队被动） |
| 暗影吸血 | DarkLifesteal | 暗魇 111 | 入场获50%吸血 |
| 暗蚀 | DarkSteal | 幽煞 121 | 入场偷敌方2能量 |
| 低耗壁垒 | DualDefUp | 沧岩 131-132 | 总能耗<4 双防+80%（实时） |
| 属性亲和 | TypeResist | 曜岩 141-142 | 受携带技能系别攻击-40%（C++子类） |
| 光合充能 | GrassEnergy | 草系回能 151/153 | 回合结束回3能量 |
| 牺牲 | Sacrifice | 牺牲×4 161-192 | 死亡不耗魔力值 |
| 首击烈焰 | FireFirstAtk | 炎 201-202 | 入场首回合物攻+100% |
| 能量壁垒 | EnergyDefense | 衡灵 211 | 每1能量双防+10%（实时） |
| 毒疫 | ToxinOverflow | 毒系 221-223 | 在场双方中毒额外触发1次 |

### 数据表（导入源）
- `JSON/ElfBase.json`：≈52 只 / 23 家族（行号 1~223），形态比例按家族约 5:8:10
- `JSON/ElfSkill.json`：159 技能（11 属性 × 14 + 特殊，含毒系 20xxx；占位 20000/30000 已重编号 40000/40001）
- `JSON/Ability.json`：20 特性
- `JSON/TypeEffectiveness.json`：12 行完整克制表（毒克草/光、被地/暗抵抗；光暗互相克制）
- `CSV/Buff.csv`：20 buff（吸血/中毒/灼烧/能量壁垒等；`bHalveStacksOnTurnEnd` 列）
- `Scripts/`：`gen_skills.py`、`gen_elf_learnsets.py`（幂等生成脚本，改参数重跑）

### GM 调试
- 面板 `WBP_GMHUD`（容器）+ `UElfGMWidget`（替换精灵面板）；进游戏自动开、进战斗后本次不再弹
- `GMReplaceElf(精灵行名, 技能数组, SlotIndex)`：按索引替换（越界取最后一只、队伍空新增）、等级继承、个体随机、技能不足补满；Server RPC

## 续接方向（特性相关）

### 待完善
- [ ] **状态判定型触发**：`OnField` / `OnBench` / `SelfHasBuff` / `EnemyHasBuffOrDebuff` 语义待定（是"常驻判断"不是"触发一次"，需想清楚）
- [ ] `CanTrigger` 实现 `TriggerChance`（概率）/ `HPThreshold`（血量阈值）——当前仅 UseElementSkill 属性匹配 + TotalCostThreshold
- [ ] 回合内特性配 `TriggerDelay` 时当前不阻塞回合流程（只记录，动画窗口后续处理）
- [ ] 攻击/防御技能的 Effects 不生效（命中附加减益/自增益等）——想法记录在 PLAN 19 待办，确认后加 C++ 支持
- [x] 吸血效果（`EEffectID::Lifesteal` buff + `ApplyAttack` 治疗后）——已完成
- [ ] 更多特殊特性子类（如 攻击附带减益、防御自增益）
- [ ] 特性播报绑定 UI（`WBP_BattleTips` 蓝图）

### 相关系统（后续）
- [ ] 精灵属性与 等级/个体值/努力值/性格 挂钩（`ElfStatCalculator` 目前只算种族值，`MaxHP=BaseHP×2`）
- [ ] 等级解锁技能（`UnlockLevel` 全 0）
- [x] `WorldBlueprint` / `BattleBlueprint` 为 None 时回退 `Default` 行通用模型（各精灵专属蓝图仍待制作）
- [ ] PVP 回合同步（服务器仲裁，当前双端本地模拟）
- [ ] 逃跑确认、双打、道具背包、精灵球制作
- [ ] `UElfBattleController` 职责拆分第 2 步（PLAN 17）
