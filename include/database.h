#pragma once

#include <unordered_set>
#include <sstream>
#include <vector>

#include "restring.h"

struct PoetryItem {
    std::string dynasty;
    std::string author;
    size_t id;

    std::string title;
    ReString content;
    std::vector<ReString> sentences;

    PoetryItem(){}

    size_t estimateMemoryUsage() const;
};

class PoetryDatabase {
private:
    std::vector<PoetryItem> poetry_items_;

public:
    int loadFromCSV(const std::string& filename);
    
    const std::vector<PoetryItem>& getAllPoetry() const;

    size_t estimateMemoryUsage() const;

    std::vector<std::pair<ReString, size_t>> 
    findSentencesByCharSet(const std::string& charset_utf8);

    PoetryItem getPoetryById(size_t id) const;

    static std::vector<ReString> splitSentences(const ReString& content);

private:
    bool parseCSVLine(std::string& line, 
                     std::string& title, 
                     std::string& dynasty, 
                     std::string& author, 
                     std::string& content);
    
    std::string trimQuotes(const std::string& str);

    static bool isSentenceTerminator(uint16_t ch);

    void insertItem(std::string& title, 
                    std::string& dynasty, 
                    std::string& author, 
                    std::string& content){
        auto id = poetry_items_.size();
        PoetryItem item;
        item.id = id;
        item.title = title;
        item.dynasty = dynasty;
        item.author = author;
        item.content = ReString(content, true);
        item.sentences = splitSentences(item.content);
        poetry_items_.emplace_back(std::move(item));
    }
};