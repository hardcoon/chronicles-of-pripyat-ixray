#include "StdAfx.h"
#include "SleepTimelapse.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "Level.h"

extern bool g_bDisableAllInput;

// COP_ENGINE_PATCH: COP-ENG-185 - this state never owns a CActor/effector pointer.
namespace cop_sleep
{
namespace
{
constexpr ECamEffectorType camera_type = static_cast<ECamEffectorType>(9185);
struct State
{
    bool enabled = false;
    bool previous_input = false;
    u32 generation = 0;
    u16 actor_id = u16(-1);
    u32 refreshed = 0;
    float previous_time_factor = 1.f;
    float sleep_needs = 1.f;
    Fvector position = {};
    Fvector direction = {};
    void release()
    {
        if (!enabled) return;
        enabled = false;
        Device.time_factor(previous_time_factor);
        g_bDisableAllInput = previous_input;
    }
} state;

bool valid_vector(const Fvector& value)
{
    return _valid(value.x) && _valid(value.y) && _valid(value.z)
        && fabsf(value.x) < 1000000.f && fabsf(value.y) < 1000000.f && fabsf(value.z) < 1000000.f;
}

class Camera final : public CEffectorCam
{
    const u32 generation;
public:
    explicit Camera(u32 epoch) : CEffectorCam(camera_type, FLT_MAX), generation(epoch) {}
    ~Camera() override { if (generation == state.generation) state.release(); }
    BOOL Valid() override { return state.enabled && generation == state.generation; }
    bool AbsolutePositioning() override { return true; }
    BOOL ProcessCam(SCamEffectorInfo& info) override
    {
        if (!Valid()) return FALSE;
        auto actor = Actor();
        if (!g_pGameLevel || !actor || actor->ID() != state.actor_id || !actor->g_Alive())
        {
            state.release();
            return FALSE;
        }
        // Unscaled watchdog: the scripted time factor can be hundreds of times faster.
        if (Device.Paused()) state.refreshed = Device.dwTimeContinual;
        if (u32(Device.dwTimeContinual - state.refreshed) > 10000)
        {
            state.release();
            return FALSE;
        }
        info.p.set(state.position);
        info.d.set(state.direction);
        Fvector up; up.set(0.f, 1.f, 0.f);
        if (fabsf(info.d.y) > .98f) up.set(0.f, 0.f, 1.f);
        info.r.crossproduct(up, info.d).normalize_safe();
        info.n.crossproduct(info.d, info.r).normalize_safe();
        return TRUE;
    }
};
}

bool active() { return state.enabled; }
float needs_factor() { return state.enabled ? state.sleep_needs : 1.f; }

bool set(const Fvector& position, const Fvector& direction)
{
    auto actor = Actor();
    if (!g_pGameLevel || !actor || !actor->g_Alive() || !valid_vector(position)
        || !valid_vector(direction) || direction.square_magnitude() < EPS_S) return false;
    if (state.enabled && actor->ID() != state.actor_id) clear();
    if (!state.enabled)
    {
        state.previous_time_factor = Device.time_factor();
        state.previous_input = g_bDisableAllInput;
        state.actor_id = actor->ID();
        state.sleep_needs = pSettings->line_exist("actor_condition", "pf_sleep_needs_factor")
            ? pSettings->r_float("actor_condition", "pf_sleep_needs_factor") : 1.f;
        if (!_valid(state.sleep_needs)) state.sleep_needs = 1.f;
        clamp(state.sleep_needs, 0.f, 1.f);
        ++state.generation;
        state.enabled = true;
        actor->Cameras().AddCamEffector(new Camera(state.generation));
    }
    state.position.set(position);
    state.direction.set(direction).normalize_safe();
    state.refreshed = Device.dwTimeContinual;
    return true;
}

void clear()
{
    state.release();
    if (g_pGameLevel && Actor()) Actor()->Cameras().RemoveCamEffector(camera_type);
}
}
