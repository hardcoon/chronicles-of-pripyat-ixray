#pragma once

// COP_ENGINE_PATCH: COP-ENG-239 - a save and its Lua state commit together.
// A durable rollback journal makes an interrupted two-file commit recoverable.
// This header is also compiled directly by the native failure-injection tests.
#include <Windows.h>
#include <cstdint>
#include <string>
#include "save_path251.h"

namespace save_pair239
{
struct Paths
{
    std::string main, data, staged_main, staged_data, backup_main, backup_data, journal;
    explicit Paths(const char* name) : main(name), data(main + "_data"),
        staged_main(main + ".pf64_pending"), staged_data(data + ".pf64_pending"),
        backup_main(main + ".pf64_previous"), backup_data(data + ".pf64_previous"),
        journal(main + ".pf64_transaction") {}
};

struct Journal
{
    uint32_t magic = 0x39333250; // P239
    uint32_t version = 1;
    uint32_t old_files = 0;
    uint32_t check = 0;
};

struct Files
{
    static bool exists(const std::string& name)
    {
        const DWORD attrs = GetFileAttributesW(save_path251::wide(name).c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
    }
    static bool remove(const std::string& name)
    {
        if (DeleteFileW(save_path251::wide(name).c_str())) return true;
        return GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND;
    }
    static bool sync(const std::string& name)
    {
        HANDLE file = CreateFileW(save_path251::wide(name).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        const bool ok = FlushFileBuffers(file) != FALSE;
        CloseHandle(file);
        return ok;
    }
    static bool copy(const std::string& from, const std::string& to)
    {
        return CopyFileW(save_path251::wide(from).c_str(), save_path251::wide(to).c_str(), FALSE) && sync(to);
    }
    static bool move(const std::string& from, const std::string& to)
    {
        return MoveFileExW(save_path251::wide(from).c_str(), save_path251::wide(to).c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    }
    static bool write_save(const std::string& name, const uint32_t (&header)[3], const void* data, uint32_t size)
    {
        HANDLE file = CreateFileW(save_path251::wide(name).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        DWORD bytes = 0;
        bool ok = WriteFile(file, header, sizeof(header), &bytes, nullptr) && bytes == sizeof(header);
        if (ok) ok = WriteFile(file, data, size, &bytes, nullptr) && bytes == size;
        if (ok) ok = FlushFileBuffers(file) != FALSE;
        const bool closed = CloseHandle(file) != FALSE;
        if (!ok || !closed) { remove(name); return false; }
        return true;
    }
    static bool write_journal(const std::string& name, Journal value)
    {
        value.check = value.magic ^ value.version ^ value.old_files ^ 0x64A239C1;
        const std::string pending = name + ".init";
        HANDLE file = CreateFileW(save_path251::wide(pending).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        DWORD bytes = 0;
        const bool ok = WriteFile(file, &value, sizeof(value), &bytes, nullptr) && bytes == sizeof(value) && FlushFileBuffers(file);
        CloseHandle(file);
        if (!ok) { remove(pending); return false; }
        return move(pending, name);
    }
    static bool read_journal(const std::string& name, Journal& value)
    {
        HANDLE file = CreateFileW(save_path251::wide(name).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        DWORD bytes = 0;
        const bool ok = GetFileSize(file, nullptr) == sizeof(value) &&
            ReadFile(file, &value, sizeof(value), &bytes, nullptr) && bytes == sizeof(value);
        CloseHandle(file);
        return ok && value.magic == 0x39333250 && value.version == 1 && !(value.old_files & ~3u) &&
            value.check == (value.magic ^ value.version ^ value.old_files ^ 0x64A239C1);
    }
};

template<class IO = Files>
bool recover(const Paths& paths)
{
    if (!IO::exists(paths.journal)) return true;
    Journal journal;
    if (!IO::read_journal(paths.journal, journal)) return false;
    // Keep both backups and the journal until both previous members are restored.
    const bool main_ok = (journal.old_files & 1) ? IO::copy(paths.backup_main, paths.main) : IO::remove(paths.main);
    const bool data_ok = (journal.old_files & 2) ? IO::copy(paths.backup_data, paths.data) : IO::remove(paths.data);
    if (!main_ok || !data_ok || !IO::remove(paths.journal)) return false;
    IO::remove(paths.backup_main);
    IO::remove(paths.backup_data);
    return true;
}

template<class IO = Files>
bool prepare(const Paths& paths)
{
    return recover<IO>(paths) && IO::remove(paths.staged_main) && IO::remove(paths.staged_data);
}

template<class IO = Files>
bool commit(const Paths& paths, bool has_data)
{
    if (!IO::exists(paths.staged_main) || (has_data && !IO::exists(paths.staged_data))) return false;
    if (!IO::sync(paths.staged_main) || (has_data && !IO::sync(paths.staged_data))) return false;
    Journal journal;
    if (IO::exists(paths.main)) journal.old_files |= 1;
    if (IO::exists(paths.data)) journal.old_files |= 2;
    if ((journal.old_files & 1) && !IO::copy(paths.main, paths.backup_main)) return false;
    if ((journal.old_files & 2) && !IO::copy(paths.data, paths.backup_data)) return false;
    if (!IO::write_journal(paths.journal, journal)) return false;
    if (!IO::move(paths.staged_main, paths.main) ||
        !(has_data ? IO::move(paths.staged_data, paths.data) : IO::remove(paths.data)))
    {
        recover<IO>(paths);
        return false;
    }
    // The new pair is complete. A crash before journal removal rolls back both.
    if (!IO::remove(paths.journal)) { recover<IO>(paths); return false; }
    IO::remove(paths.backup_main);
    IO::remove(paths.backup_data);
    return true;
}
}
