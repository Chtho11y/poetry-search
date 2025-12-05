#include "database.h"
#include <cstdlib>


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

int PoetryDatabase::loadFromCSV(const std::string& filename) {
    FILE* file = std::fopen(filename.c_str(), "r");

    if (!file) {
        return false;
    }

    const size_t BUFFER_SIZE = 4 << 20; // 4MB
    std::vector<char> buffer(BUFFER_SIZE);
    std::string line_buf;

    int line_cnt = 0;
    while (true) {
        size_t bytes_read = std::fread(buffer.data(), 1, BUFFER_SIZE, file);
        if (bytes_read == 0) break;

        char* p = buffer.data();
        char* end = buffer.data() + bytes_read;

        while(p < end){
            char* nl = (char*)memchr(p, '\n', end - p);
            if (!nl) {
                line_buf.append(p, end - p);
                break;
            }
            line_buf.append(p, nl - p);
            p = nl + 1;
            if (line_cnt == 0) {
                line_cnt++;
                line_buf.clear();
                continue; // skip header
            }

            if (!line_buf.empty()) {
                std::string title, dynasty, author, content;
                if (parseCSVLine(line_buf, title, dynasty, author, content)) {
                    insertItem(title, dynasty, author, content);
                    line_cnt++;
                }
            }

            line_buf.clear();
        }
    }

    if (!line_buf.empty() && line_cnt > 0) {
        std::string title, dynasty, author, content;
        if (parseCSVLine(line_buf, title, dynasty, author, content)) {
            insertItem(title, dynasty, author, content);
            line_cnt++;
        }
    }

    return line_cnt <= 1 ? 0 : line_cnt - 1; // exclude header
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

std::vector<std::pair<ReString, size_t>>
PoetryDatabase::findSentencesByCond(CondList& cond_list){
    std::vector<std::pair<ReString, size_t>> result;
    cond_list.init();

    // step2: 遍历所有诗句
    for (const auto& item : getAllPoetry()) {
        for (const auto& sent : item.sentences) {
            if(cond_list.match_all(sent)){
                result.emplace_back(sent, item.id);
                break; // 该诗出现一次即可
            }
        }
    }

    return result;
}