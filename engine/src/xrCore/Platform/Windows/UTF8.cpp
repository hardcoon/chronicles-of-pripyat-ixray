#pragma once
#include "stdafx.h"

// COP_ENGINE_PATCH: COP-ENG-248 - preserve UTF-8 and bound Windows conversion buffers.
XRCORE_API wchar_t* Platform::ANSI_TO_TCHAR(const char* C)
{
    static thread_local std::wstring wide;
    const int count = MultiByteToWideChar(CP_UTF8, 0, C, -1, nullptr, 0);
    wide.resize(count > 0 ? count : 1);
    wide[0] = 0;
    if (count > 0) MultiByteToWideChar(CP_UTF8, 0, C, -1, wide.data(), count);
    return wide.data();
}

XRCORE_API xr_string Platform::ANSI_TO_UTF8(const xr_string& ansi)
{
    const std::string utf8 = UTF8Text::Normalize(std::string_view(ansi.data(), ansi.size()));
    return xr_string(utf8.data(), static_cast<u32>(utf8.size()));
}

XRCORE_API xr_string Platform::UTF8_to_CP1251(xr_string const& utf8)
{
	if (!utf8.empty() && IsUTF8(utf8.data()))
	{
		int wchlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), nullptr, 0);
		if (wchlen > 0 && wchlen != 0xFFFD)
		{
			xr_vector<wchar_t> wbuf(wchlen);
			MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), &wbuf[0], wchlen);
			xr_vector<char> buf(wchlen);
			WideCharToMultiByte(1251, 0, &wbuf[0], wchlen, &buf[0], wchlen, 0, 0);

			return xr_string(&buf[0], wchlen);
		}
	}

	return utf8;
}

XRCORE_API wchar_t* Platform::ANSI_TO_TCHAR_U8(const char* C)
{
	if (IsUTF8(C))
		return ANSI_TO_TCHAR(C);

	return ANSI_TO_TCHAR(ANSI_TO_UTF8(C).c_str());
}

namespace
{
xr_string FromWide(const wchar_t* input, unsigned codepage)
{
    const int length = static_cast<int>(std::wcslen(input));
    const int count = WideCharToMultiByte(codepage, 0, input, length, nullptr, 0, nullptr, nullptr);
    xr_string result;
    result.resize(count);
    if (count) WideCharToMultiByte(codepage, 0, input, length, result.data(), count, nullptr, nullptr);
    return result;
}
}

XRCORE_API xr_string Platform::TCHAR_TO_ANSI_U8(const wchar_t* input)
{
    return FromWide(input, 1251); // Explicit legacy boundary retained for existing callers.
}

XRCORE_API xr_string Platform::CP_TCHAR_TO_ANSI_U8(const wchar_t* input)
{
    return FromWide(input, CP_UTF8);
}
