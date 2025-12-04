#include "database.h"

size_t PoetryItem::estimateMemoryUsage() const {
    size_t total = sizeof(PoetryItem);
    total += title.capacity() + 1;
    total += content.estimateMemoryUsage();
    total += sentences.capacity() * sizeof(ReString);
    total += dynasty.capacity() + 1;
    total += author.capacity() + 1;

    for (const auto& sentence : sentences) {
        total += sentence.estimateMemoryUsage();
    }
    return total;
}

bool PoetryDatabase::loadFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::string title, dynasty_name, author, content;
        
        if (!parseCSVLine(line, title, dynasty_name, author, content)) {
            continue;
        }

        PoetryItem item;
        item.content = content;
        item.title = title;
        item.sentences = splitSentences(content);
        item.dynasty = dynasty_name;
        item.author = author;
        item.id = poetry_items_.size();
        
        poetry_items_.push_back(item);
    }
    
    return true;
}

const std::vector<PoetryItem>& PoetryDatabase::getAllPoetry() const {
    return poetry_items_;
}

size_t PoetryDatabase::estimateMemoryUsage() const {
    size_t total = 0;

    total += poetry_items_.capacity() * sizeof(PoetryItem);
    total += sizeof(poetry_items_);

    for (const auto& item : poetry_items_) {
        total += item.estimateMemoryUsage();
    }

    return total;
}

std::vector<std::pair<ReString, size_t>> 
PoetryDatabase::findSentencesByCharSet(const std::string& charset_utf8) 
{
    // step1: 把 charset 转成 code 集合
    ReString rs(charset_utf8, false); // false: 不新增新字符
    std::unordered_set<uint16_t> allowed;
    for (auto c : rs) {
        if (c != ReString::getIllegalCode())
            allowed.insert(c);
    }

    std::vector<std::pair<ReString, size_t>> result;

    // step2: 遍历所有诗句
    for (const auto& item : getAllPoetry()) {
        for (const auto& sent : item.sentences) {
            bool ok = true;
            for (auto ch : sent) {
                if (!allowed.count(ch)) { 
                    ok = false; 
                    break; 
                }
            }
            if (ok) {
                result.emplace_back(sent, item.id);
                break; // 该诗出现一次即可
            }
        }
    }

    return result;
}

PoetryItem PoetryDatabase::getPoetryById(size_t id) const {
    return poetry_items_.at(id);
}

bool PoetryDatabase::parseCSVLine(std::string& line, 
                 std::string& title, 
                 std::string& dynasty, 
                 std::string& author, 
                 std::string& content) {
    std::string field;
    std::vector<std::string> fields;
    for(char c: line){
        if(c==','){
            fields.push_back(field);
            field.clear();
        }else{
            field.push_back(c);
        }
    }
    fields.push_back(field);
    if(fields.size() < 4)
        return false;

    title = trimQuotes(fields[0]);
    dynasty = trimQuotes(fields[1]);
    author = trimQuotes(fields[2]);
    content = trimQuotes(fields[3]);

    return true;
}

std::string PoetryDatabase::trimQuotes(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

// 判断 codepoint 是否为我们认为的句末标点（中文/全角逗号句号问叹号等）
bool PoetryDatabase::isSentenceTerminator(uint16_t ch) {
    auto cp = ReString::getUtf8Code(ch);
    return cp == 0xFF0C || cp == 0x3002 || cp == 0xFF01 || cp == 0xFF1F;
}

std::vector<ReString> PoetryDatabase::splitSentences(const ReString& content) {
    std::vector<ReString> result;
    ReString current_sentence;
    for (size_t i = 0; i < content.size(); ++i) {
        uint16_t ch = content[i];
        
        if (isSentenceTerminator(ch)) {
            if(!current_sentence.empty())
                result.push_back(current_sentence);
            current_sentence.clear();
        }else{
            current_sentence.push_back(ch);
        }
    }
    if (!current_sentence.empty()) {
        result.push_back(current_sentence);
    }
    return result;
}