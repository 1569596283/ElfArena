# -*- coding: utf-8 -*-
# 生成 ElfSkill.json 中的"规则技能"：10 属性 × (7 攻击 + 4 状态 + 3 防御)
# - 保留已有技能（1 / 2 / 10000 / 20000 / 30000 等特殊或旧编号技能）
# - 幂等：重复运行会先删掉上次生成的技能再重新生成
# 运行：python gen_skills.py
import json
import os

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # .../Content/Blueprint/Data
path = os.path.join(BASE, 'JSON', 'ElfSkill.json')


def load_json(p):
    raw = open(p, 'rb').read()
    if raw[:2] == b'\xff\xfe':
        return json.loads(raw.decode('utf-16'))
    if raw[:3] == b'\xef\xbb\xbf':
        return json.loads(raw.decode('utf-8-sig'))
    return json.loads(raw.decode('utf-8'))


skills = load_json(path)

A = "/Script/CoreUObject.Class'/Script/AITest.AttackSkillBase'"
S = "/Script/CoreUObject.Class'/Script/AITest.StatusSkillBase'"
D = "/Script/CoreUObject.Class'/Script/AITest.DefensiveSkillBase'"

# (属性名, 中文名, 属性代码, 状态技能主属性buff, 次属性buff)
elems = [
    ('Normal', '铁拳', 10, ('AddAttack', '物攻'), ('AddDefense', '物防')),
    ('Fire', '火焰', 11, ('AddAttack', '物攻'), ('AddMagicAttack', '魔攻')),
    ('Water', '流水', 12, ('AddMagicAttack', '魔攻'), ('AddMagicDefense', '魔防')),
    ('Grass', '藤蔓', 13, ('AddDefense', '物防'), ('AddMagicDefense', '魔防')),
    ('Electric', '雷电', 14, ('AddMagicAttack', '魔攻'), ('AddSpeed', '速度')),
    ('Earth', '大地', 15, ('AddDefense', '物防'), ('AddAttack', '物攻')),
    ('Wind', '疾风', 16, ('AddSpeed', '速度'), ('AddAttack', '物攻')),
    ('Ice', '冰霜', 17, ('AddMagicAttack', '魔攻'), ('AddDefense', '物防')),
    ('Dark', '暗影', 18, ('AddAttack', '物攻'), ('AddSpeed', '速度')),
    ('Light', '圣光', 19, ('AddAttack', '物攻'), ('AddMagicDefense', '魔防')),
    ('Poison', '毒素', 20, ('AddAttack', '物攻'), ('AddSpeed', '速度')),
    ('Fighting', '格斗', 21, ('AddAttack', '物攻'), ('AddDefense', '物防')),
    ('Mechanical', '机械', 22, ('AddDefense', '物防'), ('AddMagicDefense', '魔防')),
    ('Bug', '虫', 23, ('AddSpeed', '速度'), ('AddAttack', '物攻')),
]

def power_effect(v):
    return {"Type": "Power", "Value": v, "EffectTarget": "Caster", "BuffRowName": "None"}

def buff_effect(t, v, target, row):
    return {"Type": t, "Value": v, "EffectTarget": target, "BuffRowName": row}

def make_skill(sid, cls, disp, desc, stype, dmg, elem, effects, counter, counter_effects, cost):
    return {
        "Name": str(sid),
        "SkillClass": cls,
        "DisplayName": disp,
        "Description": desc,
        "Icon": "None",
        "SkillType": stype,
        "DamageType": dmg,
        "ElementType": elem,
        "Effects": effects,
        "Counter": counter,
        "CounterEffects": counter_effects,
        "Priority": 0,
        "EnergyCost": cost,
    }

# 攻击：0~4费威力 40/60/80/100/120；后两个为带应对的版本（威力降档，应对威力+X%）
# 后缀一眼可辨攻击
att_names = ['冲击', '连击', '重击', '爆裂', '灭世', '反击', '绝杀']
att_tpl = [
    (0, 40, False, 0, 'Physical'),
    (1, 60, False, 0, 'Magical'),
    (2, 80, False, 0, 'Physical'),
    (3, 100, False, 0, 'Magical'),
    (4, 120, False, 0, 'Physical'),
    (2, 60, True, 50, 'Physical'),
    (4, 100, True, 100, 'Magical'),
]

# 状态：后缀可辨状态（增益类）
status_names = ['祝福', '鼓舞', '强化', '觉醒']
def_names = ['守护', '护盾', '神盾']

new_skills = []
gen_ids = set()

for (ename, cn, code, stA, stB) in elems:
    seq = 1 if code == 10 else 0  # Normal 避开已有 10000

    def nid(cost, s):
        return code * 1000 + cost * 100 + s

    # 攻击技能 7 个
    for i, (cost, power, isc, cval, dmg) in enumerate(att_tpl):
        sid = nid(cost, seq)
        disp = cn + att_names[i]
        desc = f"威力{power}"
        ce = []
        if isc:
            desc += f"，应对状态技能时威力+{cval}%"
            ce = [power_effect(cval)]
        new_skills.append(make_skill(sid, A, disp, desc, 'Attack', dmg, ename, [power_effect(power)], isc, ce, cost))
        gen_ids.add(str(sid))
        seq += 1

    # 状态技能 4 个（层数：0费7 / 1费10 / 2费13 / 4费20，可分摊+附加减益）
    stA_name, stA_label = stA
    stB_name, stB_label = stB
    statuses = [
        (0, [buff_effect('AddBuff', 7, 'Caster', stA_name)], f"自身{stA_label}+70%"),
        (1, [buff_effect('AddBuff', 5, 'Caster', stA_name), buff_effect('AddBuff', 5, 'Caster', stB_name)], f"自身{stA_label}+50%、{stB_label}+50%"),
        (2, [buff_effect('AddBuff', 7, 'Caster', stA_name), buff_effect('AddBuff', 6, 'Caster', stB_name)], f"自身{stA_label}+70%、{stB_label}+60%"),
        (4, [buff_effect('AddBuff', 8, 'Caster', stA_name), buff_effect('AddBuff', 6, 'Caster', stB_name), buff_effect('AddDebuff', 6, 'Opponent', 'ReduceDefense')], f"自身{stA_label}+80%、{stB_label}+60%，敌方物防-60%"),
    ]
    for i, (cost, effects, desc) in enumerate(statuses):
        sid = nid(cost, seq)
        new_skills.append(make_skill(sid, S, cn + status_names[i], desc, 'Status', 'Physical', ename, effects, False, [], cost))
        gen_ids.add(str(sid))
        seq += 1

    # 防御技能 3 个（应对减伤：1费70% / 2费80% / 2费90%）
    defenses = [(1, 70), (2, 80), (2, 90)]
    for i, (cost, red) in enumerate(defenses):
        sid = nid(cost, seq)
        new_skills.append(make_skill(sid, D, cn + def_names[i], f"减伤{red}%", 'Defense', 'Physical', ename, [], True, [power_effect(red)], cost))
        gen_ids.add(str(sid))
        seq += 1

# 幂等：移除上次生成的技能（按 ID），保留其余
skills = [s for s in skills if s['Name'] not in gen_ids]
skills.extend(new_skills)

with open(path, 'w', encoding='utf-16', newline='') as f:
    f.write(json.dumps(skills, ensure_ascii=False, indent='\t') + '\n')

print(f"total skills: {len(skills)} (generated {len(new_skills)})")
