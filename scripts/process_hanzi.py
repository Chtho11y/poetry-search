import json


def pinyin_formatter(pinyin):
    pylist = [
        ['ā', 'á', 'ǎ', 'à'],
        ['ē', 'é', 'ě', 'è'],
        ['ī', 'í', 'ǐ', 'ì'],
        ['ō', 'ó', 'ǒ', 'ò'],
        ['ū', 'ú', 'ǔ', 'ù'],
        ['ǖ', 'ǘ', 'ǚ', 'ǜ']
    ]

    ascii_py = "aeiouv"
    res = ""
    shengdiao = 0
    for i in range(len(pinyin)):
        flag = False
        for j in range(len(pylist)):
            if pinyin[i] in pylist[j]:
                res += ascii_py[j]
                shengdiao = pylist[j].index(pinyin[i]) + 1
                flag = True
                break
        if not flag:
            res += pinyin[i]
    
    if shengdiao != 0:
        res += str(shengdiao)
    return res

with open('chinese-dictionary/character/char_base.json', 'r', encoding='utf-8') as ch_file:
    s = ch_file.read()
    full_data = json.loads('[' + s + ']')

with open('chaizi/chaizi-jt.txt', 'r', encoding='utf-8') as ch_file:
    lines = ch_file.readlines()
    chaizi_data = {}
    for line in lines:
        items = line.split('\t')
        hanzi = items[0]
        chaizi = []
        for i in range(1, len(items)):
            chaizi.append("".join(items[i].strip().split(' ')))
        chaizi_data[hanzi] = chaizi

# 处理数据并保存到新文件
processed_data = []
for char_data in full_data:
    processed_char = {}
    
    # 保留除了variant之外的所有字段
    for key in char_data:
        if key != 'variant':
            processed_char[key] = char_data[key]
    
    # 转换拼音格式
    if 'pinyin' in processed_char:
        processed_pinyin = []
        for py in processed_char['pinyin']:
            processed_pinyin.append(pinyin_formatter(py))
        processed_char['pinyin'] = processed_pinyin
    
    # 添加拆字数据（如果存在）
    char = processed_char['char']
    if char in chaizi_data:
        processed_char['chaizi'] = chaizi_data[char]
    
    processed_data.append(processed_char)

# 保存到新文件
with open('hanzi_data.json', 'w', encoding='utf-8') as f:
    json.dump(processed_data, f, ensure_ascii=False, indent=2)

print(f"处理完成！共处理了 {len(processed_data)} 个汉字数据，已保存到 hanzi_data.json")