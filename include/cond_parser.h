// #pragma once

// #include <memory>
// #include <vector>
// #include <string>
// #include <cstdint>

// #include "restring.h"

// struct Cond{
//     enum class CondType{
//         Base,
//         Comb,
//         Option,
//         List
//     };

//     enum class BaseCondType{
//         Character,
//         Strokes,
//         Pinyin,
//         Frequency,
//         Structure,
//         Chaizi,
//         Wildcard
//     };

//     virtual ~Cond() = default;
// };

// uint32_t readUTF8Char(const std::string& str, size_t& pos, bool movePos = true);

// struct utf8stream{
//     std::string str;
//     size_t pos = 0;

//     utf8stream(std::string str): str(str){}

//     uint32_t read(){
//         auto cp = readUTF8Char(str, pos);
//         if (cp == INVALID_CODE_POINT)
//             throw std::invalid_argument("invalid utf8 char at " + std::to_string(pos) + " byte in " + str);
//         return cp;
//     }

//     uint32_t read_nonblanks(){
//         skip_blanks();
//         return read();
//     }

//     int read_int(){
//         uint32_t ch = readUTF8Char(str, pos);
//         if(!isdigit(ch))
//             throw std::invalid_argument("expected digit at " + std::to_string(pos) + " byte in " + str);
//         std::string numStr;
//         numStr += ch;
//         while(pos < str.size()){
//             ch = readUTF8Char(str, pos, false);
//             if(!isdigit(ch))
//                 break;
//             readUTF8Char(str, pos);
//             numStr += ch;
//         }
//         return std::stoi(numStr);
//     }

//     uint32_t peek(){
//         return readUTF8Char(str, pos, false);
//     }

//     bool eof(){
//         return pos >= str.size();
//     }

//     bool has_next(){
//         return pos < str.size();
//     }

//     size_t tell(){
//         return pos;
//     }

//     void seek(size_t pos){
//         this->pos = pos;
//     }

//     void rollback(size_t delta = 1){
//         if(delta > pos)
//             pos = 0;
//         else
//             pos -= delta;
//     }

//     void skip_blanks(){
//         while(pos < str.size()){
//             auto ch = readUTF8Char(str, pos, false);
//             if(ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
//                 break;
//             readUTF8Char(str, pos);
//         }
//     }
// };

// using cond_ptr = std::shared_ptr<Cond>;

// struct BaseCond: Cond{
//     Cond::BaseCondType baseType = Cond::BaseCondType::Character;

//     BaseCond(Cond::BaseCondType type): baseType(type){}
// };

// struct CharCond: BaseCond{
//     uint16_t ch;

//     CharCond(std::string value): BaseCond(Cond::BaseCondType::Character){
//         size_t pos = 0;
//         ch = readUTF8Char(value, pos);
//         if(pos != value.size()){
//             throw std::invalid_argument("invalid character cond: " + value);
//         }
//     }
// };

// struct FreqCond: BaseCond{
//     int freq;

//     FreqCond(std::string value): BaseCond(Cond::BaseCondType::Frequency){
//         freq = std::stoi(value);
//     }
// };

// struct StrokeCond: BaseCond{
//     int strokes;

//     StrokeCond(std::string value): BaseCond(Cond::BaseCondType::Strokes){
//         strokes = std::stoi(value);
//     }
// };

// struct StructCond: BaseCond{
//     char group;
//     int subGroup;
//     StructCond(std::string value): BaseCond(Cond::BaseCondType::Structure){
//         if(value.size() == 0 || value.size() > 2 || !isalpha(value[0]) || (value.size() == 2 && !isdigit(value[1]))){
//             throw std::invalid_argument("invalid structure cond: " + value);
//         }
//         group = value[0];
//         subGroup = value.size() == 2 ? value[1] - '0' : 0;
//     }
// };

// struct CombCond: Cond{
//     std::vector<cond_ptr> conds;
// };

// struct OptionCond: Cond{
//     std::vector<cond_ptr> conds;
// };

// struct CondList: Cond{
//     std::vector<cond_ptr> conds;
// };

// std::shared_ptr<BaseCond> parseBaseCond(utf8stream& stream){
//     auto baseCond = std::make_shared<BaseCond>();
//     while (stream.has_next()){
//         auto ch = stream.read_nonblanks();
//         if (ch == EOF_CP)
//             throw std::invalid_argument("unexpected end of string in the end of \"" + stream.str + "\"");

//         if (ch == '*'){
//             baseCond = std::make_shared<BaseCond>("*", Cond::BaseCondType::Wildcard);
//             break;
//         }else if (ch == '$'){
//             int freq = stream.read_int();
//             baseCond = std::make_shared<FreqCond>(std::to_string(freq));
//             break;
//         }else if (ch == '@'){
//             std::string structStr;
            
//             break;
//         }else if (isdigit(ch)){
//             stream.rollback();
//             int strokes = stream.read_int();
//             baseCond = std::make_shared<StrokeCond>(std::to_string(strokes));
//             break;
//         }else if (isalpha(ch)){
            
//         } else {

//         }
//         break;
//     }
//     return baseCond;
// }

// std::shared_ptr<CombCond> parseCombCond(utf8stream& stream){
//     auto combCond = std::make_shared<CombCond>();
//     //解析token
//     return combCond;
// }

// std::shared_ptr<OptionCond> parseOptionCond(utf8stream& stream){
//     auto optionCond = std::make_shared<OptionCond>();
//     //解析token
//     return optionCond;
// }

// std::shared_ptr<CondList> parseCondList(utf8stream& stream){
//     auto condList = std::make_shared<CondList>();
//     while(stream.has_next()){
//         auto ch = stream.read_nonblanks();
//         if(ch == EOF_CP)
//             break;

//         if(ch == '['){
//             auto optionCond = parseOptionCond(stream);
//             condList->conds.push_back(optionCond);
//         }else{
//             auto baseCond = parseBaseCond(stream);
//             condList->conds.push_back(baseCond);
//         }
//     }
//     return condList;
// }

// std::shared_ptr<CondList> parseCond(const std::string& condStr){
//     std::vector<cond_ptr> conds;

//     utf8stream stream(condStr);
//     return parseCondList(stream);
// }