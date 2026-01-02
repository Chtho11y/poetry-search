#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <ctime>

#include "database.h"
#include "cond_parser.h"
#include "executor.h"


namespace py = pybind11;

struct PyHanziInfo {
    std::string character;
    std::string traditional;
    int strokes;
    int frequency;
    std::string radicals;
    std::string structure;
    std::vector<std::string> chaizi;
    std::vector<std::string> pinyin;

    PyHanziInfo(const HanziData& hd)
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

struct PyPoetryItem {
    size_t id;
    std::string title;
    std::string author;
    std::string dynasty;
    std::string content;
    std::vector<std::string> sentences;

    PyPoetryItem(const PoetryItem& item)
        : id(item.id),
          title(item.title),
          author(item.author),
          dynasty(item.dynasty),
          content(item.content.toString()) {
        for (const auto& sentence : item.sentences) {
            sentences.push_back(sentence.toString());
        }
    }

    std::string toString(){
        std::string result;
        result += title + "\n";
        result += "["+ dynasty + "] " + author;
        result += "\n" + content + "\n";
        return result;
    }
};

struct PyQueryResult {
    std::vector<QueryResult> res;
    PoetryDatabase* db;

    PyQueryResult(std::vector<QueryResult> res, PoetryDatabase* db)
        : res(res), db(db) {}

    PyPoetryItem get(size_t index) const {
        return db->getPoetryById(res.at(index).poetry_id);
    }

    std::pair<size_t, std::vector<size_t>> get_matched_info(size_t index) const {
        return {res.at(index).poetry_id, res.at(index).match_positions};
    }

    size_t size() const {
        return res.size();
    }

    std::string toString(){
        return show(5);
    }

    std::string show(size_t lim){
        lim = std::min(lim, res.size());
        std::string result;
        for(size_t i = 0; i < lim; i++){
            auto& item = db->getPoetryById(res.at(i).poetry_id);
            result += item.sentences[res.at(i).match_positions[0]].toString();
            result += "<<" + item.title + ">>";
            result += " [" + item.dynasty + "] " + item.author;
            result += "\n";
        }
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

    PyPoetryItem get_poetry_by_id(size_t id) const {
        auto& item = db_.getPoetryById(id);
        
        return PyPoetryItem(item);
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

    PyQueryResult match(const std::string& query) {
        auto cond = parseCond(query);
        if(!cond){
            throw std::runtime_error("Failed to parse query string");
        }
        auto matcher = cond->compile();
        int tim = clock();
        Executor<ExecuteStrategy::Parallel> executor;
        auto results = executor.execute(matcher, db_.getAllPoetry());
        tim = clock() - tim;
        std::cout << "Found " << results.size() << " results in " << (tim / 1000.0) << " seconds." << std::endl;
        return PyQueryResult(results, &db_);
    }

    static size_t get_mapped_char_count() {
        return ReString::char_map.size();
    }

    static PyHanziInfo get_char_info(int index) {
        auto it = ReString::hanzi_data.find(static_cast<uint16_t>(index));
        if (it != ReString::hanzi_data.end()) {
            return PyHanziInfo(it->second);
        } else {
            throw std::runtime_error("Character index not found");
        }
    }

    static std::string parse_cond(const std::string& cond_str) {
        auto cond = parseCond(cond_str);
        if(cond){
            // return cond->toString();
            auto mather = cond->compile();
            return mather.to_string();
        }
        return "Invalid condition";
    }

    static void test() {
        
    }
};

PYBIND11_MODULE(poetry_search, m) {
    m.doc() = "Chinese poetry search library";

    py::class_<PyPoetryItem>(m, "PoetryItem")
        .def_readonly("id", &PyPoetryItem::id)
        .def_readonly("title", &PyPoetryItem::title)
        .def_readonly("author", &PyPoetryItem::author)
        .def_readonly("dynasty", &PyPoetryItem::dynasty)
        .def_readonly("content", &PyPoetryItem::content)
        .def_readonly("sentences", &PyPoetryItem::sentences)
        .def("__str__", &PyPoetryItem::toString,
             "Get string representation of the poetry item");

    py::class_<PyHanziInfo>(m, "HanziInfo")
        .def_readonly("character", &PyHanziInfo::character)
        .def_readonly("traditional", &PyHanziInfo::traditional)
        .def_readonly("strokes", &PyHanziInfo::strokes)
        .def_readonly("frequency", &PyHanziInfo::frequency)
        .def_readonly("radicals", &PyHanziInfo::radicals)
        .def_readonly("structure", &PyHanziInfo::structure)
        .def_readonly("chaizi", &PyHanziInfo::chaizi)
        .def_readonly("pinyin", &PyHanziInfo::pinyin)
        .def("__str__", &PyHanziInfo::toString,
             "Get string representation of the Hanzi information");
    
    py::class_<PyQueryResult>(m, "QueryResult")
        .def("get_poetry", &PyQueryResult::get_matched_info,
             "Get poetry details by ID", py::arg("id"))
        .def("show", &PyQueryResult::show,
             "Show the query result in the console", py::arg("lim")=100)
        .def("__len__", &PyQueryResult::size,
             "Get the number of results")
        .def("__getitem__", &PyQueryResult::get,
             "Get poetry details by index", py::arg("index"))
        .def("__str__", &PyQueryResult::toString,
             "Get string representation of the query result")
        .def("__repr__", &PyQueryResult::toString,
             "Get string representation of the query result");

    py::class_<Database>(m, "Database")
        .def(py::init<>())
        .def("load", &Database::load, "Load poetry data from CSV file",
             py::arg("filename"))
        .def("load_hanzi_info", &Database::load_hanzi_info,
             "Load hanzi information from JSON file",
             py::arg("filename"))

        .def("get_poetry", &Database::get_poetry_by_id,
             "Get poetry details by ID", py::arg("id"))
        
        
        
        .def("match", &Database::match,
             "Find sentences matching specified conditions",
             py::arg("query"))

        .def("get_poetry_count", &Database::get_poetry_count,
             "Get total number of poetry items")
        .def("estimate_memory_usage", &Database::estimate_memory_usage,
             "Estimate memory usage of the database")
        .def("get_memory_usage", &Database::get_memory_usage,
                    "Get memory usage of character mapping tables and database")
        
        .def("__len__", &Database::get_poetry_count,
             "Get total number of poetry items")
        .def("__getitem__", &Database::get_poetry_by_id,
             "Get poetry details by ID", py::arg("id"))
    
        .def_static("test", &Database::test,
             "Test the condition string tokenizer")
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