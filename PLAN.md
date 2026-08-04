# 精灵对战系统 — 开发计划

## 1. 场景与探索

### 1.1 玩家控制

- **WASD**：控制角色在场景中移动
- **鼠标**：晃动鼠标控制镜头旋转/视角

### 1.2 精灵探索

- 精灵分为两种状态：
  - **大世界状态**：在场景中自由移动
  - **对战状态**：进入战斗后切换为对战模式
- 野生精灵在**固定区域内随机移动**
- 玩家释放的精灵在**玩家周围移动**
- 与精灵触碰后触发战斗
- 三种对战类型的触发方式：
  - **野生精灵**：玩家角色碰撞野生精灵 → 触发
  - **NPC 训练家**：与 NPC 对话 / NPC 发现玩家 → 触发（后续实现）
  - **玩家对战（PvP）**：手动邀请或匹配 → 触发（后续实现）

## 2. 战斗触发流程

### 2.1 通用触发流程

1. 触发条件满足 → 进入战斗
2. **输入切换**：禁用世界操作输入（移动/跳跃/镜头），启用对战操作输入（技能快捷键）
3. 播放开场动画（VS 界面），展示双方信息
4. **精灵选择阶段**：玩家在 VS 界面中选择首发精灵
   - 默认选中队伍第一只
   - 短按己方精灵 → 选中
   - 短按敌方精灵 → 查看详情
   - 长按 → 查看精灵详情弹窗
5. 玩家点击就绪 → 等待双方就绪
6. **退出动画**：双方就绪后播放开场关闭动画
7. 进入对战 UI 界面：
   - 我方精灵 → 屏幕右下角
   - 敌方精灵 → 屏幕右上角
8. 双方精灵出场动画（见 2.3）
9. 显示对战 HUD：双方精灵状态（HP、能量、等级、名称）及我方技能面板
10. 正式进入战斗，进入技能选择阶段

### 2.2 对战类型

### 2.2 战斗场地

在关卡远处（如 X=100000 附近）摆放一个固定战斗场景，包含：

- 平坦地面
- 环境光 / 灯光
- 双方精灵站位点
- 摄像机机位标记

**战斗流程：**

1. 触发战斗 → 玩家角色原地不动，禁用移动输入
2. 摄像机飞到战场机位（`SetViewTarget` 或直接控制摄像机位置）
3. 双方精灵模型客户端本地生成（不复制），播出场动画
4. 显示对战 HUD
5. 战斗结束 → 摄像机飞回玩家角色，恢复控制

**多人互相不影响**：每个客户端看的都是同一个场景，精灵是本地的，其他人不可见。

### 2.3 精灵出场方式

进入对战 UI 时，双方精灵在场景中出场：

- **野生精灵**：直接出现在战斗场地上（简单生成）
- **训练家精灵（NPC / 玩家）**：从精灵球/召唤动画中释放出场
  - 当前简化方案：生成精灵 Actor，从小缩放到正常大小
  - 后续可制作训练家投掷动画 + 精灵球开启动画

### 2.4 对战类型

- **野生精灵对战**：与场景中的野生精灵对战，额外增加**捕捉系统**
- **训练家对战**：与 NPC 训练家对战
- **玩家对战（PvP）**：玩家与玩家对战

### 2.3 开场动画信息展示

开场动画（VS 界面）展示对战双方信息，根据对战类型不同显示内容不同：

#### 己方（固定）
- 玩家头像
- 玩家所选卡片（如角色卡 / 训练家卡）
- 玩家名称
- 携带精灵数量

#### 敌方
**训练家对战（NPC / 玩家）：**
- 训练家名称
- 训练家头像
- 精灵数量
- 卡片（训练家卡）

**野生精灵对战：**
- 野生精灵名称
- 野生精灵头像
- 卡片（暂定统一固定卡片）
- 数量（可能多只同时上场）

---

### 2.4 战斗输入控制

进入战斗后，输入系统分为两类：

| 输入类别 | 状态 | 说明 |
|---------|------|------|
| 移动（WASD） | ❌ 禁用 | 角色在世界中的移动 |
| 镜头控制（鼠标） | ❌ 禁用 | 视角旋转 |
| 跳跃（空格） | ❌ 禁用 | 跳跃 |
| 技能快捷键（1~4） | ✅ 启用 | 选择/使用精灵技能 |
| 道具（I） | ✅ 启用 | 打开道具菜单 |
| 捕捉（C，仅野生） | ✅ 启用 | 丢出精灵球 |
| 逃跑（R） | ✅ 启用 | 脱离战斗 |
| 切换精灵（T） | ✅ 启用 | 打开队伍面板 |

实现方式：
- 使用 EnhancedInput 的 **InputMappingContext 切换**
- 世界输入映射：`WorldInputContext`（移动/镜头/跳跃）→ 进入战斗时移除
- 对战输入映射：`BattleInputContext`（技能/道具/捕捉/逃跑）→ 进入战斗时添加
- 两个 Context 互不重叠，优先级明确

## 3. 对战 UI 信息面板

开场动画结束后，双方精灵信息展示：

- 血量（HP）
- 等级
- 精灵名称
- 能量（当前值 / 最大值）

### 3.1 技能面板

我方精灵最多拥有 **4 个技能**，每个技能显示：

- 技能名称
- 消耗能量
- 技能属性（火、水、草等）
- 技能威力
- 技能类型：攻击 / 状态 / 防御

### 3.2 技能按钮状态

- 当前能量 ≥ 技能消耗能量 → 可用（可点击）
- 当前能量 < 技能消耗能量 → 不可用（置灰/禁点）

## 4. 回合制战斗流程

### 4.1 回合规则

1. 双方同时选择技能（野生对战中可选择**捕捉**替代技能）或使用道具
2. 选定后回合开始，按出手顺序依次执行

### 4.1.1 逃跑

- 对战中可选择**逃跑**脱离战斗
- 野生精灵对战：逃跑无惩罚
- 训练家对战：逃跑视为战败

### 4.1.2 道具使用

- 对战中可使用道具（恢复血量、能量等）
- **道具使用不占用技能选择**，使用道具后仍可选择释放技能或捕捉
- 一轮中可同时使用道具和技能

### 4.2 出手顺序判定

1. 比较双方所选技能的**先制度**，高者先出手
2. 先制度相同 → 比较双方**速度**，快者先出手
3. 速度也相同 → **随机**决定先后

### 4.3 应对系统（Counter）

技能类型存在克制关系：

```
攻击  ←→  防御  ←→  状态  ←→  攻击
```

- 攻击应对状态 → 伤害增加
- 状态应对防御 → 获得增益/减益增加
- 防御应对攻击 → 减少受到的伤害

**应对触发流程**：当一方 A 触发应对时，先由另一方 B 释放技能，在 B 释放过程中 A 释放技能并触发应对效果。

### 4.4 技能执行

- 伤害计算（考虑属性克制、威力、攻防数值）
- 状态效果应用（buff / debuff / 异常状态）
- 能量管理（消耗与回复）

### 4.5 死亡判定

- 每次受到伤害后判断精灵 HP 是否 ≤ 0
- **多段伤害**：承受完所有段数后再统一判定死亡
- 精灵死亡 → 立即进行**胜负结算**

### 4.6 回合结束

- 双方技能都执行完成后，若均未死亡，进入回合结束阶段
- 回合结束时触发**回合结束效果**（如持续伤害、回复等）
- 处理顺序如下（与技能释放顺序规则一致：先制度 → 速度 → 随机）：
  1. 先手方执行回合结束效果
  2. 进行**死亡判定**
  3. 若后手方仍存活，执行其回合结束效果
  4. 再次进行**死亡判定**

### 4.7 派出下一只精灵

- 回合结束阶段完成后，若有精灵死亡的一方仍有存活精灵，则进入**派出阶段**
- 死亡方选择派出下一只精灵
- 另一方等待
- 双方精灵就绪后，回到**技能选择阶段**，开始新的一回合

#### 切换精灵流程（主动切换 / 阵亡换将）

1. **收回**：当前在场精灵移到队伍末尾
2. **前置**：新选择的精灵移到队伍最前面（`MoveToFront`）
3. **释放**：在场上生成新精灵（`SpawnCreature`）

### 4.8 胜负结算

- 每方有**精灵死亡次数限制**（即可被击倒几只精灵后判负）
- 当一方累计死亡精灵数 **超过限制次数** → 战斗结束
- 超过限制的一方失败，另一方获胜
- 胜利方获得经验、物品等奖励

## 5. 捕捉系统（野生对战专属）

### 5.1 触发条件

- 仅在与野生精灵对战时可用
- 在选择技能阶段可以选择 **捕捉** 替代技能

### 5.2 捕捉规则

- 选择捕捉后视为使用技能，具有**较高优先级**
- 大多数情况下，选择捕捉后由玩家先行动：
  1. 玩家丢出精灵球
  2. 判定捕捉是否成功
     - 成功 → 被捕捉精灵视同战败
     - 失败 → 精灵开始释放技能（我方精灵本回合不释放技能）
- 捕捉判定的影响因素：野生精灵血量、状态异常、精灵球种类等

## 7. 精灵属性设定

### 7.1 基础信息

- **种类**：精灵的种类名称（如小火龙、水精灵等）
- **性别**：雄 / 雌 / 无性别
- **系别**：每个精灵拥有 1~2 个属性（火、水、草、电、地、风等）
- **性格**：影响属性修正，共多种性格，效果为：
  - 不影响任何属性
  - 或：一项属性 +10%，另一项属性 -10%

### 7.2 种族值（Base Stats）

决定精灵的六项基础能力：

| 属性 | 说明 |
|------|------|
| 生命值（HP） | 血量上限 |
| 物理攻击（ATK） | 影响物理技能伤害 |
| 魔法攻击（MATK） | 影响魔法/特殊技能伤害 |
| 物理防御（DEF） | 减少受到的物理伤害 |
| 魔法防御（MDEF） | 减少受到的魔法伤害 |
| 速度（SPD） | 影响出手顺序 |

### 7.3 个体值（IV）与努力值（EV）

- **个体值**：每项属性 0~31 随机，决定个体差异
- **努力值**：通过战斗获得，可自由分配增强属性

### 7.4 等级与经验

- 精灵通过战斗获得经验值
- 经验值达到一定值后升级
- 等级影响各项属性数值

### 7.5 进化

- 精灵可在满足条件后进化为其他精灵
- 进化条件示例：
  - 达到指定等级
  - 使用特定道具
  - *（后续可扩展）*

### 7.6 道具使用

- 在精灵背包中可使用道具：
  - **经验道具**：直接提升精灵等级
  - **进化道具**：触发进化

### 7.7 特性（Ability）

- 每个精灵拥有一个特性
- 特性是一种被动效果，在**各种时机**可能触发
- 特性效果多种多样（增伤、减伤、回复、改变属性等），复用 `FSkillEffect` / `FEffectData` 风格

#### 数据表结构（已实现）

- **`DT_Ability`（`FAbilityData`）**：特性定义表
  - `AbilityClass`（`TSubclassOf<UElfAbilityBase>`）— 默认基类，特殊特性用子类
  - `Name / Description / Icon`
  - `Trigger`（`FGameplayTag`）— 触发时机，`Battle.Trigger.*` 系列
  - `TriggerChance`（概率）、`HPThreshold`（血量阈值）、`TargetElement`（指定属性）、`BuffRowName`（指定增益）
  - `Effects`（`TArray<FSkillEffect>`）— 效果列表
  - `TriggerDelay`（秒）— 触发延迟，>0 时特性完成后等待该时长再进入下一阶段（用于播动画）

#### 触发时机（GameplayTag，已实现）

触发时机用 **`FGameplayTag`** 表达（`ElfGameplayTags` 定义 `Battle_Trigger_*` 15 个），不再是枚举：
`EnterBattle / OnField / TurnStart / TurnEnd / DealSuperEffective / UseElementSkill / FirstAttack / TakeDamage / OnBench / OnDeath / EnemyLeftField / RestoreEnergy / SelfLeftField / SelfHasBuff / EnemyHasBuffOrDebuff`

#### 效果实现方式（已实现）

- **混合模式（与技能系统一致）**：
  - `UElfAbilityBase`（UObject 基类，非抽象，可数据驱动直接使用）：`TriggerAbility()` 默认遍历 `Effects` 走通用效果
  - 通用效果复用 `FSkillEffect` / BuffManager 接口（回血/回能/加Buff/加Debuff）
  - **特殊特性才新建 C++ 子类**重写 `TriggerAbility()` / `CanTrigger()`
  - 基类持 `SetContext(BattleModel, BuffManager, TurnManager, BattleController)` 注入本局上下文

#### 效果落地规则（已实现）

- **一次性效果**：触发时立刻执行 `Effects` 列表（`HealHPPercent` 回血 / `RestoreEnergy` 回能 / `AddBuff`、`AddDebuff` 施加增益）
- **常驻效果**（"在场上就生效"）：本质 = 施放 **`Duration=-1 + bPersistent` 的 Buff**，统一走 `Effects` 里的 `AddBuff`
- **属性增减**：走 Buff 属性修正（`EElfBuffStat` + `StatModPercent` / `ModifyFlat`），**Value 填整数百分比**（`10`=+10%, `-10`=-10%）
- **直接伤害增减**：独立乘区（`DirectDamageGain`/`DirectDamageReduce`），伤害公式处额外乘算

#### 实例生命周期（已实现）

- **战斗时创建**：`UElfAbilityManager::CreateAbilityInstances()`（在 `EnterBattle`，TurnManager/BuffManager 已就绪后）创建，存 `FBattleSideData::AbilityInstances`
- 每个特性实例创建时 `Init(AbilityID, Trigger, Effects, TriggerDelay)` + `SetContext(...)` 注入本局引用
- 战斗结束随 BattleModel 销毁

#### 特性管理器（已实现）

- **`UElfAbilityManager`**（由 BattleManager 持有，与 TurnManager 平级）：
  - 创建/持有特性实例、注入上下文
  - 入场触发：`TriggerEnter(Side)` 手动切换 / `TriggerEnterBattle()` 战斗开始（按速度先后 + 间隔）/ `TriggerEnterForced(Side)` 阵亡换将
  - 事件触发：订阅全局事件总线 `UElfEventManager`，`TriggerByEvent(Tag, Creature)` 按精灵定位触发
  - 触发完成后：`OnAllAbilitiesTriggered` 委托（含 `TriggerDelay` 延迟等待）
  - 播报：`BattleController->OnCreatureAbility.Broadcast(Side, AbilityID)`

#### 全局事件总线（已实现）

- **`UElfEventManager`**（GameInstanceSubsystem）：`BroadcastEvent(Tag, Creature)` + `OnGameplayEvent` 原生多播委托
- 用途：触发时机分发（特性等 C++ 订阅），UI 播报走 BattleController 的 `OnCreatureAbility`（蓝图可绑）

#### 入场阶段（已实现）

- `ETurnPhase::EnterPhase` 精灵入场阶段：战斗开始后先进入，UI 隐藏按钮/禁用输入
- 流程：`BeginEnterPhase()` → 精灵入场动画（0.4s）→ 0.6s 后 `TriggerEnterBattle()`（双方特性按速度+间隔+延迟触发）→ `OnAllAbilitiesTriggered` → `StartTurn()` 进入玩家决策
- 双方特性触发间隔 = 快方特性的 `TriggerDelay`（≤0 时保底 0.5s）

#### 触发条件（已实现）

- 数值字段（概率 / 血量阈值等）配置在 `FAbilityData` 内，基类 `CanTrigger()` 统一判断（当前默认恒真，子类可重写）

### 7.8 最终属性计算

最终属性 = 种族值 + 个体值 + 努力值 + 性格修正

## 8. 技能学习系统

### 8.1 技能学习条件

- 每个精灵可学习多个技能，由开发者配置（可扩展）
- 学习方式：
  - **等级**：精灵达到指定等级后解锁
  - **技能石**：使用道具学习
  - *（后续可增加更多方式）*

### 8.2 技能携带

- 玩家在精灵详情 UI 中选择精灵携带的技能
- 每个精灵可携带 **0 ~ 4 个技能**

## 9. 精灵背包（队伍系统）

### 9.1 背包规则

- 玩家可携带 **0 ~ 6 只**精灵
- 背包为空时，无法触发精灵对战

### 9.2 对战前准备

- 进入对战时，所有精灵恢复满状态（HP、能量等）
- 若玩家全部精灵死亡 → 对战失败（即使未超过死亡次数限制）

### 9.3 上场流程（融入对战循环）

1. 对战开始后，进入**选择上场精灵**阶段
2. 若场上已有精灵（如替换下场）→ 等待双方选择
3. 双方选择完毕后 → 进入技能选择阶段

### 9.4 背包 UI

- 玩家可在背包界面查看当前**携带的精灵**及其状态
- **精灵仓库**与背包集成在同一界面：
  - 仓库显示玩家拥有的**所有精灵**
  - 已携带的精灵有**特殊标识**（如标记）
- 操作：
  - 仓库 → 背包：将精灵加入队伍
  - 背包 → 仓库：将精灵移出队伍

## 10. 增益减益系统

### 10.1 数据结构

**`DT_BuffDef`（`FEffectData`）** — Buff/印记定义表：

| 字段 | 类型 | 说明 |
|------|------|------|
| Name | FText | 名称 |
| Description | FText | 描述 |
| Icon | TSoftObjectPtr\<UTexture2D\> | 图标 |
| EffectID | `EEffectID`（枚举） | 效果ID，决定处理逻辑 |
| TargetType | `EBuffTargetType` | Individual=增益减益, Side=印记 |
| Duration | int32 | 持续回合（-1=无限） |
| bPersistent | bool | 退场保留 |
| TargetStat | `EElfBuffStat` | **StatModPercent/ModifyFlat** 时指定属性 |
| Value | float | 主数值（百分比/数值/比例） |
| SecondaryValue | float | 辅助数值（ModifyEnergyCostAndPower/TurnEndElementDamage） |

**`FActiveBuff`（运行时）** — 作用中的 Buff 实例：

| 字段 | 说明 |
|------|------|
| BuffDefRowName | 指向 `DT_BuffDef` 的行名 |
| EffectID | `EEffectID` 枚举值 |
| Params | `TArray<float>` 运行时参数（由配置字段转换而来） |
| StackCount | 当前层数 |
| RemainingTurns | 剩余回合（-1=无限） |
| bPersistent | 退场保留 |
| bIsBuff | true=增益, false=减益 |

**存储位置：**
- 印记 → `FBattleSideData::SideBuffs`
- 增益/减益 → `FElfCreatureInstance::ActiveBuffs`

### 10.2 技能配置

`FSkillEffect` 的 `AddBuff`/`AddDebuff` 通过 `BuffRowName` 引用 `DT_BuffDef`：
```
Effects[0] = { Type: AddBuff, BuffRowName: "Buff_AtkUp", EffectTarget: 自己, Value: 10 }
```

### 10.3 效果ID清单（`EEffectID` 枚举）

#### 印记效果

| EffectID | Value | 说明 | 钩子点 |
|----------|-------|------|--------|
| `ModifyEnergyCost` | 能耗变化（负=减少） | 技能能耗修正 | 能耗计算 |
| `TurnEndRestoreEnergy` | 回复量 | 回合结束恢复能量 | 回合结束 |
| `ModifySpeed` | 速度变化 | 速度修正 | 速度计算 |
| `ModifyEnergyCostAndPower` | Value=威力倍率, SecondaryValue=能耗变化 | 能耗+威力同时修正 | 能耗+伤害 |
| `ExtraBuffStack` | 额外层数 | 获得增益时额外层数 | 添加增益前 |
| `TurnEndDamage` | 最大生命比例(0.03=3%) | 回合结束伤害 | 回合结束 |
| `EnterDrainEnergy` | 扣除量 | 上场扣能 | 上场时 |

#### 直接伤害乘区（独立乘区）

- **`DirectDamageGain`**（直接伤害增益）：`GainCondition`=条件类型, `Value`=每单位增益倍率(0.1=+10%)
- **`DirectDamageReduce`**（直接伤害减免）：`GainCondition`=条件类型, `Value`=每单位减免倍率(0.1=-10%)
- **计算方式**：所有直接伤害增益倍率相乘 × 所有直接伤害减免倍率相乘（每项 `(1 ± Value×单位数×层数)`）
- **条件类型**（`EDirectGainCondition`）：无条件 / 对方增益数量 / 己方增益数量
- **钩子点**：伤害计算（威力乘区之后、属性克制之前）
- 典型用法："对方每有一个增益效果，伤害增加10%" → `GainCondition=对方增益数量, Value=0.1`

#### 增益/减益效果

| EffectID | 参数 | 说明 |
|----------|------|------|
| `StatModPercent` | TargetStat=属性, Value=百分比整数(10=+10%, -60=-60%) | 属性百分比修正 |
| `ModifyFlat` | TargetStat=属性, Value=数值 | 属性固定值修正 |
| `ModifySpeed` | Value=速度变化 | 速度修正 |
| `ModifyEnergyCost` | Value=能耗变化 | 能耗修正 |
| `ModifyHitCount` | Value=连击变化 | 连击数修正 |
| `DoubleHitCount` | 无参数 | 连击翻倍 |
| `FreezeHP` | Value=冻结比例(0.05=5%) | 冻结生命 |
| `TurnEndElementDamage` | Value=最大生命比例, SecondaryValue=元素类型int | 回合结束属性伤害 |
| `BlockSwitch` | 无参数 | 禁止替换 |

**`StatModPercent` 计算规则**（Value 填整数百分比，代码自动 /100，层数叠加）：
- 最终百分比 `Percent = (Value/100) × StackCount`（如 `-10 × 6 → -0.6`）
- **增益**（Percent ≥ 0）：`属性 × (1 + Percent)`（如 +40% → ×1.4）
- **减益**（Percent < 0）：`属性 × 100 / (100 + |Percent|×100)`（如 -60% → ×100/160 ≈ ×0.625）
- **净额抵消**：同一属性的增益/减益按总百分比互相抵消（威吓 -60% + 剑舞 +100% → +40%）

### 10.4 管理类

- **`UElfBuffManager`** — 独立的增益管理器，处理所有 Buff 添加/层叠/过期/钩子分发
- `TurnManager` 通过 `BuffManager->` 委托调用，职责分离

## 11. 精灵血脉属性

### 11.1 第三属性

`FElfBaseData` 新增 `Type3`（血脉属性）：
- 默认等于 `Type1`
- **不参与属性克制计算**（攻击时不考虑血脉系的加成，被攻击时不计算克制/抵抗）
- 编辑器中可为每个精灵独立设置

### 11.2 首领血脉

`EElfType` 增加 `Leader`（首领）枚举值：
- `Type3 = Leader` → 首领血脉
  - **可进化**：战斗中可使用首领化道具
  - **不可使用愿力**
- `Type3 != Leader` → 普通血脉
  - **不可进化**
  - **可使用愿力**

## 12. 战斗道具系统

### 12.1 数据表

**`DT_Item`（`FItemData`）** — 通用道具表：

| 字段 | 说明 |
|------|------|
| Name / Description / Icon | 基础信息 |
| ItemType | `EItemType`（Battle/Capture/Material/SkillBook/Evolve/General） |
| MaxBattleUses | 战斗中限用次数（仅 Battle 类型） |
| EffectID | `EEffectID` 效果ID |
| TargetRowName | 目标行名（愿力=技能ID） |
| Params | 通用数值参数 |

### 12.2 战斗道具

**只有两种战斗道具，每局只能选一个使用：**

| 道具 | EffectID | 效果 | 可用精灵 |
|------|----------|------|---------|
| 愿力 | `WishSkill` | 技能0替换为愿力冲击（血脉属性），用后恢复 | 非首领血脉 |
| 首领化 | `Evolution` | 标记待进化，回合执行时进化 | 首领血脉 |

**使用流程：**
```
点击 → PendingItemRowName 标记待定（不扣次数）
       ├─ 愿力：立即替换技能0为愿力冲击
       └─ 进化：标记 bPendingEvolution
       
取消 → 清空待定（愿力恢复技能0，进化清标记）

回合执行 → ConsumePendingItem() 消耗次数
```

### 12.3 愿力冲击

**`UAttackSkill_WishStrike`** → 继承 `UAttackSkillBase`：
- 创建时 `SkillData.ElementType` 直接设为精灵当前血脉属性
- 应对状态时额外伤害通过 `CounterEffects: [{Type: Power, Value: 150}]` 配置

**愿力取消条件：**
1. 再次点击愿力按钮 → 取消
2. 使用非技能0的技能 → 自动取消
3. 切换精灵 → 自动取消

### 12.4 UI 查询

`UElfBattleController` 提供了：

| 函数 | 返回 | 说明 |
|------|------|------|
| `CanUseBattleItem(RowName)` | bool | 血统兼容、本回合未使用过 |
| `GetItemRemainingUses(RowName)` | int32 | 剩余次数（自动处理互斥逻辑） |
| `IsBattleItemUsedThisTurn()` | bool | 本回合是否已使用道具 |
| `IsItemCompatibleWithCreature(RowName)` | bool | 仅检查血统兼容性 |
| `FindBattleItemRowName(EffectID)` | FName | 通过 EffectID 查找道具行名 |

## 13. 战斗输入系统

### 13.1 键盘快捷键

| 按键 | Command | Item | Capture | Crafting |
|------|---------|------|---------|----------|
| **1~6** | 选择技能 | 使用道具 | 广播`OnCaptureSlotSelected` | 广播`OnCraftingSlotSelected` |
| **X** | 聚能 | — | → Crafting | — |
| **Q** | → Item | — | — | — |
| **E** | → Capture | — | — | — |
| **R** | — | → Command | → Command | → Capture |

### 13.2 输入模式

`EBattleInputMode` 枚举 · `UElfBattleController::SetInputMode(NewMode)`：
- `Command` → 技能选择
- `Item` → 道具
- `Capture` → 捕捉
- `Crafting` → 精灵球制作

切换时会广播 `OnInputModeChanged`，HUD 监听后切换面板。

### 13.3 配置步骤

1. 创建 `IA_Slot1~6` / `IA_X` / `IA_Q` / `IA_E` / `IA_R` Input Actions
2. `IMC_Battle` 映射键盘按键到 Input Actions
3. `DA_InputConfig` 中每行设 InputAction + Tag（`Input.Slot1` 等）
4. `BP_PlayerController` 指定 `InputConfig` 和 `BattleInputContext`

## 14. 战斗 UI

### 14.1 HUD 面板切换

```
ElfBattleHUD（整体布局）
├─ 双方信息区（HP/能量/等级）← 始终显示
└─ WidgetSwitcher（操作面板）
   ├─ Index 0: WBP_SkillPanel  (Command)
   ├─ Index 1: WBP_ItemPanel   (Item)
   ├─ Index 2: WBP_CapturePanel (Capture)
   └─ Index 3: WBP_CraftingPanel (Crafting)
```

蓝图实现 `BP_OnInputModeChanged` 切换面板。

### 14.2 道具按钮

**`WBP_BattleItem`**（继承 `UElfBattleItem`）：

| 函数 | 说明 |
|------|------|
| `Init(RowName)` | 初始化，`OnInit` 在蓝图重写 |
| `OnClicked()` | 调用 `UseItem` + 广播 `OnBattleItemClicked` |
| `GetRemainingUses()` | 剩余次数 |
| `IsAvailable()` | 是否可用（剩余>0 && CanUseBattleItem） |
| `IsSelectedThisTurn()` | 本回合是否已选中 |

### 14.3 阶段监听

`UElfBattleHUD` 自动绑定：
- `OnInputModeChanged` → `BP_OnInputModeChanged`
- `OnBattlePhaseChanged` → `BP_OnBattlePhaseChanged`

## 15. 回合制战斗流程（完整）

### 15.1 阶段顺序

```
BattleStart → 进入战斗
SelectCreature → 选择精灵（可跳过）
PlayerDecision → 玩家决策（技能/道具/切换）
  │
  │ 双方确认后
  ▼
WaitingForOpponent → 等待对手/AI决策
EvolutionPhase → 首领化判定（速度排序，快的先进化）
ManualSwitch → 手动切换精灵
SkillExecution → 精灵行动（按速度+先制度执行技能）
TurnEnd → 回合结束效果 + Buff 过期
  │
  ▼
PlayerDecision（下一回合）
```

### 15.2 首领化

- 使用进化道具后标记 `bPendingEvolution`
- `ProcessPendingEvolutions()` 在 `SkillExecution` 前按速度排序执行
- 进化后永久改变 `CreatureRowName`，属性重算，HP 按比例缩放
- 切换精灵再上场仍是进化形态

### 15.3 关于本回合已使用的标识

- 任意道具点击/快捷键 → `bItemUsedThisTurn = true`
- 下回合 `StartTurn()` 自动重置
- 取消愿力时手动重置为 `false`

## 16. 数据表设计规范

- 所有数据表行结构后缀统一用 `Data`（`FEffectData`、`FItemData`、`FElfBaseData` 等）
- `Def` 后缀不再使用
- 类型枚举值通过 `EffectID` 识别，不依赖行名

## 17. 职责拆分

**目标**：`UElfBattleController` 当前职责混杂（UI 数据查询 / UI 事件总线 / 输入路由 / 道具捕捉状态机），需分层。

**第 1 步（已完成）**：道具/捕捉状态机迁到 `UElfTurnManager`
- 状态迁移完成：`ItemRemainingUses`、`bItemUsedThisTurn`、`PendingItemRowName`、`bCapturePending`、`PendingCaptureBallRate`、`CaptureItemQuantities`
- 逻辑迁移完成：`UseItem`、`UseCaptureItem`、`UseBattleItem`、`ConsumePendingItem`、`CancelWish`、`RefundItem`、`ResetBattleItemState`、`FindBattleItemRowName`、`CanUseBattleItem`、`GetItemRemainingUses`、`IsItemCompatibleWithCreature`、`InitCaptureItemQuantities`、`ClearCapturePending`、`IsCapturePending`、`GetCaptureBallRate`、`GetCaptureItemQuantity`
- `BattleController` 保留薄转发接口（含 Blueprint 标记，避免断蓝图引用），内部调用 TurnManager
- BattleManager 在 `EnterBattle` 用 `Controller->SetTurnManager(TurnManager)` 建立连接

**第 2 步（待办）**：拆分 Controller 的查询/事件/输入职责
- UI 数据查询（GetXxx）与事件总线保留在 Controller（薄层）
- 输入路由 `HandleInput` 可独立或保留
- 明确 Controller 不再持有战斗规则状态

## 18. 待补充…
