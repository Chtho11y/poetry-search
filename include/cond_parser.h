#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

#include "restring.h"

std::pair<uint32_t, uint32_t> readUTF8Char(const std::string& str, size_t& pos, bool movePos = true);

struct utf8stream{
    std::string str;
    size_t pos = 0, last_delta = 0;

    static const uint32_t INVALID_CP = 0xFFFFFFFEu;
    static const uint32_t EOF_CP = 0xFFFFFFFFu;

    utf8stream(std::string str): str(str){}

    uint32_t read(){
        auto [cp, len] = readUTF8Char(str, pos);
        if (cp == INVALID_CP)
            throw std::invalid_argument("invalid utf8 char at " + std::to_string(pos) + " byte in " + str);
        last_delta = len;
        return cp;
    }

    uint32_t read_nonblanks(){
        skip_blanks();
        return read();
    }

    int read_int(){
        auto [ch, len] = readUTF8Char(str, pos);
        if(!isdigit(ch))
            throw std::invalid_argument("expected digit at " + std::to_string(pos) + " byte in " + str);
        std::string numStr;
        numStr += ch;
        while(pos < str.size()){
            ch = readUTF8Char(str, pos, false).first;
            if(!isdigit(ch))
                break;
            readUTF8Char(str, pos);
            numStr += ch;
        }
        return std::stoi(numStr);
    }

    uint32_t peek(){
        return readUTF8Char(str, pos, false).first;
    }

    bool eof(){
        return pos >= str.size();
    }

    bool has_next(){
        return pos < str.size();
    }

    size_t tell(){
        return pos;
    }

    void seek(size_t pos){
        this->pos = pos;
    }

    void rollback(){
        if(last_delta > pos)
            pos = 0;
        else
            pos -= last_delta;
    }

    void skip_blanks(){
        while(pos < str.size()){
            auto [ch, _] = readUTF8Char(str, pos, false);
            if(ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
                break;
            readUTF8Char(str, pos);
        }
    }
};

struct Cond{
    enum class CondType{
        Base,
        Comb,
        Option,
        List
    };

    enum class BaseCondType{
        Character,
        Strokes,
        Pinyin,
        Frequency,
        Structure,
        Chaizi,
        Wildcard
    };

    CondType type;
    std::unordered_map<uint16_t, bool> matchCache;

    Cond(){}
    virtual ~Cond() = default;

    virtual std::string toString() const {
        return "BaseCond";
    }

    virtual bool match(const HanziData& data) const {
        return false;
    }
};

using cond_ptr = std::shared_ptr<Cond>;

struct BaseCond: Cond{
    Cond::BaseCondType baseType = Cond::BaseCondType::Character;

    BaseCond(Cond::BaseCondType type): baseType(type){}
};

struct CharCond: BaseCond{
    uint16_t ch;

    CharCond(uint32_t cp): BaseCond(Cond::BaseCondType::Character){
        ch = ReString::getCode(cp);
    }

    std::string toString() const override {
        return "\'" + ReString::codepointToString(ReString::getUtf8Code(ch)) + "\'";
    }

    virtual bool match(const HanziData& data) const override {
        return data.index == ch;
    }
};

struct WildcardCond: BaseCond{
    WildcardCond(): BaseCond(Cond::BaseCondType::Wildcard){}

    std::string toString() const override {
        return "Any";
    }

    virtual bool match(const HanziData&) const override {
        return true;
    }
};

struct FreqCond: BaseCond{
    int freq;

    FreqCond(int value): BaseCond(Cond::BaseCondType::Frequency){
        freq = value;
    }

    std::string toString() const override {
        return "Freq=" + std::to_string(freq);
    }

    virtual bool match(const HanziData& data) const override {
        return data.frequency <= freq;
    }
};

struct StrokeCond: BaseCond{
    int strokes;

    StrokeCond(int value): BaseCond(Cond::BaseCondType::Strokes){
        strokes = value;
    }

    std::string toString() const override {
        return "Stroke=" + std::to_string(strokes);
    }

    virtual bool match(const HanziData& data) const override {
        return data.strokes == strokes;
    }
};

struct StructCond: BaseCond{
    char group;
    int subGroup;
    StructCond(std::string value): BaseCond(Cond::BaseCondType::Structure){
        if(value.size() == 0 || value.size() > 2 || !isalpha(value[0]) || (value.size() == 2 && !isdigit(value[1]))){
            throw std::invalid_argument("invalid structure cond: " + value);
        }
        group = value[0];
        subGroup = value.size() == 2 ? value[1] - '0' : 0;
    }

    std::string toString() const override {
        return "Struct=" + std::string(1, group) + (subGroup > 0 ? std::to_string(subGroup) : "");
    }

    virtual bool match(const HanziData& data) const override {
        if(data.structure.size() == 0)
            return false;
        if(data.structure[0] != group)
            return false;
        if(subGroup > 0){
            if(data.structure.size() < 2)
                return false;
            if(data.structure[1] - '0' != subGroup)
                return false;
        }
        return true;
    }
};

struct PinyinCond: BaseCond{
    std::string pinyin;

    PinyinCond(std::string value): BaseCond(Cond::BaseCondType::Pinyin){
        pinyin = value;
    }

    std::string toString() const override {
        return "Pinyin=" + pinyin;
    }

    virtual bool match(const HanziData& data) const override {
        for(const auto& py : data.pinyin){
            if(py == pinyin)
                return true;
        }
        return false;
    }
};

struct CombCond: Cond{
    std::vector<cond_ptr> conds;

    CombCond() {
        type = Cond::CondType::Comb;
    }

    std::string toString() const override {
        std::string result = "CombCond: [ ";
        for(const auto& c : conds){
            result += c->toString() + " ";
        }
        result += "]";
        return result;
    }

    virtual bool match(const HanziData& data) const override {
        for(const auto& c : conds){
            if(!c->match(data))
                return false;
        }
        return true;
    }
};

struct OptionCond: Cond{
    std::vector<cond_ptr> conds;

    OptionCond() {
        type = Cond::CondType::Option;
    }

    std::string toString() const override {
        std::string result = "OptionCond: { ";
        for(const auto& c : conds){
            result += c->toString() + " ";
        }
        result += "}";
        return result;
    }

    virtual bool match(const HanziData& data) const override {
        for(const auto& c : conds){
            if(c->match(data))
                return true;
        }
        return false;
    }
};

struct CondList: Cond{
    std::vector<cond_ptr> conds;

    CondList() {
        type = Cond::CondType::List;
    }

    std::string toString() const override {
        std::string result = "CondList: ( ";
        for(const auto& c : conds){
            result += c->toString() + " ";
        }
        result += ")";
        return result;
    }

    virtual bool match_all(const ReString& s){
        if(s.size() != conds.size())
            return false;
        for(size_t i=0; i<conds.size(); i++){
            auto baseCond = conds[i];
            auto data = ReString::getHanziData(s[i]);
            if(!baseCond->match(data))
                return false;
        }
        return true;
    }

    virtual bool match(const HanziData&) const override {
        throw std::logic_error("kernel error: CondList::match(uint16_t) is not supported.");
    }
};

std::shared_ptr<BaseCond> parseBaseCond(utf8stream& stream);
std::shared_ptr<CombCond> parseCombCond(utf8stream& stream);
std::shared_ptr<OptionCond> parseOptionCond(utf8stream& stream);
std::shared_ptr<CondList> parseCondList(utf8stream& stream);
std::shared_ptr<CondList> parseCond(const std::string& condStr);