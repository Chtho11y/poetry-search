#pragma once

#include <unordered_map>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>

struct ReString : std::vector<uint16_t> {
    static std::unordered_map<uint32_t, int16_t> char_map;
    static std::unordered_map<uint16_t, uint32_t> code_map;

    ReString() = default;
    ReString(const std::string& s, bool create_new = true);

    std::string toString() const;

    bool operator==(const ReString& other) const;

    friend std::ostream& operator<<(std::ostream& os, const ReString& rs);

    size_t estimateMemoryUsage() const;

    static uint16_t getIllegalCode();

    static uint16_t getCodeOrCreate(uint32_t cp);

    static uint16_t getCode(uint32_t cp);

    static uint32_t getUtf8Code(uint16_t code);

    static std::pair<uint32_t, size_t> nextUtf8Codepoint(const std::string& s, size_t pos);

    static std::string codepointToString(uint32_t cp);

    static size_t estimateMapMemoryUse();
};