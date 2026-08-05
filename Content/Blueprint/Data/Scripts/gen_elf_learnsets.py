# -*- coding: utf-8 -*-
# 更新 ElfBase.json 中精灵的 LearnableSkills（本系 + 普通系 + 其他系）
# - 每只精灵数量在范围内随机（固定种子可复现）：本系10~15 / 普通3~6 / 其他6~10（最终形态）
# - 初级形态依次更少
# 运行：python gen_elf_learnsets.py
import json
import os
import random

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # .../Content/Blueprint/Data
path = os.path.join(BASE, 'JSON', 'ElfBase.json')


def load_json(p):
    raw = open(p, 'rb').read()
    if raw[:2] == b'\xff\xfe':
        return json.loads(raw.decode('utf-16'))
    if raw[:3] == b'\xef\xbb\xbf':
        return json.loads(raw.decode('utf-8-sig'))
    return json.loads(raw.decode('utf-8'))


elves = load_json(path)

# 本系技能池：属性代码 10~19，每系 14 个（7攻+4状态+3防）
OWN = {}
for c in range(10, 20):
    OWN[c] = [
        f'{c}000', f'{c}101', f'{c}202', f'{c}303', f'{c}404', f'{c}205', f'{c}406',
        f'{c}007', f'{c}108', f'{c}209', f'{c}410',
        f'{c}111', f'{c}212', f'{c}213',
    ]
# 普通系(Normal)技能生成时序号从 1 起（避开已有 10000），单独修正
OWN[10] = [
    '10001', '10102', '10203', '10304', '10405', '10206', '10407',
    '10008', '10109', '10210', '10411',
    '10112', '10213', '10214',
]

# 普通系技能池
NORMAL_POOL = ['10000', '20000', '30000', '10203', '10102', '10304']

# 其他属性技能池（每属性：1费攻/2费攻/1费状态/2费防）
OTHER_POOL = {
    11: ['11101', '11202', '11108', '11212'],
    12: ['12101', '12202', '12108', '12212'],
    13: ['13101', '13202', '13108', '13212'],
    14: ['14101', '14202', '14108', '14212'],
    15: ['15101', '15202', '15108', '15212'],
    16: ['16101', '16202', '16108', '16212'],
    17: ['17101', '17202', '17108', '17212'],
    18: ['18101', '18202', '18108', '18212'],
    19: ['19101', '19202', '19108', '19212'],
}

# 形态 → (本系数范围, 普通数范围, 其他数范围)
TIER_RANGE = {
    'basic':  ((5, 7), (1, 3), (2, 4)),
    'middle': ((7, 10), (2, 4), (4, 6)),
    'final':  ((10, 15), (3, 6), (6, 10)),
}

ALL_ELEMS = [11, 12, 13, 14, 15, 16, 17, 18, 19]

# 精灵行名: (本系代码, 形态)
config = {
    '1':  (18, 'basic'),
    '2':  (18, 'middle'),
    '3':  (18, 'final'),
    '11': (16, 'basic'),
    '12': (16, 'final'),
    '21': (11, 'basic'),
    '22': (11, 'middle'),
    '23': (11, 'final'),
    '31': (13, 'basic'),
    '32': (13, 'final'),
    '41': (12, 'final'),
    '51': (18, 'basic'),
    '52': (18, 'middle'),
    '53': (18, 'final'),
    '61': (15, 'basic'),
    '62': (15, 'middle'),
    '63': (15, 'final'),
    '71': (10, 'basic'),
    '72': (10, 'middle'),
    '73': (10, 'final'),
    '81': (12, 'basic'),
    '82': (12, 'middle'),
    '83': (12, 'final'),
    '91': ([18, 13], 'basic'),
    '92': ([18, 13], 'middle'),
    '93': ([18, 13], 'final'),
}

def pick(pool, n):
    return random.sample(pool, min(n, len(pool)))

def learnset(codes, tier):
    if isinstance(codes, int):
        codes = [codes]
    # 双属性：本系技能池 = 两系合并
    own_pool = [s for c in codes for s in OWN[c]]
    own_rng, normal_rng, other_rng = TIER_RANGE[tier]
    n_own = random.randint(*own_rng)
    n_normal = random.randint(*normal_rng)
    n_other = random.randint(*other_rng)

    own = pick(own_pool, n_own)
    normal = pick(NORMAL_POOL, n_normal)

    # 其他属性：随机挑 3~6 个非本系、非普通系属性，再从它们的池里取
    candidates = [e for e in ALL_ELEMS if e not in codes]
    n_elems = random.randint(3, min(6, len(candidates)))
    other_elems = random.sample(candidates, n_elems)
    other_pool = [s for e in other_elems for s in OTHER_POOL[e]]
    others = pick(other_pool, n_other)

    return own + normal + others

def skills_json(ids):
    return [{"UnlockLevel": 0, "SkillID": s, "bNeedSkillStone": False} for s in ids]

random.seed(20260804)  # 固定种子，重复运行结果一致

for e in elves:
    if e['Name'] in config:
        code, tier = config[e['Name']]
        ids = learnset(code, tier)
        e['LearnableSkills'] = skills_json(ids)
        print(e['Name'], len(ids))

with open(path, 'w', encoding='utf-8') as f:
    json.dump(elves, f, ensure_ascii=False, indent='\t')
    f.write('\n')
