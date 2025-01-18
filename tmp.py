# turn the 100s of defines in std_constants.zh into enums

import re

from pathlib import Path

path = Path('resources/include/std_zh/std_constants.zh')
out_path = Path('resources/include/std_zh/std_constants2.zh')

# TODO ! when done: delete std_constants2.zh, replace std_constants.zh
# TODO ! many enums in std_constants.zh need names
# TODO ! ctl+f "constants", delete/update.
# TODO ! ctl+f "will be", update.
# TODO ! ctl+f "flagset", update.
# TODO ! ex for `Game->GetCurScreen()" - let's do a shorthand, like @deprecated_getter, etc; so only one symbol.

# TODO ! this is illegal rn, assuming "AnimFlags" is changed from int -> AnimationBitflags:
    # combodata cd;
    # cd->AnimFlags = AF_FRESH|AF_CYCLE;
# Update compiler to allow bitwise amongst enum bitflags.


# TODO ! naming .... "FlagIndex" -> "Flag"? audit usages.

name_map = {
    'AF': 'AnimationBitflags',
    'AT': 'AimType',
    'BITDX': 'BlitModeBitflags',
    'BLOCKFLAG': 'BlockBitflagsInt',
    'CB': 'Button',
    'CF': 'ComboFlag',
    'CHRT': 'DMapChartedBitflags',
    'CIID': 'CurrentItemIDBitflags',
    'CMB': 'ComboSolidityBitflags',
    'D': 'DoorType',
    'DIR2': 'Dir2',
    'DMF': 'DMapBitflags',
    'DOOR': 'DoorIndex',
    'EW': 'EWeaponType',
    'EXT': 'ExtendMode',
    'FFCBF': 'FFCBitflags',
    'FFCF': 'FFCFlag',
    'FONT': 'FontType',
    'GOS': 'GameOverScreenElementIndex',
    'GOSTR': 'GameOverStringIndex',
    'HIT': 'HitIndex',
    'I': 'ItemID',
    'IMISC': 'ItemMiscBitflags',
    'IP': 'ItemPickupBitflags',
    'IPSTR': 'PickupStringBitflags',
    'ITM': 'ItemRequireBitflags',
    'L': 'BlockBitflags',
    'LI': 'LevelItemBitflags',
    'LW': 'LWeaponType',
    'MB': 'MouseButtonBitflags',
    'MOUSE': 'MouseIndex',
    'NOCARRY': 'NoCarryBitflags',
    'NORESET': 'NoResetBitflags',
    'NPC': 'NPCID',
    'NPCMF': 'EnemyMiscBitflags',
    'NPCSF': 'EnemySpawnBitflags',
    'PT': 'PolygonRenderMode',
    'RT': 'RoomType',
    'RTC': 'RealTimeClockCategory',
    'SECCMB': 'SecretComboIndex',
    'SEF': 'ScreenEnemyFlagIndex',
    'SFX': 'SoundEffect',
    'SL': 'SpriteList',
    'SP': 'SpriteID',
    'SRAM': 'StoredMemoryBitflags',
    'SUBSEL': 'MoveSubscreenSelectorBitflags',
    'SZFLAG': 'SizeOverrideBitflags',
    'TF': 'TextFormatAlign',
    'UNBLOCK': 'UnblockableBitflags',
    'VOL': 'VolumeIndex',
    'WARP': 'WarpBitflags',
    'WARPEFFECT': 'WarpEffect',
}


def normalize_comment(comment: str):
    comment = comment.strip()
    comment = f'{comment[0].upper()}{comment[1:]}'
    if not comment.endswith('.') and not comment.endswith('!'):
        comment += '.'
    return comment


content_lines = list(path.read_text().splitlines())
current_comment = ''
current_comment_lines = []
enum_top_comment = {}
enums = {}
lines_to_delete = []

for i, line in enumerate(content_lines):
    if line.startswith('//'):
        current_comment += line[2:] + '\n'
        current_comment_lines.append(i)
        continue

    if not line.lower().startswith('define'):
        current_comment = ''
        current_comment_lines = []
        continue

    result = re.search(r'[a-zA-Z\d]+_[a-zA-Z\d_]+', line)
    if not result:
        current_comment = ''
        current_comment_lines = []
        continue

    name = result.group(0)
    prefix = name.split('_')[0]

    if prefix == 'FONT' and name.endswith('_HEIGHT'):
        current_comment = ''
        current_comment_lines = []
        continue

    if name.endswith('_SIZE'):
        current_comment = ''
        current_comment_lines = []
        continue

    if prefix in [
        'DEG',
        'DIR16',
        'HP',
        'INIT',
        'MAX',
        'MIN',
        'MISC',
        'MP',
        'NUM',
        'RAD',
        'SCREEN',
        'SFW',
        'SQRT',
        'SUBSCREEN',
        'TERMINAL',
    ]:
        current_comment = ''
        continue

    # gonna delete
    if prefix in ['CSET']:
        current_comment = ''
        current_comment_lines = []
        continue

    # confusing... ignore.
    if prefix in ['NPCA', 'BIT']:
        current_comment = ''
        current_comment_lines = []
        continue

    if prefix not in enums:
        enums[prefix] = []
        if current_comment:
            enum_top_comment[prefix] = normalize_comment(current_comment)
            lines_to_delete.extend(current_comment_lines)
            current_comment = ''
            current_comment_lines = []

    value = re.search(r'=(.+);', line).group(1).strip()

    comment = None
    m = re.search(r';.*//(.+)', line)
    if m:
        comment = normalize_comment(m.group(1))
    else:
        if current_comment:
            comment = normalize_comment(current_comment)
            lines_to_delete.extend(current_comment_lines)
            current_comment = ''
            current_comment_lines = []

    enums[prefix].append((name, value, comment))
    lines_to_delete.append(i)

enum_lines = []
for enum_name, pairs in enums.items():
    if len(pairs) == 1:
        continue

    top_comment = enum_top_comment.get(enum_name, '')
    name = name_map.get(enum_name, enum_name) or enum_name

    if enum_name not in name_map:
        print(f'ERROR! missing in name map: {enum_name}')
        exit(1)

    if top_comment:
        comment_lines = [f'// {l.strip()}' for l in top_comment.splitlines()]
        enum_lines.extend(comment_lines)
    enum_lines.append(f'enum {name}')
    enum_lines.append('{')

    max_val_len = max(len(p[1]) for p in pairs)

    for name, value, comment in pairs:
        enum_lines.append(f'\t{name:30} = {value},')
        if comment:
            enum_lines[-1] += (max_val_len - len(value)) * ' ' + f' // {comment}'

    enum_lines.append('};')
    enum_lines.append('')

out_lines = []
for i, line in enumerate(content_lines):
    if i not in lines_to_delete:
        out_lines.append(line)
out_lines.append('')
out_path.write_text('\n'.join(out_lines))

# manually add to common.zh, later gotta move to correct spots.
print('\n'.join(enum_lines))
