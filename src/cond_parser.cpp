#include "cond_parser.h"

const uint32_t EOF_CP = 0xFFFFFFFFu;
const uint32_t INVALID_CP = 0xFFFFFFFEu;

std::pair<uint32_t, uint32_t> readUTF8Char(const std::string& str, size_t& pos, bool movePos){
    if(pos >= str.size())
        return {EOF_CP, 0};
    auto [cp, len] = ReString::nextUtf8Codepoint(str, pos);
    // std::cout << "readUTF8Char: pos=" << pos << ", cp=" << std::hex << cp << ", len=" << len << std::dec << std::endl;
    if(cp == 0xFFFFu){
        return {INVALID_CP, 0};
    }
    if(movePos)
        pos += len;
    return {cp, len};
}

std::shared_ptr<BaseCond> parseBaseCond(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end){
    std::shared_ptr<BaseCond> baseCond = nullptr;

    if(pos >= pos_end)
        throw ParseException("unexpected end of condition", 0, 0);

    auto& token = tokens[pos];
    if(token.type == cond_token::TokenType::Asterisk){
        baseCond = std::make_shared<WildcardCond>();
        pos++;
    }else if(token.type == cond_token::TokenType::Dollar){
        pos++;
        if(pos >= pos_end)
            throw ParseException("expected frequency number after '$'", token.original_pos.first, token.original_pos.second);
        auto& numToken = tokens[pos];
        if(numToken.type != cond_token::TokenType::Number){
            throw ParseException("expected frequency number after '$'", numToken.original_pos.first, numToken.original_pos.second);
        }
        int freq = std::stoi(numToken.value);
        baseCond = std::make_shared<FreqCond>(freq);
        pos++;
    }else if(token.type == cond_token::TokenType::At){
        pos++;
        if(pos >= pos_end)
            throw ParseException("expected structure string after '@'", token.original_pos.first, token.original_pos.second);
        auto& structToken = tokens[pos];
        if(structToken.type != cond_token::TokenType::Letters){
            throw ParseException("expected structure string after '@'", structToken.original_pos.first, structToken.original_pos.second);
        }
        baseCond = std::make_shared<StructCond>(structToken.value);
        pos++;
    }else if(token.type == cond_token::TokenType::Number){
        int strokes = std::stoi(token.value);
        baseCond = std::make_shared<StrokeCond>(strokes);
        pos++;
    }else if(token.type == cond_token::TokenType::Letters){
        baseCond = std::make_shared<PinyinCond>(token.value);
        pos++;
    }else if(token.type == cond_token::TokenType::Char){
        uint32_t ch = ReString::nextUtf8Codepoint(token.value, 0).first;
        baseCond = std::make_shared<CharCond>(ch);
        pos++;
    }else{
        throw ParseException("Unexpected token", token.original_pos.first, token.original_pos.second);
    }
    return baseCond;
}

std::shared_ptr<CombCond> parseCombCond(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end){
    auto combCond = std::make_shared<CombCond>();
    bool flash = false;

    auto as_chaizi_cond = [](std::shared_ptr<BaseCond> baseCond) -> std::shared_ptr<ChaiziCond> {
        if(baseCond && baseCond->baseType == BaseCond::BaseCondType::Character){
            auto ch_cond = std::dynamic_pointer_cast<CharCond>(baseCond);
            return std::make_shared<ChaiziCond>(ch_cond->ch);
        }
        return nullptr;
    };

    while(pos < pos_end){
        auto& token = tokens[pos];
        if(token.type == cond_token::TokenType::RBracket){
            break;
        }else if(token.type == cond_token::TokenType::Comma){
            pos++;
            flash = true;
        }else {
            auto baseCond = parseBaseCond(tokens, pos, pos_end);
            
            if(auto chaiziCond = as_chaizi_cond(baseCond)){
                if(flash || combCond->conds.empty()){
                    combCond->conds.push_back(chaiziCond);
                    flash = false;
                }else{
                    auto backCond = std::dynamic_pointer_cast<BaseCond>(combCond->conds.back());
                    if(auto backChaiziCond = as_chaizi_cond(backCond)){
                        backChaiziCond->component.push_back(chaiziCond->component[0]);
                    }else{
                        combCond->conds.push_back(chaiziCond);
                    }
                    flash=false;
                }
            }else{
                combCond->conds.push_back(baseCond);
                flash = false;
            }
        }
    }
    return combCond;
}

std::shared_ptr<OptionCond> parseOptionCond(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end){
    auto optionCond = std::make_shared<OptionCond>();
    while(pos < pos_end){
        auto& token = tokens[pos];
        if(token.type == cond_token::TokenType::RBracket){
            break;
        }else if(token.type == cond_token::TokenType::LBracket){
            pos++;
            auto combCond = parseCombCond(tokens, pos, token.nxt_pos);
            optionCond->conds.push_back(combCond);
            pos++; // skip RBracket
        }else{
            auto baseCond = parseBaseCond(tokens, pos, pos_end);
            optionCond->conds.push_back(baseCond);
        }
    }
    return optionCond;
}

std::shared_ptr<CondList> parseCondList(const std::vector<cond_token>& tokens, size_t& pos, size_t pos_end){
    auto condList = std::make_shared<CondList>();
    while(pos < pos_end){
        auto& token = tokens[pos];
        if(token.type == cond_token::TokenType::LBracket){
            pos++;
            auto optionCond = parseOptionCond(tokens, pos, token.nxt_pos);
            condList->conds.push_back(optionCond);
            pos++; // skip RBracket
        }else{
            auto baseCond = parseBaseCond(tokens, pos, pos_end);
            condList->conds.push_back(baseCond);
        }
    }
    return condList;
}

std::shared_ptr<CondList> parseCond(const std::string& condStr){
    auto tokens = tokenizeCondString(condStr);
    size_t pos = 0;
    return parseCondList(tokens, pos, tokens.size());
}

void braket_match(std::vector<cond_token>& tokens){
    size_t pos = 0;
    std::vector<std::pair<cond_token::TokenType, size_t>> st;
    std::map<cond_token::TokenType, cond_token::TokenType> bracket_pairs = {
        { cond_token::TokenType::LBracket, cond_token::TokenType::RBracket },
        { cond_token::TokenType::LParen, cond_token::TokenType::RParen  },
        { cond_token::TokenType::Lt, cond_token::TokenType::Gt },
    };
    while(pos < tokens.size()){
        auto& token = tokens[pos];
        token.nxt_pos = pos + 1;
        if(bracket_pairs.count(token.type)){
            st.emplace_back(token.type, pos);
        }else{
            for(const auto& [openType, closeType]: bracket_pairs){
                if(token.type == closeType){
                    if(st.empty() || st.back().first != openType){
                        throw ParseException("unmatched closing bracket", token.original_pos.first, token.original_pos.second);
                    }else{
                        auto [_, openPos] = st.back();
                        st.pop_back();
                        tokens[openPos].nxt_pos = pos;
                    }
                }
            }
        }
        pos++;
    }
    if(!st.empty()){
        auto [openType, openPos] = st.back();
        throw ParseException("unmatched opening bracket", tokens[openPos].original_pos.first, tokens[openPos].original_pos.second);
    }
}

std::vector<cond_token> tokenizeCondString(const std::string& condStr){
    size_t pos = 0;
    using namespace std::string_literals;

    std::vector<cond_token> tokens;
    std::unordered_map<uint32_t, cond_token::TokenType> singleCharTokens = {
        { '[', cond_token::TokenType::LBracket },
        { ']', cond_token::TokenType::RBracket },
        { ',', cond_token::TokenType::Comma },
        { '*', cond_token::TokenType::Asterisk },
        { '$', cond_token::TokenType::Dollar },
        { '@', cond_token::TokenType::At },
        { '<', cond_token::TokenType::Lt },
        { '>', cond_token::TokenType::Gt },
        { '#', cond_token::TokenType::Hash },
        { '"', cond_token::TokenType::Quote},
        { '?', cond_token::TokenType::QuestionMark},
        { '&', cond_token::TokenType::And },
        { '|', cond_token::TokenType::Or  },
        { '(', cond_token::TokenType::LParen },
        { ')', cond_token::TokenType::RParen  },
    };

    auto is_digit = [](uint32_t cp){
        return ('0' <= cp && cp <= '9');
    };

    auto is_alpha = [](uint32_t cp){
        return (('a' <= cp && cp <= 'z') || ('A' <= cp && cp <= 'Z'));
    };

    auto is_space = [](uint32_t cp){
        return (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r');
    };

    while(pos < condStr.size()){
        auto [cp, len] = readUTF8Char(condStr, pos, true);
        auto pos_l = pos - len;
        if(cp == EOF_CP)
            break;

        if(is_alpha(cp) || cp == '?'){
            std::string alphaStr = ReString::codepointToString(cp);
            while (pos < condStr.size()) {
                auto [nextCp, nextLen] = readUTF8Char(condStr, pos, false);
                if (!is_alpha(nextCp) && nextCp != '?' && !is_digit(nextCp))
                    break;
                readUTF8Char(condStr, pos);
                alphaStr += ReString::codepointToString(nextCp);
            }
            tokens.emplace_back(cond_token::TokenType::Letters, alphaStr, pos_l, pos);
        }else if(singleCharTokens.find(cp) != singleCharTokens.end()){
            tokens.emplace_back(singleCharTokens[cp], ReString::codepointToString(cp), pos_l, pos);
        }else if (is_digit(cp)) {
            std::string numStr = ReString::codepointToString(cp);
            while (pos < condStr.size()) {
                auto [nextCp, nextLen] = readUTF8Char(condStr, pos, false);
                if (!is_digit(nextCp))
                    break;
                readUTF8Char(condStr, pos);
                numStr += ReString::codepointToString(nextCp);
            }
            tokens.emplace_back(cond_token::TokenType::Number, numStr, pos_l, pos);
        }else if(is_space(cp)){
            // skip
        }else if(cp <= 127){
            throw ParseException("invalid character", pos_l, pos);
        }else{
            tokens.emplace_back(cond_token::TokenType::Char, ReString::codepointToString(cp), pos_l, pos);
        }
    }

    braket_match(tokens);
    return tokens;
}