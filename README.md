# 数据源

拆字字典： https://github.com/kfcd/chaizi

汉字字典：https://github.com/mapull/chinese-dictionary

上述字典已经过处理后内置于 `resource/hanzi_data.json`

古诗数据库：https://github.com/Werneror/Poetry

该文件过大，自行下载后按指引执行 merge.py 获取poetry.csv


# 构建

```
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -T host=x86 -A x64 ..
cmake --build . --config Release
```

确保你已经安装 cmake MSVC 和 python.