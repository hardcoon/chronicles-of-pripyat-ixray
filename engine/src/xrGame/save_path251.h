#pragma once
// COP_ENGINE_PATCH: COP-ENG-251 - identical Unicode paths for both save members.
// File payloads and the transaction protocol are unchanged. Accept old CP1251 inputs.
#include <Windows.h>
#include <limits>
#include <string>
#include "../xrCore/UTF8Text.h"
namespace save_path251
{
inline std::wstring wide(const std::string& name)
{
    const auto utf8 = UTF8Text::Normalize(name);
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, nullptr, 0);
    if (!size) return {};
    std::wstring result(size, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, result.data(), size)) return {};
    result.pop_back();
    return result;
}
inline bool write(const std::string& name, const void* data, size_t size)
{
    if (size > (std::numeric_limits<DWORD>::max)()) return false;
    const auto path = wide(name);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const bool ok = WriteFile(file, data, static_cast<DWORD>(size), &written, nullptr) && written == size && FlushFileBuffers(file);
    const bool closed = CloseHandle(file) != FALSE;
    if (!ok || !closed) DeleteFileW(path.c_str());
    return ok && closed;
}
inline bool read(const std::string& name, std::string& data)
{
    const auto path = wide(name);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER length;
    if (!GetFileSizeEx(file, &length) || length.QuadPart < 0 || length.QuadPart > (std::numeric_limits<int>::max)())
    { CloseHandle(file); return false; }
    data.resize(static_cast<size_t>(length.QuadPart));
    DWORD count = 0;
    const bool ok = ReadFile(file, data.data(), static_cast<DWORD>(data.size()), &count, nullptr) && count == data.size();
    const bool closed = CloseHandle(file) != FALSE;
    if (!ok || !closed) data.clear();
    return ok && closed;
}
// At config restoration only, a pre-fix last-save name may refer to the newer
// ANSI-misencoded twin. Menu selection keeps its exact physical filename.
inline std::string restored_last_name(const std::string& name, const std::string& root, const char* extension)
{
    const std::string normalized = UTF8Text::Normalize(name);
    const int count = MultiByteToWideChar(1251, 0, normalized.c_str(), -1, nullptr, 0);
    if (!count) return normalized;
    std::wstring ansi(count, L'\0');
    MultiByteToWideChar(1251, 0, normalized.c_str(), -1, ansi.data(), count);
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, ansi.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (!bytes) return normalized;
    std::string broken(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ansi.c_str(), -1, broken.data(), bytes, nullptr, nullptr);
    broken.pop_back();
    if (broken == normalized) return normalized;
    WIN32_FILE_ATTRIBUTE_DATA original{}, legacy{};
    const bool has_original = GetFileAttributesExW(wide(root + normalized + extension).c_str(), GetFileExInfoStandard, &original) != FALSE;
    const bool has_legacy = GetFileAttributesExW(wide(root + broken + extension).c_str(), GetFileExInfoStandard, &legacy) != FALSE;
    if (has_legacy && !(legacy.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (!has_original || CompareFileTime(&legacy.ftLastWriteTime, &original.ftLastWriteTime) > 0)) return broken;
    return normalized;
}
}
