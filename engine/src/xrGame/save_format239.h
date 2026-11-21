#pragma once

namespace save_format239
{
// Older binaries accept any ALIFE_VERSION >= 6. A distinct magic is required
// to stop their 16-KiB packet readers from loading larger object records.
constexpr u32 legacy_magic = u32(-1);
constexpr u32 extended_magic = u32(-2);
constexpr bool supported(u32 magic) { return magic == legacy_magic || magic == extended_magic; }
}
