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
- **回合状态机**：`PlayerCommand → WaitingForOpponent(PvP) / AIDecision(PvE) → Resolving → Executing → TurnEnd → Switch → BattleEnd`
- **行动排序**：按先制度 → 速度 → 随机
- 技能执行管线：扣能量 → 标记使用 → 应用伤害/效果 → 死亡判定
- 双方技能依次发动，3 秒执行窗口
- `OnActionPhaseStarted / OnActionPhaseEnded` 事件（HUD 隐藏/显示 UI）
- **防御连用限制**：同一精灵不能连续两回合使用防御
- 主动换人算一次行动，重置防御标记
- 技能同 CDO → 独立实例化（`UElfSkillBase` 子类每个精灵独立）

### 敌方 AI（UElfBattleAI）
- 配置权重：克制攻击 / 增益回能 / 被克制防御 / 被克制换精灵 / 聚能回退
- 野怪：随机技能 → 聚能
- 训练家：AI 策略决策
- 属性克制感知、能量管理

### 默认技能系统
- `DefaultSkillIDs`（GameInstance 配置）
- 默认技能不占装备位，独立存储
- 聚能（RestoreEnergy）通用回退技能

### 战斗结束
- 精灵销毁 + 镜头切回玩家 + 恢复输入模式 + 清理战场

### 摄像与输入
- 战斗时 `EnterBattleMode` 显示鼠标 + GameAndUI 模式
- `ExitBattleMode` 隐藏鼠标 + GameOnly 模式
- 镜头切换到战场后角色原地不动

### Spawner 优化
- `InitCreatureData` — 完整初始化野怪数据（等级、HP、能量、技能，按 LearnableSkills 自动装备最多 4 个）
- 胶囊体高度偏移，防止野怪陷地
- `CreatureData` 标记 `Replicated`，修复客户端数据同步问题

### PvP 修复
- 服务端传 `PlayerState` 给客户端（而非 Character），解决对手数据拿不到的问题
- `InitDefaultTeam` 在服务端 `BeginPlay` 调用，`Replicated` 自动同步

### 数据与工具
- `ElfSkillData.h / DT_ElfSkill` — 技能数据表
- `GenderDataTable / TypeDataTable` — 性别/系别图片表
- `GetElfGender / GetElfType` — 枚举值查图函数
- `SkillDataTable` — GameInstance 数据表引用

### UI 组件
- UUIManager 通用 UI 开/关
- UElfUserWidget 基类（WidgetController 注入）
- UElfPlayerInfo（EInfoSide 标记己方/敌方，显示名称/头像/队伍）
- UElfBattleHUD（HP 显示、伤害动画）
- UElfBattleIntro（VS 开场动画 + 精灵选择 + 退出动画）
- UElfBlueprintFunctionLibrary（快速获取 GameInstance）

## NPC 系统
- NPCData / ElfMemberData 数据表
- NPCCharacter（BattleTrigger + NPCDataID）
- NPCSpawner（子 SceneComponent 标记生成点）

## 多人模式支持
- PlayerState 数据复制（TeamCreatures、WarehouseCreatures、AvatarID、CardID）
- Server RPC 队伍操作
- 网络模式下服务端生成精灵/NPC → 复制到客户端
