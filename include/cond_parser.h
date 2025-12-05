#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <set>
#include <regex>

#include "restring.h"

std::pair<uint32_t, uint32_t> readUTF8Char(const std::string& str, size_t& pos, bool movePos = true);

struct ParseException: public std::exception{
    std::string message;
    size_t pos_l, pos_r;

    ParseException(std::string info, size_t pos_l, size_t pos_r)
        : pos_l(pos_l), pos_r(pos_r){
        message = info + " at position [" + std::to_string(pos_l) + ", " + std::to_string(pos_r) + ")";
    }
    const char* what() const noexcept override {
        return message.c_str();
    }
};

struct cond_token{
    enum TokenType{
        Char, Letters, Number, LBracket, RBracket, LSquare, RSquare, 
        Comma, Quote, Lt, Gt, At, Hash, Dollar, Asterisk, QuestionMark,
        And, Or, LParen, RParen
    };
    size_t nxt_pos;
    std::pair<size_t, size_t> original_pos;
    TokenType type;
    std::string value;

    cond_token(TokenType type, std::string value, size_t pos_l, size_t pos_r): 
        type(type), value(value), nxt_pos(0), original_pos(pos_l, pos_r){}
};

std::vector<cond_token> tokenizeCondString(const std::string& condStr);

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
    std::vector<bool> cache;

    Cond(){}
    virtual ~Cond() = default;

    virtual std::string toString() const {
        return "BaseCond";
    }

    virtual bool match(const HanziData& data) const {
        return false;
    }

    bool match(uint16_t code) const {
        if(code >= cache.size())
            return false;
        return cache[code];
    }

    virtual void init(){
        auto size = ReString::char_map.size();
        cache.clear();
        cache.resize(size, false);

        for(auto& [code, data]: ReString::hanzi_data){
            cache[code] = match(data);
        }
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

    // void init() override {
    //     //no op
    // }
};

struct WildcardCond: BaseCond{
    WildcardCond(): BaseCond(Cond::BaseCondType::Wildcard){}

    std::string toString() const override {
        return "Any";
    }

    virtual bool match(const HanziData&) const override {
        return true;
    }

    // void init() override {
    //     //no op
    // }
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
        return data.frequency == freq;
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
        group = toupper(value[0]);
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
    std::string regex;


    PinyinCond(std::string value): BaseCond(Cond::BaseCondType::Pinyin){
        pinyin = value;
        for(auto c: pinyin){
            if(c == '?'){
                regex += "[a-zɡ]*";
            }else if(c == 'g'){
                regex += "[gɡ]";
            }else{
                regex += c;
            }
        }
        if(!pinyin.empty() && !isdigit(pinyin.back())){
            regex += "[0-4]?";
        }
    }

    std::string toString() const override {
        return "Pinyin=" + regex;
    }

    virtual bool match(const HanziData& data) const override {
        auto matcher = std::regex(regex);
        for(const auto& py : data.pinyin){
            if(std::regex_match(py, matcher)){
                return true;
            }
        }
        return false;
    }
};

struct ChaiziCond: BaseCond{
    ReString component;

    ChaiziCond(const ReString& value): BaseCond(Cond::BaseCondType::Chaizi){
        component = value;
    }

    ChaiziCond(uint16_t cp): BaseCond(Cond::BaseCondType::Chaizi){
        component.push_back(cp);
    }

    std::string toString() const override {
        return "Chaizi=" + component.toString();
    }

    virtual bool match(const HanziData& data) const override {
        auto get_components = [](const ReString& rs){
            std::multiset<uint16_t> s;
            for (auto c : rs) {
                s.insert(c);
            }
            return s;
        };

        auto target_set = get_components(component);
        int las = -1;
        
        for(const auto& cz : data.chaizi){
            auto cz_set = get_components(cz);
            bool found = true;
            for(const auto& c : target_set){
                if(c == las)
                    continue;
                if(cz_set.count(c) < target_set.count(c)){
                    found = false;
                    break;
                }
                las = c;
            }
            if(found)
                return true;
        }

        return component == data.character;
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

    void init() override {
        for(const auto& c : conds){
            c->init();
        }
        auto size = ReString::char_map.size();
        cache.clear();
        cache.resize(size, true);

        for(auto& [code, data]: ReString::hanzi_data){
            for(const auto& c : conds){
                if(!c->match(code)){
                    cache[code] = false;
                    break;
                }
            }
        }
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

    void init() override {
        for(const auto& c : conds){
            c->init();
        }
        auto size = ReString::char_map.size();
        cache.clear();
        cache.resize(size, false);

        for(auto& [code, data]: ReString::hanzi_data){
            for(const auto& c : conds){
                if(c->match(code)){
                    cache[code] = true;
                    break;
                }
            }
        }
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
        for(size_t i = 0; i < s.size(); ++i){
            if(!conds[i]->match(s[i]))
                return false;
        }
        return true;
    }

    virtual bool match(const HanziData&) const override {
        throw std::logic_error("kernel error: CondList::match(uint16_t) is not supported.");
    }

    void init() override {
        for(const auto& c : conds){
            c->init();
        }
    }
};

std::shared_ptr<CondList> parseCond(const std::string& condStr);
std::shared_ptr<BaseCond> parseBaseCond(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end);
std::shared_ptr<CombCond> parseCombCond(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end);
std::shared_ptr<OptionCond> parseOptionCond(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end);
std::shared_ptr<CondList> parseCondList(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end);
