#include "StdAfx.h"
#include "Weapon.h"
#include "Level.h"

namespace
{
constexpr LPCSTR section = "hike_weapon_inertia";

float read_value(LPCSTR name, float fallback, float low, float high)
{
    const float value = READ_IF_EXISTS(pSettings, r_float, section, name, fallback);
    return std::isfinite(value) ? weapon_hike::bounded(value, low, high) : fallback;
}

void read_vector4(LPCSTR name, float (&values)[4], float limit)
{
    if (!pSettings->line_exist(section, name)) return;
    const Fvector4 v = pSettings->r_fvector4(section, name);
    const float data[] = {v.x, v.y, v.z, v.w};
    for (int i = 0; i < 4; ++i)
        if (std::isfinite(data[i])) values[i] = weapon_hike::bounded(data[i], -limit, limit);
}

void read_vector2(LPCSTR name, float (&values)[2], float limit)
{
    if (!pSettings->line_exist(section, name)) return;
    const Fvector2 v = pSettings->r_fvector2(section, name);
    if (std::isfinite(v.x)) values[0] = weapon_hike::bounded(v.x, -limit, limit);
    if (std::isfinite(v.y)) values[1] = weapon_hike::bounded(v.y, -limit, limit);
}
}

// COP_ENGINE_PATCH: COP-ENG-224 - one default for every firearm/launcher,
// independent of individual hud_base inheritance; devices keep their own HUD.
void CWeapon::LoadHikeInertia()
{
    m_hike_inertia_enabled = !cast_weapon_knife() && !cast_weapon_binoculars() &&
        READ_IF_EXISTS(pSettings, r_bool, section, "enabled", true) &&
        READ_IF_EXISTS(pSettings, r_bool, hud_sect, "hike_inertion_enabled", true);
    if (!m_hike_inertia_enabled) return;

    auto& h = m_hike_inertia_settings.hip;
    auto& a = m_hike_inertia_settings.aim;
    read_vector4("inertion_offset_LRUD", h.offset, .1f);
    read_vector4("inertion_offset_LRUD_aim", a.offset, .1f);
    read_vector4("inertion_rot_LRUD", h.rotation, 20.f);
    read_vector4("inertion_rot_LRUD_aim", a.rotation, 20.f);
    read_vector2("inertion_rot_FB", h.bank, 20.f);
    read_vector2("inertion_rot_FB_aim", a.bank, 20.f);
    h.response = read_value("inertion_tendto_speed_swm", h.response, 0.f, 10.f);
    a.response = read_value("inertion_aim_tendto_speed_swm", a.response, 0.f, 10.f);
    h.returnSpeed = read_value("inertion_tendto_ret_speed", h.returnSpeed, .01f, 30.f);
    a.returnSpeed = read_value("inertion_tendto_ret_aim_speed", a.returnSpeed, .01f, 30.f);
    h.threshold = read_value("inertion_min_angle", h.threshold, 0.f, 10.f);
    a.threshold = read_value("inertion_min_angle_aim", a.threshold, 0.f, 10.f);
    m_hike_inertia_settings.transitionTime = read_value("inertion_transition_time", .6f, .01f, 5.f);

    // OGSR adds -OriginOffset, IX-Ray adds +OriginOffset. Convert the sign;
    // do not stack IX-Ray's large old lag/pitch bias on top of Hike's motion.
    m_base_inertion.OriginOffset = -read_value("inertion_origin_offset", -.0001f, -.1f, .1f);
    m_zoom_inertion.OriginOffset = -read_value("inertion_zoom_origin_offset", 0.f, -.1f, .1f);
    m_base_inertion.TendtoSpeed = read_value("inertion_tendto_speed", 6.f, .1f, 30.f);
    m_zoom_inertion.TendtoSpeed = read_value("inertion_zoom_tendto_speed", 6.f, .1f, 30.f);
    m_base_inertion.PitchOffsetR = m_zoom_inertion.PitchOffsetR = 0.f;
    m_base_inertion.PitchOffsetD = m_zoom_inertion.PitchOffsetD = 0.f;
    m_base_inertion.PitchOffsetN = m_zoom_inertion.PitchOffsetN = 0.f;
    m_current_inertion = m_base_inertion;
    ResetHikeInertia();
}

void CWeapon::ResetHikeInertia()
{
    m_hike_inertia.reset();
    m_hike_inertia_frame = u32(-1);
}

// COP_ENGINE_PATCH: COP-ENG-224 - only the actor's first-person HUD transform.
// No camera, firing direction, save/net state or NPC transform is modified.
void CWeapon::UpdateHikeInertia(Fmatrix& transform, Fvector& offset, const CActor& actor)
{
    offset.set(0.f, 0.f, 0.f);
    if (!m_hike_inertia_enabled) return;
    if (H_Parent() != Level().CurrentViewEntity() || !actor.HUDview())
    {
        ResetHikeInertia();
        return;
    }
    if (m_hike_inertia_frame != Device.dwFrame)
    {
        if (m_hike_inertia_frame == u32(-1) || Device.dwFrame - m_hike_inertia_frame > 1)
            m_hike_inertia.reset();
        else
            m_hike_inertia.update(Device.fTimeDelta, actor.fFPCamYawMagnitude,
                actor.fFPCamPitchMagnitude, m_zoom_params.m_fZoomRotationFactor, m_hike_inertia_settings);
        m_hike_inertia_frame = Device.dwFrame;
    }
    if (m_hike_inertia.rotation[0] != 0.f || m_hike_inertia.rotation[1] != 0.f || m_hike_inertia.rotation[2] != 0.f)
    {
        const Fvector origin = transform.c;
        Fvector hpb;
        transform.getHPB(hpb);
        transform.setHPB(hpb.x - m_hike_inertia.rotation[0], hpb.y - m_hike_inertia.rotation[1],
            hpb.z - m_hike_inertia.rotation[2]);
        transform.c = origin;
    }
    offset.set(m_hike_inertia.position[0], m_hike_inertia.position[1], m_hike_inertia.position[2]);
}
