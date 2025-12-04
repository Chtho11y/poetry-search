#include "restring.h"

std::optional<std::vector<HanziDataJson>> readHanziData(const std::string& filename) {
    std::vector<HanziDataJson> hanziList;
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << std::endl;
            return hanziList;
        }
        
        json j;
        file >> j;
        
        for (const auto& item : j) {
            HanziDataJson hanzi;
            
            hanzi.index = item["index"];
            hanzi.character = item["char"];
            hanzi.strokes = item["strokes"];
            hanzi.radicals = item["radicals"];
            hanzi.frequency = item["frequency"];
            
            for (const auto& py : item["pinyin"]) {
                hanzi.pinyin.push_back(py);
            }
            
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