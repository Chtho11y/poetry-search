// #include "cond_parser.h"

// const uint32_t EOF_CP = 0xFFFFFFFFu;
// const uint32_t INVALID_CODE_POINT = 0xFFFFFFFEu;

// uint32_t readUTF8Char(const std::string& str, size_t& pos, bool movePos){
//     if(pos >= str.size())
//         return EOF_CP;
//     auto [cp, len] = ReString::nextUtf8Codepoint(str, pos);
//     if(cp == 0xFFFFu){
//         return INVALID_CODE_POINT;
//     }
//     if(movePos)
//         pos += len;
//     return cp;
// }

