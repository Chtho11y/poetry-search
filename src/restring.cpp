
#include "restring.h"

std::unordered_map<uint32_t, int16_t> ReString::char_map;
std::unordered_map<uint16_t, uint32_t> ReString::code_map;

ReString::ReString(const std::string& s, bool create_new) {
    size_t i = 0;
    while (i < s.size()) {
        auto [cp, len] = nextUtf8Codepoint(s, i);
        uint16_t code = create_new ? getCodeOrCreate(cp) : getCode(cp);
        this->push_back(code);
        i += len;
    }
}

std::string ReString::toString() const {
    std::string result;
    for (auto code : *this) {
        uint32_t cp = code_map.at(code);
        result += codepointToString(cp);
    }
    return result;
}

bool ReString::operator==(const ReString& other) const {
    if (this->size() != other.size()) return false;
    for (size_t i = 0; i < this->size(); ++i) {
        if ((*this)[i] != other[i]) return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const ReString& rs) {
    os << rs.toString();
    return os;
}

size_t ReString::estimateMemoryUsage() const {
    size_t total = 0;
    total += this->capacity() * sizeof(uint16_t);
    return total;
}

uint16_t ReString::getIllegalCode() {
    return 0xFFFF;
}

uint16_t ReString::getCodeOrCreate(uint32_t cp) {
    auto it = char_map.find(cp);
    if (it != char_map.end()) {
        return it->second;
    } else {
        uint16_t code = static_cast<uint16_t>(char_map.size());
        char_map[cp] = code;
        code_map[code] = cp;
        return code;
    }
}

uint16_t ReString::getCode(uint32_t cp) {
    auto it = char_map.find(cp);
    if (it != char_map.end()) {
        return it->second;
    } else {
        return getIllegalCode();
    }
}

uint32_t ReString::getUtf8Code(uint16_t code) {
    auto it = code_map.find(code);
    if (it != code_map.end()) {
        return it->second;
    } else {
        return '?';
    }
}

std::pair<uint32_t, size_t> ReString::nextUtf8Codepoint(const std::string& s, size_t pos) {
    unsigned char c = static_cast<unsigned char>(s[pos]);
    if (c <= 0x7F) return { c, 1 };
    // 多字节序列判定
    if ((c & 0xE0) == 0xC0 && pos + 1 < s.size()) { // 2 bytes
        unsigned char c1 = static_cast<unsigned char>(s[pos + 1]);
        if ((c1 & 0xC0) == 0x80) {
            uint32_t cp = ((c & 0x1F) << 6) | (c1 & 0x3F);
            return { cp, 2 };
        }
    } else if ((c & 0xF0) == 0xE0 && pos + 2 < s.size()) { // 3 bytes
        unsigned char c1 = static_cast<unsigned char>(s[pos + 1]);
        unsigned char c2 = static_cast<unsigned char>(s[pos + 2]);
        if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80) {
            uint32_t cp = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            return { cp, 3 };
        }
    } else if ((c & 0xF8) == 0xF0 && pos + 3 < s.size()) { // 4 bytes
        unsigned char c1 = static_cast<unsigned char>(s[pos + 1]);
        unsigned char c2 = static_cast<unsigned char>(s[pos + 2]);
        unsigned char c3 = static_cast<unsigned char>(s[pos + 3]);
        if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
            uint32_t cp = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            return { cp, 4 };
        }
    }
    return { getIllegalCode(), 1 };
}

std::string ReString::codepointToString(uint32_t cp) {
    std::string result;
    if (cp <= 0x7F) {
        result.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        result.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        result.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        result.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return result;
}

size_t ReString::estimateMapMemoryUse() {
    size_t total = 0;
    total += char_map.bucket_count() * sizeof(void*);
    total += code_map.bucket_count() * sizeof(void*);
    total += (sizeof(uint32_t) + sizeof(int16_t)) * char_map.size();
    total += (sizeof(uint16_t) + sizeof(uint32_t)) * code_map.size();
    return total;
}

std::vector<HanziData> ReString::hanzi_data;

bool ReString::loadHanziData(const std::string& filename) {
    auto res = readHanziData(filename);
    if(!res.has_value()) {
        return false;
    }
    hanzi_data = res.value();
    for (auto & hanzi : hanzi_data) {
        int pos = 0;
        auto [cp, _] = nextUtf8Codepoint(hanzi.character, pos);
        uint16_t idx = hanzi.index - 1;
        char_map[cp] = idx;
        code_map[idx] = cp;
    }
    return true;
}