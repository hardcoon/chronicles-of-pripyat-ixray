#include "stdafx.h"
#include "PFOnlineTv.h"
#include "mpv/client.h"
#include "mpv/render.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>

// COP_ENGINE_PATCH: COP-ENG-194
// Optional libmpv is loaded lazily. No sockets, decoding or child-process waits
// run in the game's frame loop; only nonblocking API calls and a 640x360 upload.
namespace PFOnlineTv
{
namespace
{
struct Player
{
    // COP_ENGINE_PATCH: COP-ENG-195 - async transport/status, no network waits.
    // The actor binder runs on GameThread while the main thread renders.
    // Serialize frame storage and core lifetime; libmpv's wake callback only
    // touches the atomic dirty flag and never takes this lock.
    std::recursive_mutex mutex;
    HMODULE dll = nullptr;
    mpv_handle* core = nullptr;
    mpv_render_context* render = nullptr;
    std::atomic<bool> dirty{false};
    std::vector<unsigned char> frame;
    bool playing = false, paused = false;
    bool changed = true, loggedFrame = false;
    bool frameReady = false; // COP_ENGINE_PATCH: COP-ENG-196 - explicit video/fallback signal.
    u32 servicedFrame = u32(-1);
    u32 heartbeat = 0;
    float gain = 0;
    bool userPaused = false, inhibited = false;
    int state = 0, quality = 360, error = 0;
    unsigned serial = 0;
    double position = 0, duration = 0, resume = 0;
    int64_t entry = -1, width = 0, height = 0;
    std::string url, title;
#define MPV_FUNCTION(name) decltype(&mpv_##name) name = nullptr
    MPV_FUNCTION(create);
    MPV_FUNCTION(set_option_string);
    MPV_FUNCTION(initialize);
    MPV_FUNCTION(command_async);
    MPV_FUNCTION(wait_event);
    MPV_FUNCTION(observe_property);
    MPV_FUNCTION(request_log_messages);
    MPV_FUNCTION(terminate_destroy);
    MPV_FUNCTION(render_context_create);
    MPV_FUNCTION(render_context_set_update_callback);
    MPV_FUNCTION(render_context_update);
    MPV_FUNCTION(render_context_render);
    MPV_FUNCTION(render_context_report_swap);
    MPV_FUNCTION(render_context_free);
#undef MPV_FUNCTION
} p;

void Wake(void*) { p.dirty.store(true, std::memory_order_release); }

std::string BinPath(const char* file)
{
    char path[MAX_PATH] = {};
    const DWORD size = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (!size || size >= MAX_PATH) return {};
    std::string result(path);
    const auto slash = result.find_last_of("\\/");
    if (slash == std::string::npos) return {};
    result.resize(slash + 1);
    result += "pf_tv/";
    result += file;
    for (auto& ch : result) if (ch == '\\') ch = '/';
    return result;
}

bool Option(const char* key, const std::string& value)
{
    const int error = p.set_option_string(p.core, key, value.c_str());
    if (error < 0) Msg("! [PF-TV] option %s failed (%d)", key, error);
    return error >= 0;
}

bool Init()
{
    if (p.core) return true;
    const auto dll = BinPath("libmpv-2.dll");
    if (dll.empty()) return false;
    p.dll = LoadLibraryExA(dll.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!p.dll) { Msg("! [PF-TV] optional player unavailable (%lu)", GetLastError()); return false; }
#define LOAD_MPV(name) p.name = reinterpret_cast<decltype(p.name)>(GetProcAddress(p.dll, "mpv_" #name)); if (!p.name) { Shutdown(); return false; }
    LOAD_MPV(create); LOAD_MPV(set_option_string); LOAD_MPV(initialize);
    LOAD_MPV(command_async); LOAD_MPV(wait_event); LOAD_MPV(observe_property); LOAD_MPV(request_log_messages);
    LOAD_MPV(terminate_destroy); LOAD_MPV(render_context_create);
    LOAD_MPV(render_context_set_update_callback); LOAD_MPV(render_context_update);
    LOAD_MPV(render_context_render); LOAD_MPV(render_context_report_swap);
    LOAD_MPV(render_context_free);
#undef LOAD_MPV
    p.core = p.create();
    if (!p.core) { Shutdown(); return false; }
    bool ok = Option("config", "no") && Option("terminal", "no") &&
        Option("vo", "libmpv") && Option("osc", "no") && Option("osd-level", "0") &&
        Option("input-default-bindings", "no") && Option("load-scripts", "no") &&
        Option("ytdl", "yes") && Option("hwdec", "no") && Option("volume", "0") &&
        Option("cache", "yes") && Option("demuxer-max-bytes", "16MiB") &&
        Option("demuxer-max-back-bytes", "1MiB") && Option("video-timing-offset", "0") &&
        Option("network-timeout", "15") &&
        Option("ytdl-format", "bestvideo[height<=360]+bestaudio/best[height<=360]/best") &&
        Option("script-opts", "ytdl_hook-ytdl_path=" + BinPath("yt-dlp.exe")) &&
        Option("ytdl-raw-options", "js-runtimes=node:" + BinPath("node.exe"));
    if (!ok || p.initialize(p.core) < 0) { Shutdown(); return false; }
    p.request_log_messages(p.core, "warn");
    p.observe_property(p.core, 1, "time-pos", MPV_FORMAT_DOUBLE);
    p.observe_property(p.core, 2, "duration", MPV_FORMAT_DOUBLE);
    p.observe_property(p.core, 3, "media-title", MPV_FORMAT_STRING);
    p.observe_property(p.core, 4, "width", MPV_FORMAT_INT64);
    p.observe_property(p.core, 5, "height", MPV_FORMAT_INT64);
    mpv_render_param params[] = {{MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_SW)}, {MPV_RENDER_PARAM_INVALID, nullptr}};
    if (p.render_context_create(&p.render, p.core, params) < 0) { Shutdown(); return false; }
    p.frame.assign(Width * Height * 4, 0);
    p.render_context_set_update_callback(p.render, Wake, nullptr);
    Msg("* [PF-TV] online player initialized, output %ux%u, bounded memory cache", Width, Height);
    return true;
}

void Async(const char* command, const char* a = nullptr, const char* b = nullptr)
{
    if (!p.core) return;
    const char* args[] = {command, a, b, nullptr};
    const int result = p.command_async(p.core, 0, args);
    if (result < 0) Msg("! [PF-TV] command %s failed (%d)", command, result);
}

bool Allowed(const char* url)
{
    if (!url || strlen(url) > 1500 || strpbrk(url, " \t\r\n\"")) return false;
    for (const char* prefix : {"https://www.youtube.com/", "https://youtube.com/", "https://youtu.be/", "https://rutube.ru/", "https://www.rutube.ru/"})
        if (strncmp(url, prefix, strlen(prefix)) == 0) return true;
    return false;
}
}

bool IsTexture(const char* name) { return name && (_stricmp(name, "pf_hf_video\\tv_online") == 0 || _stricmp(name, "pf_hf_video/tv_online") == 0); }

void Stop()
{
    std::lock_guard<std::recursive_mutex> lock(p.mutex);
    p.frameReady = false;
    if (p.core) { Async("set", "volume", "0"); Async("stop"); }
    p.playing = false;
    p.state = 0; p.position = 0; p.userPaused = false; p.gain = 0;
    p.frame.assign(Width * Height * 4, 0);
    p.changed = true;
}

// COP_ENGINE_PATCH: COP-ENG-195 - bounded, read-only snapshot for Lua via console.
void Status(char* output, unsigned size)
{
    std::lock_guard<std::recursive_mutex> lock(p.mutex);
    char title[161] = {}; const char* hex = "0123456789abcdef";
    size_t count = std::min(size_t(80), p.title.size());
    while (count && count < p.title.size() && (static_cast<unsigned char>(p.title[count]) & 0xc0) == 0x80) --count;
    for (size_t i = 0; i < count; ++i) { const auto ch = static_cast<unsigned char>(p.title[i]); title[2*i] = hex[ch>>4]; title[2*i+1] = hex[ch&15]; }
    _snprintf_s(output, size, _TRUNCATE, "TV2 %u %d %.2f %.2f %lld %lld %d %s", p.serial, p.state, p.position, p.duration, p.width, p.height, p.error, title);
}

void Command(const char* args)
{
    std::lock_guard<std::recursive_mutex> lock(p.mutex);
    if (!args) return;
    if (strncmp(args, "play ", 5) == 0)
    {
        const char* url = args + 5;
        if (!Allowed(url)) { Msg("! [PF-TV] expected an HTTPS YouTube or Rutube link"); return; }
        if (p.inhibited) return;
        if (!Init()) { p.state = 5; p.error = -1; ++p.serial; return; }
        p.url = url; p.title.clear(); ++p.serial; p.state = 1; p.error = 0;
        p.position = p.duration = p.resume = 0; p.width = p.height = 0; p.entry = -1;
        p.userPaused = false;
        p.playing = true; p.loggedFrame = false; p.paused = false; p.frameReady = false;
        p.frame.assign(Width * Height * 4, 0); p.changed = true;
        p.heartbeat = GetTickCount();
        Async("set", "pause", "no");
        Async("loadfile", url, "replace");
        Msg("* [PF-TV] loading online video");
    }
    else if (strcmp(args, "stop") == 0) { Stop(); Msg("* [PF-TV] stopped"); }
    else if (strcmp(args, "reset") == 0) { p.inhibited = true; Stop(); }
    else if (strcmp(args, "ready") == 0) { p.inhibited = false; }
    else if (strncmp(args, "pause ", 6) == 0)
    {
        if (strcmp(args + 6, "0") && strcmp(args + 6, "1")) return;
        p.userPaused = args[6] == '1';
        if (p.playing) { p.paused = Device.Paused() || p.userPaused; Async("set", "pause", p.paused ? "yes" : "no"); if (p.state != 1) p.state = p.userPaused ? 3 : 2; }
    }
    else if (strncmp(args, "seek ", 5) == 0)
    {
        double seconds = 0; char tail = 0;
        if (sscanf(args + 5, "%lf %c", &seconds, &tail) != 1 || !std::isfinite(seconds) || seconds < 0 || seconds > 864000) return;
        if (p.playing && p.duration > 0) { char value[32]; sprintf_s(value, "%.3f", std::min(seconds, p.duration)); Async("seek", value, "absolute+exact"); }
    }
    else if (strncmp(args, "quality ", 8) == 0)
    {
        int quality = 0; char tail = 0;
        if (sscanf(args + 8, "%d %c", &quality, &tail) != 1) return;
        if (quality != 0 && quality != 144 && quality != 240 && quality != 360 && quality != 480 && quality != 720 && quality != 1080) return;
        if (!Init()) return;
        p.quality = quality;
        char format[160];
        if (quality) sprintf_s(format, "bestvideo[height<=%d]+bestaudio/best[height<=%d]/best", quality, quality);
        else strcpy_s(format, "bestvideo+bestaudio/best");
        Async("set", "ytdl-format", format);
        if (p.playing && !p.url.empty()) { p.resume = p.position; p.state = 1; p.entry = -1; p.frameReady = false; Async("loadfile", p.url.c_str(), "replace"); }
    }
    else if (strncmp(args, "gain ", 5) == 0)
    {
        float gain = 0; char tail = 0;
        if (sscanf(args + 5, "%f %c", &gain, &tail) != 1 || !std::isfinite(gain) || gain < 0 || gain > 100) return;
        p.heartbeat = GetTickCount();
        if (p.core && fabsf(gain - p.gain) > .05f)
        {
            char value[32]; sprintf_s(value, "%.3f", gain);
            Async("set", "volume", value); p.gain = gain;
        }
    }
    else Msg("* [PF-TV] pf_tv play <HTTPS URL> | stop | gain <0..100>");
}

void Tick()
{
    std::lock_guard<std::recursive_mutex> lock(p.mutex);
    if (!p.core || p.servicedFrame == Device.dwFrame) return;
    p.servicedFrame = Device.dwFrame;
    const bool paused = Device.Paused() || p.userPaused;
    if (p.playing && p.paused != paused)
    {
        Async("set", "pause", paused ? "yes" : "no");
        p.paused = paused;
        p.heartbeat = GetTickCount(); // Resume must allow the Lua owner to refresh its heartbeat.
    }
    // Stop orphaned playback if the Lua owner disappears (unload/destroy/crash).
    if (p.playing && !paused && GetTickCount() - p.heartbeat > 3000) Stop();
    for (int count = 0; count < 16; ++count)
    {
        const auto* event = p.wait_event(p.core, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) break;
        if (event->event_id == MPV_EVENT_START_FILE)
        {
            const auto* start = static_cast<const mpv_event_start_file*>(event->data);
            if (start) p.entry = start->playlist_entry_id;
        }
        if (event->event_id == MPV_EVENT_FILE_LOADED && p.playing)
        {
            p.state = p.userPaused ? 3 : 2;
            if (p.resume > 0) { char value[32]; sprintf_s(value, "%.3f", p.resume); Async("seek", value, "absolute+exact"); p.resume = 0; }
            Msg("* [PF-TV] online stream opened");
        }
        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE && p.playing)
        {
            const auto* property = static_cast<const mpv_event_property*>(event->data);
            if (property && property->data)
            {
                if (property->format == MPV_FORMAT_DOUBLE)
                {
                    const double value = *static_cast<double*>(property->data);
                    if (std::isfinite(value) && value >= 0) { if (event->reply_userdata == 1) p.position = value; else if (event->reply_userdata == 2) p.duration = value; }
                }
                else if (property->format == MPV_FORMAT_STRING && event->reply_userdata == 3) { const char* value = *static_cast<char**>(property->data); if (value) p.title.assign(value, std::min(strlen(value), size_t(240))); }
                else if (property->format == MPV_FORMAT_INT64) { if (event->reply_userdata == 4) p.width = *static_cast<int64_t*>(property->data); else if (event->reply_userdata == 5) p.height = *static_cast<int64_t*>(property->data); }
            }
        }
        if (event->event_id == MPV_EVENT_END_FILE)
        {
            const auto* end = static_cast<const mpv_event_end_file*>(event->data);
            Msg("* [PF-TV] playback ended, reason=%d error=%d", end ? int(end->reason) : -1, end ? end->error : event->error);
            // A replaced stream can end while another load is pending; only errors
            // and EOF blank the display. Explicit stop already cleared it.
            if (p.playing && end && end->playlist_entry_id == p.entry && (end->reason == MPV_END_FILE_REASON_ERROR || end->reason == MPV_END_FILE_REASON_EOF))
            { p.playing = false; p.frameReady = false; p.state = end->reason == MPV_END_FILE_REASON_EOF ? 4 : 5; p.error = end->error; p.frame.assign(Width * Height * 4, 0); p.changed = true; }
        }
        if (event->event_id == MPV_EVENT_LOG_MESSAGE)
        {
            const auto* log = static_cast<const mpv_event_log_message*>(event->data);
            // Never copy temporary CDN query tokens or a full stream URL into logs.
            if (log && log->text && !strstr(log->text, "://")) Msg("! [PF-TV] %.400s", log->text);
        }
    }
    if (!p.dirty.exchange(false, std::memory_order_acquire)) return;
    const auto flags = p.render_context_update(p.render);
    if (!(flags & MPV_RENDER_UPDATE_FRAME)) return;
    int size[] = {Width, Height}, block = 0;
    size_t stride = Width * 4;
    mpv_render_param params[] = {{MPV_RENDER_PARAM_SW_SIZE, size}, {MPV_RENDER_PARAM_SW_FORMAT, const_cast<char*>("bgr0")},
        {MPV_RENDER_PARAM_SW_STRIDE, &stride}, {MPV_RENDER_PARAM_SW_POINTER, p.frame.data()},
        {MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, &block}, {MPV_RENDER_PARAM_INVALID, nullptr}};
    if (p.render_context_render(p.render, params) >= 0)
    {
        p.changed = true;
        if (p.playing && (p.state == 2 || p.state == 3) && p.width > 0 && p.height > 0)
        {
            p.frameReady = true;
            if (!p.loggedFrame) { Msg("* [PF-TV] first online video frame decoded"); p.loggedFrame = true; }
        }
    }
    p.render_context_report_swap(p.render);
}

bool CopyFrame(void* target, unsigned stride)
{
    std::lock_guard<std::recursive_mutex> lock(p.mutex);
    if (stride < Width * 4 || !target) return false;
    if (p.frame.empty()) p.frame.assign(Width * Height * 4, 0);
    // COP_ENGINE_PATCH: COP-ENG-196 - BGR0's unused byte is our availability flag.
    // A genuinely black video frame remains valid; colour is never used to
    // detect an empty player. Pause preserves the last frame, stop clears it.
    const unsigned char alpha = p.frameReady && p.playing ? 255 : 0;
    for (unsigned row = 0; row < Height; ++row)
    {
        auto* out = static_cast<unsigned char*>(target) + row * stride;
        memcpy(out, p.frame.data() + row * Width * 4, Width * 4);
        for (unsigned x = 3; x < Width * 4; x += 4) out[x] = alpha;
    }
    p.changed = false;
    return true;
}

void Shutdown()
{
    std::lock_guard<std::recursive_mutex> lock(p.mutex);
    if (p.render) { p.render_context_set_update_callback(p.render, nullptr, nullptr); p.render_context_free(p.render); p.render = nullptr; }
    if (p.core) { p.terminate_destroy(p.core); p.core = nullptr; }
    if (p.dll) { FreeLibrary(p.dll); p.dll = nullptr; }
    p.playing = false; p.frameReady = false; p.dirty.store(false); std::vector<unsigned char>().swap(p.frame); p.gain = 0;
}
}
