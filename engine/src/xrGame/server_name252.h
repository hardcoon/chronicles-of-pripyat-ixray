#pragma once
#include <string>
#include <string_view>

// COP_ENGINE_PATCH: COP-ENG-252 - save names are server-option prefixes too.
// Keep every encoded byte; an 83-byte legacy filename must not enter string64.
namespace server_name252
{
inline std::string first(const char* options)
{
    std::string_view name(options ? options : "");
    name = name.substr(0, name.find('/'));
    // Preserve the existing _GetItem trim semantics without a fixed buffer.
    while (!name.empty() && static_cast<unsigned char>(name.front()) <= ' ') name.remove_prefix(1);
    while (!name.empty() && static_cast<unsigned char>(name.back()) <= ' ') name.remove_suffix(1);
    return std::string(name);
}
}
