#include "cond_parser.h"

std::pair<uint32_t, uint32_t> readUTF8Char(const std::string& str, size_t& pos, bool movePos){
    if(pos >= str.size())
        return {utf8stream::EOF_CP, 0};
    auto [cp, len] = ReString::nextUtf8Codepoint(str, pos);
    // std::cout << "readUTF8Char: pos=" << pos << ", cp=" << std::hex << cp << ", len=" << len << std::dec << std::endl;
    if(cp == 0xFFFFu){
        return {utf8stream::INVALID_CP, 0};
    }
    if(movePos)
        pos += len;
    return {cp, len};
}

std::shared_ptr<BaseCond> parseBaseCond(utf8stream& stream){
    std::shared_ptr<BaseCond> baseCond = nullptr;

    auto is_alpha = [](uint32_t ch){
        return (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'));
    };

    auto is_digit = [](uint32_t ch){
        return ('0' <= ch && ch <= '9');
    };

    while (stream.has_next()){
        auto ch = stream.read_nonblanks();
        if (ch == utf8stream::EOF_CP)
            throw std::invalid_argument("unexpected end of string in the end of \"" + stream.str + "\"");

        if (ch == '*'){
            baseCond = std::make_shared<WildcardCond>();
            break;
        }else if (ch == '$'){
            int freq = stream.read_int();
            baseCond = std::make_shared<FreqCond>(freq);
            break;
        }else if (ch == '@'){
            std::string structStr;
            auto nextCh = stream.read();
            if(is_alpha(nextCh)){
                structStr += nextCh;
                auto peekCh = stream.peek();
                if(is_digit(peekCh)){
                    structStr += stream.read();
                }
            }
            baseCond = std::make_shared<StructCond>(structStr);
            break;
        }else if (is_digit(ch)){
            stream.rollback();
            int strokes = stream.read_int();
            baseCond = std::make_shared<StrokeCond>(strokes);
            break;
        }else if (is_alpha(ch) || ch == '?'){
            std::string pinyinStr;
            stream.rollback();
            while (stream.has_next()){
                auto pch = stream.read_nonblanks();
                if (is_alpha(pch) || pch == '?' || pch == '0' || pch == '1' || pch == '2' || pch == '3' || pch == '4'){
                    pinyinStr += ReString::codepointToString(pch);
                }else{
                    stream.rollback();
                    break;
                }
            }
            baseCond = std::make_shared<PinyinCond>(pinyinStr);
        }else if (ch < 128){
            throw std::invalid_argument("Failed to parse condition.");
        }else{
            baseCond = std::make_shared<CharCond>(ch);
        }
        break;
    }
    return baseCond;
}

std::shared_ptr<CombCond> parseCombCond(utf8stream& stream){
    auto combCond = std::make_shared<CombCond>();
    while(stream.has_next()){
        auto ch = stream.read_nonblanks();
        if(ch == utf8stream::EOF_CP)
            throw std::invalid_argument("unexpected end of string in the end of \"" + stream.str + "\"");

        if(ch == ']'){
            break;
        }else if(ch == ','){
            auto baseCond = parseBaseCond(stream);
            combCond->conds.push_back(baseCond);
        }else {
            stream.rollback();
            auto baseCond = parseBaseCond(stream);
            combCond->conds.push_back(baseCond);
        }
    }
    return combCond;
}

std::shared_ptr<OptionCond> parseOptionCond(utf8stream& stream){
    auto optionCond = std::make_shared<OptionCond>();
    while(stream.has_next()){
        auto ch = stream.read_nonblanks();
        if(ch == utf8stream::EOF_CP)
            throw std::invalid_argument("unexpected end of string in the end of \"" + stream.str + "\"");

        if(ch == ']'){
            break;
        }else if(ch == '['){
            auto combCond = parseCombCond(stream);
            optionCond->conds.push_back(combCond);
        }else{
            stream.rollback();
            auto baseCond = parseBaseCond(stream);
            optionCond->conds.push_back(baseCond);
        }
    }
    return optionCond;
}

std::shared_ptr<CondList> parseCondList(utf8stream& stream){
    auto condList = std::make_shared<CondList>();
    while(stream.has_next()){
        auto ch = stream.read_nonblanks();
        // std::cout << "parseCondList: read char " << std::hex << ch << std::dec << std::endl;
        if(ch == utf8stream::EOF_CP)
            break;

        if(ch == '['){
            auto optionCond = parseOptionCond(stream);
            condList->conds.push_back(optionCond);
        }else{
            stream.rollback();
            auto baseCond = parseBaseCond(stream);
            condList->conds.push_back(baseCond);
        }
    }
    return condList;
}

std::shared_ptr<CondList> parseCond(const std::string& condStr){
    utf8stream stream(condStr);
    return parseCondList(stream);
}