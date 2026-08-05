# 已完成功能

## 角色控制
- WASD 移动（镜头方向为准，忽略 Y 轴）
- 鼠标控制镜头（可调灵敏度、可反转 Y 轴、俯仰限制 ±70°）
- 空格跳跃（播放蒙太奇）
- EnhancedInput 输入系统
- InputTag 统一输入路由

## 存档系统
- USaveGame 结构（精灵队伍、仓库、位置、世界缓存等）
- PlayerController 上的 SaveGame/LoadGame 函数
- 多存档位支持
- BeginPlay 先检查存档 → 有则 LoadGame，无则 InitDefaultTeam（修复重进丢精灵）
- 战斗结束自动 SaveGame（捕捉的精灵可持久化）

## 精灵数据
- DataTable 种族数据（FElfBaseData）
- 个体实例数据（FElfCreatureInstance）
- UDataManager（UGameInstanceSubsystem，缓存野生精灵）
- PlayerState 管理队伍（0~6）和仓库，支持多人复制
- **`MaxHP = BaseHP × 2`**（`UElfStatCalculator`，提升战斗时长）
- 显示名字段统一为 `DisplayName`（`Name` 留给 JSON 行名键，7 处结构体已改名，显示名可走 JSON/CSV）
- 已配置精灵家族：暗系/风系/火系/草系/水系/暗系2/地系/普通/水系2/暗草（1~100 行号，共 27 只）
- 已知限制：属性计算目前只取种族值，等级/个体值/努力值/性格修正未参与（待实现）

## 精灵基类
- ElfCharacterBase（基础数据引用）
- ElfWorldBase（大世界：活动原点/半径、AI 控制器配置、Wild/Follow 角色）
- ElfBattleBase（占位）
- ElfAnimInstance（参数：GroundSpeed、bIsInAir、Direction）

## Spawner 生成系统
- SpawnTargetComponent 标记点位
- 支持按 Target 顺序生成 + 随机生成
- 射线落地检测（±500 单位）
- 精灵被击败/捕捉后定时刷新
- 每步移动距离限制（MaxWanderDistance）
- 高度随机偏移

## AI 行为
- AElfAIController（自动读取 SpawnOrigin/WanderRadius）
- BTT_Wild_SetTarget（随机选点：限制步长 + 不超过活动半径 + 高度变化）
- BehaviorTree 驱动随机移动

## 动画资源
- LittleDragons 龙模型/全套动画
- ST_Characters_JC 角色模型/动画

## 数据管理
- UElfGameInstance 统一管理所有 DataTable（精灵、NPC、头像、卡片）
- ElfStatCalculator 属性计算库
- ImageData、NPCData 数据表结构

## 战斗系统（MVC 框架）

### 检测与触发
- 野生精灵 BattleTrigger 碰撞检测 → 进入战斗
- NPC 碰撞检测 → 进入战斗（训练家对战）
- 玩家 PvP 碰撞检测 → 双方进入战斗
- 所有触发点延迟一帧启用，防止出生即触发
- 战斗中标记防重复触发

### UI 流程（UElfBattleManager 驱动）
- ShowIntro → VS 动画（WBP_BattleIntro）→ 精灵选择 → 就绪 → 退出动画 → EnterBattle
- PvE：玩家就绪后自动进入战斗
- 可选择跳过动画直接进战斗（SkipToBattle）
- `PlayExitAnimation` — Intro 退出动画，结束后回调 `CompleteIntro` 进入战斗

### 精灵选择（ElfBattleSelect / ElfBattleController）
- `SelectedSlotIndex` — 当前选中精灵（默认 0）
- `SelectCreature / ConfirmReady / CancelReady` — 选择/就绪/取消
- `OnCreatureSelected / OnPlayerReadyStateChanged` — 事件广播
- 长按/短按判断（Timer 方案），长按显示详情

### 战斗 MVC（UElfBattleController / UElfBattleModel）
- 初始化双方队伍数据（支持 Wild / Trainer / PvP）
- 自动计算六维属性
- HP / Energy 变更委托广播
- 三段伤害动画队列
- 获取存活精灵数量
- `SwitchToBattleCamera` — 镜头切换到固定战场

### 战斗场地（AElfBattleSceneActor）
- 远离主世界的固定场地（PlayerSpawnPoint / EnemySpawnPoint / BattleCameraActor）
- `MoveCameraToBattle / MoveCameraBackToPlayer` — 镜头切换
- `bCameraAtBattle` — 防重复切换标记

### 精灵进场
- `SpawnCreature` — 生成精灵到指定点位，胶囊体高度偏移防陷地
- `ReleaseCreature` — 移到队首 + 生成 + 从小放大动画
- `RecallCreature` — 收回当前精灵到队尾
- `PlaySpawnAnimation` — 缩放动画（0→1）
- 玩家初始精灵：`ReleaseCreature(Self, SelectedSlotIndex)`
- 野生精灵：`SpawnBattleCreatures` 自动生成
- 训练家精灵：`ReleaseCreature(Enemy, RandomIdx)` 随机首发出场
- 进入战斗满血满能量（能量上限固定 10）

### 战斗 HUD
- `ElfBattleInfo` — 精灵信息控件（HP/能量/等级/名称），分己方/敌方
- `ElfBattleSkill` — 技能控件（4 槽位），绑定能量变化自动刷新
- `OnCreatureSwitched` — 切换精灵事件
- `GetCurrentStats` — 快捷获取当前 HP/MaxHP/能量
- `ElfBattleInfo` 自动绑定 HP/能量/换精灵事件，新精灵上场自动刷新

### 提示 UI
- `WBP_BattleTips` — 局内提示：左己方/右敌方，VerticalBox 堆叠
- 新提示出现在下方，退场时 SizeBox HeightOverride lerp 到 0，下方提示自动上移
- 单条提示计时后自动消失

### 逐行动作执行 + 技能展示
- `OnExecutionTimer` 重构为逐行动作 + 定时器链（显示 1.5s / 结算 1s）
- 应对情况：显示顺序 [被应对→应对]，结算顺序 [应对→被应对]
- 事件：`OnSkillDisplayStarted(Side, SkillRowName, bIsCounter)`
- 事件：`OnCreatureSummoned(Side, CreatureRowName)`（切换入场时，入场动画前）
- 事件：`OnCreatureAbility(Side, AbilityID)`（预留，特性触发时广播）
- 事件：`OnAllSkillsDisplayed`
- 修复 `bSwiftDone` / `bActionSetupDone` 定时器状态导致的多回合卡死

### 切换精灵（完整）
- Switch 模式按键 `Slot_N → Index = N`（跳过当前在场精灵索引0）
- `OnSwitchSlotSelected` → `OnPlayerSwitchRequest` 绑定（`UFUNCTION` 缺失已修复）
- 切换作为本回合行动：WaitingForOpponent → 敌人行动 → SkillExecution
- `StartTurn` 复位输入模式到 Command
- 退场动画（1→0 缩放，0.3s）→ 0.5s 间隔 → 入场动画（0→1）

### 逃跑（ESC）
- 仅玩家操作回合（PlayerDecision）可用
- 野生 → `EndBattle(Run)`，训练家/PvP → `EndBattle(PlayerLose)`
- 按钮入口：`UElfBattleController::RequestRun()`
- 配置：`IA_Escape` + `IMC_Battle` ESC 映射 + `DA_InputConfig` Tag `Input.Escape`

### 技能系统
- `FSkillData` — 技能数据表结构（名称/类型/系别/效果列表/先制度/能量消耗/技能类）
- `UElfSkillBase / UAttackSkillBase / UDefensiveSkillBase / UStatusSkillBase` — 技能逻辑类分层
- **效果系统**：`FSkillEffect` 统一替换旧 Param1/Param2，支持 `Power / HealHPPercent / RestoreEnergy / AddBuff / AddDebuff`
- 技能**实例化**：每个精灵每个技能独立实例，支持 `UseCount`、实例状态
- 伤害公式：`最终伤害 = 技能威力 × 对应攻击力 / 对应防御力`，区分物理/魔法
- **属性克制**：`ElfTypeChart` + 可配 DataTable，公式 `(1+克制数)/(1+抵抗数)`
- **应对系统**：攻击→状态→防御→攻击 应对链，防御减伤/攻击增伤
- `GetEnergyCost / GetDescription` — 虚函数支持子类重写
- `CalculateDamage / GetHitCount / GetDamageReduction` — 攻击/防御特有接口
- `GetSkillEnergyCost / CalculateSkillPower / GetCurrentPower / GetPowerState` — UI 查询
- `EEnergyState / EPowerState` — 能量/威力显示状态枚举
- 数据表：`DT_ElfSkill`

### 回合制战斗（UElfTurnManager）
- **回合状态机**：`BattleStart → SelectCreature → PlayerDecision → WaitingForOpponent → CapturePhase → EvolutionPhase → ForcedSwitch → SkillExecution → TurnEnd`
- **行动排序**：按先制度 → 速度（相同则随机）
- **应对系统（Counter）**：攻击→状态 增伤，防御→攻击 减伤，状态→防御 增益，通过 `CounterEffects` 配置
- 技能执行管线：扣能量（Buff修正）→ 标记使用 → 应用伤害+效果 → 死亡判定 → 离场检测
- Swift 技能执行：上场时自动释放，按速度排序
- `OnActionPhaseStarted / OnActionPhaseEnded` 事件
- `OnBattlePhaseChanged` — 阶段广播供 UI 使用
- 防连续防御限制
- 技能独立实例化

### 增益减益系统（UElfBuffManager）
- `DT_BuffDef`（`FEffectData`）数据表
- `EEffectID` 枚举驱动，14 种效果
- 印记（SideBuffs）和个体增益（ActiveBuffs）
- 自动层叠、过期 tick、退场清理
- 技能能量/速度/属性/连击数修正

### 战斗输入系统
- EnhancedInput + `IMC_Battle` + `DA_InputConfig`
- Q=道具 W=捕捉 E=切换 R=技能 Space=确认 X=聚能
- 五种输入模式：Command / Item / Switch / Capture / Crafting
- 选中→确认两阶段流程（高亮 + Space）
- `OnInputModeChanged` 事件驱动 UI 面板切换

### 战斗道具系统
- `DT_Item`（`FItemData`）通用道具表
- **愿力**：替换技能0 → 愿力冲击（血脉属性），自动取消/恢复
- **首领化**：首领血脉可用，速度排序，永久保留
- 每局只能选一个，延迟消耗（回合执行时扣次数）
- 道具按钮 `UElfBattleItem` 按索引初始化

### 捕捉系统
- 精灵球 `ItemType == Capture`
- 按索引获取，翻页由 UI 切片
- 概率公式：`捕捉能力 / 捕捉难度`
- 成功→加入背包/仓库，失败→跳过己方行动
- 数量跨战斗保留，进游戏补满5个

### 迅捷（Swift）
- 技能效果 `EEffectType::Swift`
- 手动切换上场时自动检查并执行
- 按速度排序，链式 Tick 执行

### 离场（ForceSwitch）
- `ForceSwitchSelf / ForceSwitchEnemy / ForceSwitchBoth`
- 技能执行过程中触发，异步等待选精灵
- 不算死亡计数，和主动切换一样处理

### 精灵血脉属性
- `Type3` 血脉属性（不参与属性克制计算）
- `EElfType::Leader` 首领血脉 → 可进化，不可用愿力
- 愿力冲击 `UAttackSkill_WishStrike`：技能属性 = 血脉属性

### 敌方 AI（UElfBattleAI）
- 配置权重：克制攻击 / 增益回能 / 被克制防御 / 被克制换精灵 / 聚能回退
- 野怪：随机技能 → 聚能
- 训练家：AI 策略决策

### 默认技能系统
- `DefaultSkillIDs`（GameInstance 配置）
- 聚能（RestoreEnergy）通用回退技能

### 战斗结束
- 清除所有 Buff → 销毁精灵 → 切回玩家 → 自动存档
- `CaptureItemQuantities` 保留到 `GameInstance`

### UI 组件
- UUIManager 通用 UI 开/关
- `UElfBattleHUD` — WidgetSwitcher 面板切换 + `BP_OnInputModeChanged` / `BP_OnBattlePhaseChanged`
- `UElfBattleInfo` / `UElfBattleSkill` / `UElfBattleSelect`
- `UElfBattleItem` — 通用道具按钮（索引初始化，自动解析行名）
- 蓝图子控件：`WBP_BattleItem` / `WBP_BattleElfSwitch` / `WBP_ElfBattleItem`

## NPC 系统
- NPCData / ElfMemberData 数据表
- NPCCharacter（BattleTrigger + NPCDataID）
- NPCSpawner（子 SceneComponent 标记生成点）

## 特性系统（Ability）

### 数据与结构
- `FAbilityData`（`DT_Ability`）— 特性定义表：`AbilityClass` / `Trigger`(FGameplayTag) / `TriggerChance` / `HPThreshold` / `TargetElement` / `BuffRowName` / `Effects` / `TriggerDelay`
- 触发时机用 `FGameplayTag`（`ElfGameplayTags` 定义 `Battle_Trigger_*` 15 个：入场/在场/回合开始/回合结束/克制伤害/属性技能/先手/受击/场下/死亡/敌离场/回能/己离场/持增益等）
- `GameInstance` 挂 `AbilityDataTable` + `GetAbilityData()`

### 框架
- `UElfAbilityBase`（非抽象基类，可数据驱动）：`Init(AbilityID, Trigger, Effects, Delay)` + `SetContext(Model, BuffMgr, TurnMgr, BattleController)` + `CanTrigger` / `TriggerAbility`（默认遍历 Effects 执行回血/回能/加Buff/加Debuff）
- `UElfAbilityManager`（BattleManager 持有）：创建特性实例、注入上下文、三种入场触发（`TriggerEnter`/`TriggerEnterBattle`/`TriggerEnterForced`）、事件触发（`TriggerByEvent`）、延迟完成（`OnAllAbilitiesTriggered`）
- `UElfEventManager`（GameInstanceSubsystem 全局事件总线）：`BroadcastEvent(Tag, Creature)` + `OnGameplayEvent`，触发时机分发用
- 特性实例存 `FBattleSideData::AbilityInstances`，`EnterBattle` 时创建

### 入场阶段
- `ETurnPhase::EnterPhase` 精灵入场阶段（战斗开始后 UI 隐藏按钮/禁用输入）
- 流程：`BeginEnterPhase` → 入场动画 → `TriggerEnterBattle`（双方按速度先后 + 间隔触发）→ `OnAllAbilitiesTriggered` → `StartTurn`
- 双方触发间隔 = 快方特性的 `TriggerDelay`（≤0 保底 0.5s）
- 播报：`BattleController->OnCreatureAbility(Side, AbilityID)`（UI 蓝图绑定弹提示）

### 回合内触发点
- `UElfEventManager` 事件总线在 TurnManager/BuffManager/BattleManager 钩子处广播，`TriggerByEvent` 按精灵定位触发
- 已接入：`TurnStart` / `TurnEnd` / `TakeDamage`（每段）/ `DealSuperEffective`（每段一次）/ `UseElementSkill`（`CanTrigger` 匹配 `TargetElement`）/ `FirstAttack` / `OnDeath` / `SelfLeftField` / `EnemyLeftField` / `RestoreEnergy`
- `CanTrigger` 支持 UseElementSkill 属性匹配（`FElfCreatureInstance::LastSkillElement` 记录最近技能属性）
- 回合内触发不阻塞回合流程（`TriggerByEvent` 不广播 `OnAllAbilitiesTriggered`）

### 关联修改
- Buff `StatModPercent`：Value 填**整数百分比**（代码 /100），增益 `×(1+x)` / 减益 `×100/(100+x)`，同属性增益/减益**净额抵消**
- `EEffectID` 新增 `DirectDamageGain` / `DirectDamageReduce`（直接伤害乘区，`EDirectGainCondition` 条件）
- `EEffectID` 枚举末尾追加新值（避免破坏已有序列化）
- `DT_Buff` 重建（删除废弃的 `DT_BuffDef` 重定向器），`BP_GameInstance` 需挂 `BuffDataTable` / `AbilityDataTable`

### 效果与数值修正扩展
- `EEffectType` 新增：`DealDamage`（物理威力伤害，不触发受击类效果）、`DrainEnemyEnergy`（敌方失能）、`StealEnergy`（偷取：目标失N、己方得实际偷取量）
- `UElfAbilityBase` 新增虚函数：`ModifySkillPower`（技能威力增幅倍率）、`ModifyEnergyCost`（技能能耗修正）
- `FAbilityData` 新增字段：`EnergyCostCondition`（能耗条件，>=0 时仅该能耗技能触发增伤）、`bNoPrompt`（被动特性不弹提示）
- 集中式计算：`UElfTurnManager::GetAttackerDamageMultiplier`（buff威力+直接伤害乘区+攻击方特性增伤）、`GetSkillEnergyCost`（buff修正+在场精灵特性修正）
- 特殊特性 C++ 子类：`UAbility_WaterDefense`（防御技能能耗-2）

### 特性清单（已配）
| 特性 | 家族 | 效果 |
|------|------|------|
| Intimidate 威吓 | 暗系 1-3 | 入场敌方攻-60% |
| FirstStrike 先手强化 | 风系 11-12 | 先手攻击伤害+50% |
| FireBoost 烈焰双攻 | 火系 21-23 | 火系技能后双攻+10%×2层 |
| GrassHeal 回春 | 草系 31-32 | 草系技能后回10%血 |
| WaterCost 水脉节能 | 水系 41 | 水系技能后能耗-1 |
| EnergyDrain 能量虹吸 | 暗系2 51-53 | 暗系技能后敌方-2能量 |
| Retaliate 反击 | 地系 61-63 | 受击对攻击者50物理伤害 |
| LowCostPower 低耗强化 | 普通 71-73 | 能耗1技能威力+50% |
| WaterDefenseDiscount 水御 | 水系2 81-83 | 防御技能能耗-2 |
| DuskDrain 暮色汲取 | 暗草 91-93 | 回合结束偷敌方1能量 |

## GM 调试
- GM 面板进入游戏自动打开，进入战斗后本次运行不再弹（`bGMDismissed`）；`GMWidgetClass` 可在 BP_PlayerController 配置为 GMHUD 容器
- `GMReplaceElf(精灵行名, 技能行名数组)`：等级继承、个体随机；技能不足4个从可学技能补满（不重复）
- 客户端调用走 Server RPC（`Server_GMReplaceElf`），保证服务器执行并复制
- `ElfBattleController` 新增 `GetCreatureMaxHP/GetCreatureCurrentHP(Side, SlotIndex)` 供 UI 按槽位取数值

## 多人模式支持
- PlayerState 数据复制（TeamCreatures、WarehouseCreatures、AvatarID、CardID）
- Server RPC 队伍操作
- 网络模式下服务端生成精灵/NPC → 复制到客户端
