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

## 精灵数据
- DataTable 种族数据（FElfBaseData）
- 个体实例数据（FElfCreatureInstance）
- UDataManager（UGameInstanceSubsystem，缓存野生精灵）
- PlayerState 管理队伍（0~6）和仓库，支持多人复制

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
- **行动排序**：按先制度 → 速度 → 随机
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

## 多人模式支持
- PlayerState 数据复制（TeamCreatures、WarehouseCreatures、AvatarID、CardID）
- Server RPC 队伍操作
- 网络模式下服务端生成精灵/NPC → 复制到客户端
