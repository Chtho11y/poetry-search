# 数据源

拆字字典： https://github.com/kfcd/chaizi

汉字字典：https://github.com/mapull/chinese-dictionary

上述字典已经过处理后内置于 `resource/hanzi_data.json`

古诗数据库：https://github.com/Werneror/Poetry

该文件过大，自行下载后按指引执行 merge.py 获取poetry.csv


# 构建

在 windows 环境下构建即可。
```
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -T host=x86 -A x64 ..
cmake --build . --config Release
```

确保你已经安装 cmake, MSVC 和 python.

构建后于 Release 文件夹下有 `poetry_search.cp312-win_amd64.pyd` 文件。

# 使用
将 pyd 文件置于目录下，运行python.

```python
from poetry_search import Database
db = Database()
db.load_hanzi_info("hanzi_data.json") 
db.load("poetry.csv")

# 检查导入情况
db.get_poetry_count()
db.get_memory_usage()

db.match("**依山尽")
```
请先导入汉字列表再导入诗歌，且不要重复导入。
导入诗歌预计花费 5-10 秒。

match 语法：

+ 一二三：匹配一句话，依次为一二三
+ 一*三：星号匹配任意字符
+ \[一二三\]：可选列表，匹配一个字，可以为一或二或三，也可填条件
+ @B/@E2: @后接汉字结构（见附表），只有字母则匹配该类别下所有比例。
+ 12：单独数字表示笔画数。
+ \$1: \$加数字表示字频，012为一级字，3为二级，4为三级，5为生僻字。
+ 条件间尽量空格分隔，部分无分隔也能正常解析。
+ \<一二三\>：匹配一句话，只出现一二三且不超过一次，也可填条件。

在可选列表中，可以嵌套列表表示多重条件：
+ \[\[zhi, 12\]\] 匹配一个十二画，拼音为 zhi 的字。
+ \[\[水\]\] 匹配一个包含水的字

附表：汉字结构

|结 构 方 式					|例 字|间 架 比 例	| 代码 |
| ---- | ---- | ---- |
|独 体 结 构		|米、不			|　方正			|D0|
|镶嵌结构		|爽			|　方正			    |D1|
|品 字 形 结 构	|晶、众			|各部分相同		|A0|
|上 下 结 构		|			| 					|B0|
| - |录、华			|　上下相等					|B1|
| - |它、花				|上小下大				|B2|
| - |基、想				|上大下小				|B3|
| 田字结构 |叕、茻			|　四部分相同			|B4|
|上 中 下 结 构		|			| 				|E0|
| - |意、翼		|上中下相等		 				|E1|
| - | 量、裹				|上中下不等				|E2|
|左 右 结 构		|			| 				    |H0|
| - |羽、联			|左右相等 					|H1|
| - |伟、搞				|左窄右宽				|H2|
| - |刚、郭				|左宽右窄				|H3|
|左 中 右 结 构		|			| 				|M0|
| - |街、掰				|左中右相等				|M1|
| - |辩、傲				|左中右不等				|M2|
|全 包 围 结 构		|圆、国		|全包围			|Q0|
|半 包 围 结 构		|			| 				|R0|
| - |匠、区		        |左包右			 		|R1|
| - |历、尾				|左上包右下				|R2|
| - |勾、句				|右上包左下				|R3|
| - |遍、廷				|左下包右上				|R4|
| - |冈、闲				|上包下					|R5|
| - |函、凶				|下包上					|R6|