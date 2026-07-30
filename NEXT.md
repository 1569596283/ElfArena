# 战斗系统 — 当前状态 + 续接说明

## 已完成的系统

### 回合制战斗
- 双方选择技能 → 优先级+速度排序 → 技能执行 → 死亡判定 → 回合结束
- 应对系统（Counter）：攻击→状态 增伤，防御→攻击 减伤，状态→防御 增益
- 聚能（DefaultSkill）

### 增益减益系统
- `DT_BuffDef`（`FEffectData`）数据表 + `UElfBuffManager`
- 14 种 `EEffectID`：属性修正、能耗、连击、回合效果、冻结、禁止替换等
- 印记（SideBuffs）和个体增益（ActiveBuffs）
- 层叠/过期/退场清理

### 输入系统
- EnhancedInput + `IMC_Battle` + `DA_InputConfig`
- 四种输入模式：Command / Item / Switch / Capture / Crafting
- 快捷键：Q=道具 W=捕捉 E=切换 R=技能 Space=确认
- 1~6 通用 Slot 键（各模式功能不同）
- 选中→确认两阶段流程（高亮 + Space）

### 战斗道具
- `DT_Item`（`FItemData`）通用道具表
- 愿力（WishSkill）：替换技能0为愿力冲击，自动取消/恢复
- 首领化（Evolution）：首领血脉可进化，速度排序，永久保留
- `PendingItemRowName` + `ConsumePendingItem` 延迟消耗
- 每局只能用一个战斗道具

### 捕捉系统
- `ItemType == Capture` 精灵球，按索引获取
- `FElfBaseData::CaptureDifficulty` 捕捉难度
- 捕捉概率 = 球捕捉能力 / 精灵难度
- 成功 → 加入背包/仓库 → 战斗胜利
- 失败 → 跳过己方行动
- 精灵球数量跨战斗保留（GI 层），进游戏补满5个

### 精灵属性
- `Type3` 血脉属性（不参与属性克制）
- `EElfType::Leader` 首领血脉 → 可进化，不可用愿力
- 愿力冲击（`UAttackSkill_WishStrike`）：属性 = 血脉属性
- 应对状态额外伤害通过 `CounterEffects: [{Power, 150}]` 配置

### UI
- `UElfBattleHUD` + WidgetSwitcher 面板切换
- `OnInputModeChanged` / `OnBattlePhaseChanged` 事件绑定
- `UElfBattleItem` 通用道具按钮（索引初始化）
- `UElfBattleSelect` 精灵选择槽位

### 离场机制
- `ForceSwitchSelf/Enemy/Both` 技能效果
- 异步切换流程，副本地选中→Timer 继续
- 死亡计数不变，和主动切换一样处理

### 数据规范
- 数据表行结构后缀统一 `Data`（`FEffectData`、`FItemData` 等）
- 不依赖行名，通过 `EEffectID` 或 `ItemType` 识别

## 待完成

### 切换精灵
- [ ] Switch 模式 UI（WBP_SwitchPanel）显示队伍精灵
- [ ] 选中 → Space 确认切换
- [ ] 切换快捷键 E 的回调绑定
- [ ] 切换后 Swift 技能执行

### 动画
- [ ] 精灵出场/退场动画
- [ ] 技能释放动画
- [ ] 进化动画
- [ ] 捕捉投掷动画
- [ ] 伤害/回复数字动画

### 其他
- [ ] 双打支持（后续讨论）
- [ ] 道具背包系统（后续）
- [ ] 精灵球制作（Crafting 模式）
- [ ] 特性（Ability）系统
