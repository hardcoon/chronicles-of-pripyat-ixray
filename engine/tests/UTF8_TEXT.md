# COP-ENG-248 text compatibility

`utf8_text_tests.cpp` is a standalone C++20 regression executable (assertions
must be enabled). It exhausts Unicode scalar values, rejects malformed UTF-8,
checks CP1250/CP1251/CP1252 conversion, BOM handling and character boundaries.

The Windows runtime normalizes XML files and their includes independently.
Valid UTF-8 takes priority over a stale encoding declaration. Legacy input uses
its declared Windows codepage; undeclared localization files use the source
language path, with CP1251 as the default. Numeric XML entities remain Unicode.
Fonts, widths, wrapping, ellipsis and edit buffers then share Unicode codepoints.
The old `opentype` setting no longer changes Unicode character identities.

This does not transcode Lua/LTX files on disk, change save/network formats,
install fonts for every writing system, or add IME/shaping support. The existing
keyboard mapping remains in place; text editing and clipboard storage keep
complete UTF-8 characters. Ambiguous legacy byte sequences that are also valid
UTF-8 are deliberately interpreted as UTF-8.
