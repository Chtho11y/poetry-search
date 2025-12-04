#include "restring.h"

std::optional<std::vector<HanziData>> readHanziData(const std::string& filename) {
    std::vector<HanziData> hanziList;
    
    try {
        // 读取JSON文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << std::endl;
            return hanziList;
        }
        
        // 解析JSON
        json j;
        file >> j;
        
        // 遍历所有汉字数据
        for (const auto& item : j) {
            HanziData hanzi;
            
            // 解析必需字段
            hanzi.index = item["index"];
            hanzi.character = item["char"];
            hanzi.strokes = item["strokes"];
            hanzi.radicals = item["radicals"];
            hanzi.frequency = item["frequency"];
            
            // 解析拼音数组
            for (const auto& py : item["pinyin"]) {
                hanzi.pinyin.push_back(py);
            }
            
            // 解析可选字段
            if (item.contains("traditional")) {
                hanzi.traditional = item["traditional"];
            }
            
            if (item.contains("chaizi")) {
                for (const auto& cz : item["chaizi"]) {
                    hanzi.chaizi.push_back(cz);
                }
            }

            if (item.contains("structure")) {
                hanzi.structure = item["structure"];
            } else {
                hanzi.structure = "U0";
            }
            
            hanziList.push_back(hanzi);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    for (size_t i = 0; i < hanziList.size(); ++i) {
        hanziList[i].index = static_cast<int>(i);
    }

    return hanziList;
}