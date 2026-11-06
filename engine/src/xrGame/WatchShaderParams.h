#pragma once
// COP_ENGINE_PATCH: COP-ENG-172 - fixed watch-only Lua/render data exchange.
#include <array>
#include <cmath>
#include <cstring>
#include <mutex>

namespace cop_watch
{
constexpr unsigned version = 172;
constexpr unsigned count = 16;
class Params
{
    std::array<float, count> values{{0,0,0,0, 1,1,0,1, 1,1,1,1, .1f,.9f,.25f,0}};
    std::mutex mutex;
public:
    bool set(const char* name, float x, float y, float z, float w)
    {
        if (!name || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(w))
            return false;
        unsigned index;
        if (!std::strcmp(name,"game_time")) index=0;
        else if (!std::strcmp(name,"watch_actor_params_1")) index=4;
        else if (!std::strcmp(name,"watch_actor_params_2")) index=8;
        else if (!std::strcmp(name,"watch_color_params")) index=12;
        else return false;
        // The four registered vectors are the complete protocol. No allocation,
        // map insertion, shader reload or file access occurs during updates.
        std::lock_guard<std::mutex> lock(mutex);
        values[index]=x;values[index+1]=y;values[index+2]=z;values[index+3]=w;
        return true;
    }
    bool snapshot(unsigned requested, float* out, unsigned floats)
    {
        if (requested!=version || !out || floats!=count) return false;
        std::lock_guard<std::mutex> lock(mutex);
        std::memcpy(out,values.data(),sizeof(values));
        return true;
    }
};
}
