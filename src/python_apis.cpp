#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <ctime>

#include "database.h"
#include "cond_parser.h"


namespace py = pybind11;

struct HanziPyInfo {
    std::string character;
    std::string traditional;
    int strokes;
    int frequency;
    std::string radicals;
    std::string structure;
    std::vector<std::string> chaizi;
    std::vector<std::string> pinyin;

    HanziPyInfo(const HanziData& hd)
        : character(hd.character.toString()),
          traditional(hd.traditional.toString()),
          strokes(hd.strokes),
          frequency(hd.frequency),
          radicals(hd.radicals.toString()),
          structure(hd.structure),
          pinyin(hd.pinyin) {
        for (const auto& cz : hd.chaizi) {
            chaizi.push_back(cz.toString());
        }
    }

    std::string toString(){
        std::string result;
        result += "Character: " + character + "\n";
        result += "Traditional: " + traditional + "\n";
        result += "Strokes: " + std::to_string(strokes) + "\n";
        result += "Pinyin: [ ";
        for (const auto& py : pinyin) {
            result += py + " ";
        }
        result += "]\n";
        result += "Frequency: " + std::to_string(frequency) + "\n";
        result += "Radicals: " + radicals + "\n";
        result += "Structure: " + structure + "\n";
        result += "Components: ";
        for (const auto& cz : chaizi) {
            result += cz + " ";
        }
        result += "\n";
        return result;
    }
};

class Database {
private:
    PoetryDatabase db_;

public:
    bool load(const std::string& filename) {
        int tim = clock();
        auto res = db_.loadFromCSV(filename);
        tim = clock() - tim;
        if(res > 0){
            std::cout << "Loaded "<< res <<" poems in " << (tim / 1000.0) << " seconds." << std::endl;
        }else{
            std::cout << "Failed to load poetry data from " << filename << std::endl;
        }
        return res;
    }

    bool load_hanzi_info(const std::string& filename) {
        int tim = clock();
        auto res = ReString::loadHanziData(filename);
        tim = clock() - tim;
        if (res) {
            std::cout << "Loaded " << ReString::hanzi_data.size() << " hanzi data in " << (tim / 1000.0) << " seconds." << std::endl;
        }else{
            std::cout << "Failed to load hanzi data from " << filename << std::endl;
        }
        return res;
    }

    std::vector<std::pair<std::string, size_t>> find_sentences_by_charset(const std::string& charset) {
        auto results = db_.findSentencesByCharSet(charset);
        std::vector<std::pair<std::string, size_t>> converted_results;
        
        for (const auto& [sentence, id] : results) {
            converted_results.emplace_back(sentence.toString(), id);
        }
        
        return converted_results;
    }

    py::dict get_poetry_by_id(size_t id) const {
        auto item = db_.getPoetryById(id);
        
        py::dict result;
        result["id"] = item.id;
        result["title"] = item.title;
        result["author"] = item.author;
        result["dynasty"] = item.dynasty;
        result["content"] = item.content.toString();
        
        // 转换句子列表
        py::list sentences;
        for (const auto& sentence : item.sentences) {
            sentences.append(sentence.toString());
        }
        result["sentences"] = sentences;
        
        return result;
    }

    size_t get_poetry_count() const {
        return db_.getAllPoetry().size();
    }

    size_t estimate_memory_usage() const {
        return db_.estimateMemoryUsage();
    }

    size_t get_memory_usage() {
        return ReString::estimateMapMemoryUse() + db_.estimateMemoryUsage();
    }

    static size_t get_mapped_char_count() {
        return ReString::char_map.size();
    }

    static HanziPyInfo get_char_info(int index) {
        auto it = ReString::hanzi_data.find(static_cast<uint16_t>(index));
        if (it != ReString::hanzi_data.end()) {
            return HanziPyInfo(it->second);
        } else {
            throw std::runtime_error("Character index not found");
        }
    }

    static std::string parse_cond(const std::string& cond_str) {
        auto cond = parseCond(cond_str);
        if(cond){
            return cond->toString();
        }
        return "Invalid condition";
    }
};

PYBIND11_MODULE(poetry_search, m) {
    m.doc() = "Chinese poetry search library";

    py::class_<HanziPyInfo>(m, "HanziInfo")
        .def_readonly("character", &HanziPyInfo::character)
        .def_readonly("traditional", &HanziPyInfo::traditional)
        .def_readonly("strokes", &HanziPyInfo::strokes)
        .def_readonly("frequency", &HanziPyInfo::frequency)
        .def_readonly("radicals", &HanziPyInfo::radicals)
        .def_readonly("structure", &HanziPyInfo::structure)
        .def_readonly("chaizi", &HanziPyInfo::chaizi)
        .def_readonly("pinyin", &HanziPyInfo::pinyin)
        .def("__str__", &HanziPyInfo::toString,
             "Get string representation of the Hanzi information");

    py::class_<Database>(m, "Database")
        .def(py::init<>())
        .def("load", &Database::load, "Load poetry data from CSV file",
             py::arg("filename"))
        .def("load_hanzi_info", &Database::load_hanzi_info,
             "Load hanzi information from JSON file",
             py::arg("filename"))
        .def("covered", &Database::find_sentences_by_charset, 
             "Find sentences that only contain specified characters",
             py::arg("charset"))
        .def("get_poetry_by_id", &Database::get_poetry_by_id,
             "Get poetry details by ID", py::arg("id"))
        .def("get_poetry_count", &Database::get_poetry_count,
             "Get total number of poetry items")
        .def("estimate_memory_usage", &Database::estimate_memory_usage,
             "Estimate memory usage of the database")
        .def("get_memory_usage", &Database::get_memory_usage,
                    "Get memory usage of character mapping tables and database")
        .def_static("get_char_info", &Database::get_char_info,
                    "Get Hanzi information by character index",
                    py::arg("index"))
        .def_static("get_mapped_char_count", &Database::get_mapped_char_count,
                    "Get number of mapped characters")
        .def_static("parse_cond", &Database::parse_cond,
                    "Parse a condition string into structured format",
                    py::arg("cond_str"));
}

/*
from poetry_search import Database
db = Database()
db.load_hanzi_info("hanzi_data.json") 
db.load("poetry.csv")
db.get_poetry_count()
db.get_memory_usage()
*/