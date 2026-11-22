#pragma once

#include <cstdint>
#include <string>
#include <string_view>

// COP_ENGINE_PATCH: COP-ENG-248 - one Unicode decoder for XML, fonts and UI.
// Keep byte offsets at storage boundaries; never treat a UTF-8 byte as a glyph.
namespace UTF8Text
{
inline bool Decode(std::string_view text, size_t& offset, uint32_t& value)
{
    if (offset >= text.size())
        return false;
    const size_t start = offset;
    const auto byte = static_cast<unsigned char>(text[offset++]);
    if (byte < 0x80)
    {
        value = byte;
        return true;
    }
    unsigned extra = 0;
    uint32_t minimum = 0;
    if (byte >= 0xC2 && byte <= 0xDF) { extra = 1; value = byte & 0x1F; minimum = 0x80; }
    else if (byte >= 0xE0 && byte <= 0xEF) { extra = 2; value = byte & 0x0F; minimum = 0x800; }
    else if (byte >= 0xF0 && byte <= 0xF4) { extra = 3; value = byte & 0x07; minimum = 0x10000; }
    else { value = 0xFFFD; return false; }
    if (text.size() - offset >= extra)
    {
        unsigned i = 0;
        for (; i < extra; ++i)
        {
            const auto next = static_cast<unsigned char>(text[offset + i]);
            if ((next & 0xC0) != 0x80)
                break;
            value = (value << 6) | (next & 0x3F);
        }
        if (i == extra && value >= minimum && value <= 0x10FFFF &&
            !(value >= 0xD800 && value <= 0xDFFF))
        {
            offset += extra;
            return true;
        }
    }
    offset = start + 1;
    value = 0xFFFD;
    return false;
}

inline bool IsValid(std::string_view text)
{
    size_t offset = 0;
    uint32_t value;
    while (offset < text.size())
        if (!Decode(text, offset, value))
            return false;
    return true;
}

inline void Append(std::string& text, uint32_t value)
{
    if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) value = 0xFFFD;
    if (value < 0x80) text += static_cast<char>(value);
    else if (value < 0x800)
    {
        text += static_cast<char>(0xC0 | (value >> 6));
        text += static_cast<char>(0x80 | (value & 0x3F));
    }
    else if (value < 0x10000)
    {
        text += static_cast<char>(0xE0 | (value >> 12));
        text += static_cast<char>(0x80 | ((value >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (value & 0x3F));
    }
    else
    {
        text += static_cast<char>(0xF0 | (value >> 18));
        text += static_cast<char>(0x80 | ((value >> 12) & 0x3F));
        text += static_cast<char>(0x80 | ((value >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (value & 0x3F));
    }
}

inline size_t Next(std::string_view text, size_t offset)
{
    uint32_t value;
    Decode(text, offset, value);
    return offset;
}

inline size_t Previous(std::string_view text, size_t offset)
{
    if (offset > text.size()) offset = text.size();
    if (offset == 0) return 0;
    do { --offset; } while (offset && (static_cast<unsigned char>(text[offset]) & 0xC0) == 0x80);
    return offset;
}

inline size_t Prefix(std::string_view text, size_t capacity)
{
    size_t offset = 0, previous = 0;
    while (offset < text.size())
    {
        previous = offset;
        offset = Next(text, offset);
        if (offset > capacity) return previous;
    }
    return offset;
}

inline size_t Count(std::string_view text)
{
    size_t offset = 0, count = 0;
    while (offset < text.size()) { offset = Next(text, offset); ++count; }
    return count;
}

// Legacy tables are added below. Undefined bytes become a visible replacement.
inline uint32_t LegacyCodepoint(unsigned char byte, unsigned codepage = 1251)
{
    static constexpr uint32_t cp1250[128] = {
        0x20AC, 0xFFFD, 0x201A, 0xFFFD, 0x201E, 0x2026, 0x2020, 0x2021,
        0xFFFD, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
        0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0xFFFD, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,
        0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
        0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
        0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
        0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
        0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
        0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
        0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
        0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
        0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
        0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
        0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
        0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
    };
    static constexpr uint32_t cp1251[128] = {
        0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
        0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
        0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
        0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
        0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
        0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
        0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
        0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
        0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
        0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
        0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
        0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
        0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
    };
    static constexpr uint32_t cp1252[128] = {
        0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
        0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
        0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178,
        0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
        0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
        0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
        0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
        0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
        0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
        0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
        0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
        0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
        0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
        0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF,
    };
    if (byte < 0x80 || codepage == 28591) return byte;
    return (codepage == 1252 ? cp1252 : codepage == 1250 ? cp1250 : cp1251)[byte - 0x80];
}

inline std::string Normalize(std::string_view text, unsigned codepage = 1251)
{
    if (text.size() >= 3 && text.substr(0, 3) == "\xEF\xBB\xBF") text.remove_prefix(3);
    if (IsValid(text)) return std::string(text);
    std::string result;
    result.reserve(text.size() * 2);
    for (unsigned char byte : text) Append(result, LegacyCodepoint(byte, codepage));
    return result;
}

inline unsigned XMLCodepage(std::string_view text, std::string_view source)
{
    std::string header(text.substr(0, 512));
    for (char& ch : header) if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
    const auto declaration = header.find("<?xml");
    const auto end = header.find("?>", declaration == std::string::npos ? 0 : declaration);
    if (declaration != std::string::npos && end != std::string::npos)
    {
        header = header.substr(declaration, end - declaration);
        if (header.find("1252") != std::string::npos) return 1252;
        if (header.find("1250") != std::string::npos) return 1250;
        if (header.find("1251") != std::string::npos) return 1251;
        if (header.find("iso-8859-1") != std::string::npos) return 28591;
    }
    std::string path(source);
    for (char& ch : path)
    {
        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
        if (ch == '/') ch = '\\';
    }
    path.insert(0, "\\");
    for (const char* language : {"fra", "eng", "ger", "ita", "spa"})
        if (path.find(std::string("\\") + language + "\\") != std::string::npos) return 1252;
    for (const char* language : {"pol", "cze"})
        if (path.find(std::string("\\") + language + "\\") != std::string::npos) return 1250;
    return 1251;
}
}
