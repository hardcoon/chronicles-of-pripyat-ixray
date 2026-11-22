#include "../src/xrCore/UTF8Text.h"
#include <cassert>
#include <cstdio>

// COP_ENGINE_PATCH: COP-ENG-248 - strict Unicode and legacy compatibility regressions.
int main()
{
    using namespace UTF8Text;
    for (uint32_t cp = 0; cp <= 0x10FFFF; ++cp)
    {
        if (cp >= 0xD800 && cp <= 0xDFFF) continue;
        std::string encoded;
        Append(encoded, cp);
        assert(IsValid(encoded));
        size_t offset = 0;
        uint32_t decoded;
        assert(Decode(encoded, offset, decoded) && decoded == cp && offset == encoded.size());
        assert(Count(encoded) == 1 && Previous(encoded, encoded.size()) == 0);
        for (size_t cap = 0; cap < encoded.size(); ++cap) assert(Prefix(encoded, cap) == 0);
    }
    for (const std::string invalid : {"\xC0\xAF", "\xC1\xBF", "\xE0\x80\xAF", "\xED\xA0\x80",
        "\xF0\x80\x80\xAF", "\xF4\x90\x80\x80", "\xF5\x80\x80\x80", "\x80", "\xE2\x82", "\xF0\x9F\x92"})
        assert(!IsValid(invalid));
    const std::string russian = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
    assert(Normalize("\xCF\xF0\xE8\xE2\xE5\xF2") == russian);
    const std::string french = "\xC3\xA9\xC3\xA8\xC3\xA0\xC3\xA7\xC5\x93";
    assert(Normalize("\xE9\xE8\xE0\xE7\x9C", 1252) == french);
    assert(Normalize(french, 1251) == french);
    assert(Normalize("\xEF\xBB\xBF" + russian) == russian);
    assert(Normalize(Normalize(russian)) == russian);
    assert(XMLCodepage("<?xml version='1.0' encoding='windows-1252'?>", "text/fra/a.xml") == 1252);
    assert(XMLCodepage("<?xml version='1.0' encoding='windows-1251'?>", "text/fra/a.xml") == 1251);
    assert(XMLCodepage("<?xml version='1.0' encoding='utf-8'?>", "text/fra/a.xml") == 1252);
    assert(XMLCodepage("", "text/pol/a.xml") == 1250);
    assert(XMLCodepage("", "text/rus/a.xml") == 1251);
    for (unsigned cp : {1250u, 1251u, 1252u})
        for (unsigned byte = 128; byte <= 255; ++byte)
        {
            const std::string legacy(1, static_cast<char>(byte));
            const std::string utf8 = Normalize(legacy, cp);
            assert(IsValid(utf8));
            size_t offset = 0;
            uint32_t decoded;
            assert(Decode(utf8, offset, decoded) && decoded == LegacyCodepoint(static_cast<unsigned char>(byte), cp));
            assert(Normalize(utf8, cp) == utf8);
        }
    const std::string mixed = "A" + french + russian + "\xF0\x9F\x98\x80";
    size_t offset = 0;
    while (offset < mixed.size())
    {
        const size_t next = Next(mixed, offset);
        assert(Previous(mixed, next) == offset);
        for (size_t cap = offset; cap < next; ++cap) assert(Prefix(mixed, cap) == offset);
        offset = next;
    }
    std::puts("PASS: all 1,112,064 Unicode scalar values; invalid UTF-8; CP1250/1251/1252; BOM; boundaries; idempotence.");
}
