#ifndef COP_USER_MUSIC_STANDALONE
#include "stdafx.h"
#include "Sound.h"
#endif
#include "UserMusic183.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#define DRMP3_API static
#include "thirdparty/dr_mp3.h"

// COP_ENGINE_PATCH: COP-ENG-183 - bounded streaming, Unicode filenames, lazy metadata.
namespace {
constexpr unsigned max_tracks = 4096;
constexpr std::uint64_t max_file_bytes = 256ull * 1024 * 1024;
constexpr char token_prefix[] = "@usermusic\\";
struct Track {
    struct Frame { std::uint64_t byte, frame; };
    struct Seek { std::uint64_t byte, frame, ready_frame; std::vector<Frame> preroll; };
    std::wstring path;
    std::string title, token;
    unsigned rate = 0;
    std::uint64_t frames = 0;
    std::vector<Seek> seek;
    std::shared_ptr<std::atomic<bool>> invalid = std::make_shared<std::atomic<bool>>(false);
};
std::wstring music_root;
std::once_flag catalog_once;
std::mutex metadata_mutex;
std::vector<Track> tracks;

std::string utf8(const std::wstring& value) {
    int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), int(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    if (size) WideCharToMultiByte(CP_UTF8, 0, value.c_str(), int(value.size()), &result[0], size, nullptr, nullptr);
    return result;
}
void scan() {
    if (music_root.empty()) return;
    WIN32_FIND_DATAW entry{};
    HANDLE search = FindFirstFileW((music_root + L"*").c_str(), &entry);
    if (search == INVALID_HANDLE_VALUE) return;
    do {
        if (entry.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) continue;
        std::wstring name(entry.cFileName);
        if (name.size() < 5 || _wcsicmp(name.c_str() + name.size() - 4, L".mp3")) continue;
        std::uint64_t bytes = (std::uint64_t(entry.nFileSizeHigh) << 32) | entry.nFileSizeLow;
        if (!bytes || bytes > max_file_bytes) continue;
        Track track;
        track.path = music_root + name;
        track.title = utf8(name.substr(0, name.size() - 4));
        tracks.push_back(std::move(track));
        if (tracks.size() == max_tracks) break;
    } while (FindNextFileW(search, &entry));
    FindClose(search);
    std::sort(tracks.begin(), tracks.end(), [](const Track& a, const Track& b) { return a.path < b.path; });
    for (unsigned i = 0; i < tracks.size(); ++i) {
        char token[40];
        std::snprintf(token, sizeof(token), "%s%08u", token_prefix, i);
        tracks[i].token = token;
    }
}
Track* find_track(const char* token) {
    if (!token || std::strlen(token) != sizeof(token_prefix) - 1 + 8 ||
        std::strncmp(token, token_prefix, sizeof(token_prefix) - 1)) return nullptr;
    unsigned index = 0;
    for (const char* p = token + sizeof(token_prefix) - 1; *p; ++p) {
        if (*p < '0' || *p > '9') return nullptr;
        index = index * 10 + unsigned(*p - '0');
    }
    std::call_once(catalog_once, scan);
    return index < tracks.size() ? &tracks[index] : nullptr;
}
size_t file_read(void* file, void* output, size_t bytes) { return std::fread(output, 1, bytes, static_cast<FILE*>(file)); }
drmp3_bool32 file_seek(void* file, int offset, drmp3_seek_origin origin) {
    return _fseeki64(static_cast<FILE*>(file), offset,
        origin == DRMP3_SEEK_SET ? SEEK_SET : origin == DRMP3_SEEK_CUR ? SEEK_CUR : SEEK_END) == 0;
}
drmp3_bool32 file_tell(void* file, drmp3_int64* position) {
    *position = _ftelli64(static_cast<FILE*>(file));
    return *position >= 0;
}
}
struct UserMusic183Decoder {
    drmp3 mp3{};
    FILE* file = nullptr;
    Track* track = nullptr;
    bool initialized = false;
    std::uint64_t next_frame = 0;
    ~UserMusic183Decoder() {
        if (initialized) drmp3_uninit(&mp3);
        if (file) std::fclose(file);
    }
};
void UserMusic183_SetRoot(const char* root) {
    // Called once by xrSound initialization, before any Lua or audio-thread access.
    if (!music_root.empty() || !root) return;
    int size = MultiByteToWideChar(CP_ACP, 0, root, -1, nullptr, 0);
    if (size <= 1) return;
    std::wstring path(size, L'\0');
    MultiByteToWideChar(CP_ACP, 0, root, -1, &path[0], size);
    path.resize(size - 1);
    if (path.back() != L'\\' && path.back() != L'/') path += L'\\';
    music_root = path + L"MyMusic\\";
}
unsigned xrUserMusic183_Count() { std::call_once(catalog_once, scan); return unsigned(tracks.size()); }
const char* xrUserMusic183_Name(unsigned index) { return index < xrUserMusic183_Count() ? tracks[index].title.c_str() : ""; }
const char* xrUserMusic183_Token(unsigned index) { return index < xrUserMusic183_Count() ? tracks[index].token.c_str() : ""; }
bool xrUserMusic183_Valid(const char* token) {
    std::lock_guard<std::mutex> lock(metadata_mutex);
    Track* track = find_track(token);
    return track && !track->invalid->load();
}
bool UserMusic183_IsToken(const char* name) {
    return name && !std::strncmp(name, token_prefix, sizeof(token_prefix) - 1);
}
UserMusic183Decoder* UserMusic183_Open(const char* name) {
    Track* track = find_track(name);
    if (!track || track->invalid->load()) return nullptr;
    std::unique_ptr<UserMusic183Decoder> result(new UserMusic183Decoder);
    result->track = track;
    result->file = _wfopen(track->path.c_str(), L"rb");
    if (result->file && drmp3_init(&result->mp3, file_read, file_seek, file_tell, nullptr, result->file, nullptr)) {
        result->initialized = true;
        if ((result->mp3.channels == 1 || result->mp3.channels == 2) &&
            result->mp3.sampleRate >= 8000 && result->mp3.sampleRate <= 48000) {
            return result.release();
        }
    }
    // Failure is cached; a broken/missing file is never retried every frame.
    track->invalid->store(true);
    return nullptr;
}
void UserMusic183_Close(UserMusic183Decoder*& decoder) { delete decoder; decoder = nullptr; }
bool UserMusic183_Info(const char* name, unsigned& rate, std::uint64_t& frames) {
    std::lock_guard<std::mutex> lock(metadata_mutex);
    Track* track = find_track(name);
    if (!track || track->invalid->load()) return false;
    if (!track->frames) {
        std::unique_ptr<UserMusic183Decoder> decoder(UserMusic183_Open(name));
        if (!decoder) return false;
        auto& mp3 = decoder->mp3;
        track->rate = mp3.sampleRate;
        // Index only the requested song's compressed frame headers. Store at most
        // 129 byte/sample positions with a 16-frame decoder preroll. No whole-song PCM.
        if (!drmp3_seek_to_pcm_frame(&mp3, 0)) { track->invalid->store(true); return false; }
        std::array<Track::Frame, 16> prior{};
        unsigned seen = 0;
        std::uint64_t raw_frames = 0, next_byte = 0;
        const std::uint64_t step = (std::max)(std::uint64_t(1), std::uint64_t(mp3.streamLength / 128));
        for (;;) {
            Track::Frame point{mp3.streamCursor - mp3.dataSize, raw_frames};
            unsigned samples = drmp3_decode_next_frame_ex(&mp3, nullptr, nullptr, nullptr);
            if (!samples) break;
            if (mp3.mp3FrameSampleRate != track->rate || mp3.mp3FrameChannels != mp3.channels) {
                track->invalid->store(true); return false;
            }
            prior[seen % prior.size()] = point;
            ++seen;
            if (point.byte >= next_byte && track->seek.size() < 129) {
                auto previous = prior[seen < prior.size() ? 0 : seen % prior.size()];
                if (track->seek.empty() || track->seek.back().frame != previous.frame) {
                    Track::Seek seek{previous.byte, previous.frame, point.frame, {}};
                    const unsigned available = (std::min)(seen, unsigned(prior.size()));
                    for(unsigned i=0;i<available;++i) seek.preroll.push_back(prior[(seen-available+i)%prior.size()]);
                    track->seek.push_back(std::move(seek));
                }
                next_byte = point.byte + step;
            }
            raw_frames += samples;
            if (raw_frames > std::uint64_t(track->rate) * 4 * 60 * 60) { track->invalid->store(true); return false; }
        }
        const auto trim = std::uint64_t(mp3.delayInPCMFrames) + mp3.paddingInPCMFrames;
        track->frames = raw_frames > trim ? raw_frames - trim : 0;
        // xrSound byte cursors are u32. Reject impossible or malicious duration tags.
        if (!track->frames || track->frames > 0x7fffffffULL || track->frames > std::uint64_t(track->rate) * 4 * 60 * 60) {
            track->invalid->store(true);
            return false;
        }
    }
    rate = track->rate; frames = track->frames;
    return true;
}
void UserMusic183_Read(UserMusic183Decoder* decoder, std::uint64_t frame, short* output, unsigned frames) {
    std::memset(output, 0, size_t(frames) * sizeof(short));
    if (!decoder) return;
    auto& mp3 = decoder->mp3;
    if (decoder->next_frame != frame) {
        // The index includes actual stream offsets, encoder delay and preroll.
        // Starting at byte zero would incorrectly include the Xing/Info frame.
        const auto raw_frame = frame + mp3.delayInPCMFrames;
        const Track::Seek* selected = nullptr;
        for (const auto& candidate : decoder->track->seek) {
            if (candidate.ready_frame > raw_frame) break;
            selected = &candidate;
        }
        const auto byte = selected ? selected->byte : mp3.streamStartOffset;
        if (!drmp3__on_seek_64(&mp3, byte, DRMP3_SEEK_SET)) return;
        drmp3_reset(&mp3);
        mp3.currentPCMFrame = selected ? selected->frame : 0;
        // A fresh MP3 decoder may discard leading reservoir-dependent frames.
        // Identify the first successfully decoded header, keeping its real sample coordinate.
        drmp3dec_frame_info first{};
        if (!drmp3_decode_next_frame_ex(&mp3, reinterpret_cast<drmp3d_sample_t*>(mp3.pcmFrames), &first, nullptr)) return;
        const auto first_byte = mp3.streamCursor - mp3.dataSize - first.frame_bytes;
        bool located = first_byte == byte;
        if (selected) for (const auto& header : selected->preroll) if(header.byte == first_byte) {
            mp3.currentPCMFrame = header.frame; located = true; break;
        }
        if (!located || mp3.currentPCMFrame > raw_frame) return;
        const auto discard = raw_frame - (std::max)(mp3.currentPCMFrame, std::uint64_t(mp3.delayInPCMFrames));
        if (drmp3_read_pcm_frames_s16(&mp3, discard, nullptr) != discard) return;
    }
    std::array<short, 2048> samples;
    unsigned done = 0;
    while (done < frames) {
        unsigned requested = (std::min)(frames - done, 1024u);
        unsigned read = unsigned(drmp3_read_pcm_frames_s16(&mp3, requested, samples.data()));
        if (!read) break;
        for (unsigned i = 0; i < read; ++i)
            output[done + i] = mp3.channels == 1 ? samples[i] : short((int(samples[i * 2]) + int(samples[i * 2 + 1])) / 2);
        done += read;
    }
    decoder->next_frame = frame + done;
    if (done < frames && decoder->next_frame < decoder->track->frames)
        decoder->track->invalid->store(true);
}
