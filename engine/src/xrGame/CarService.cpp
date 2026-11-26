#include "StdAfx.h"
#include "Car.h"
#include "Actor.h"
#include "HUDManager.h"
#include "Inventory.h"
#include "Level.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "eatable_item.h"
#include "pch_script.h"
#include "../xrScripts/script_engine.h"
#include "script_game_object.h"
#include "xrServer_Objects_ALife_Items.h"
#include "../Include/xrRender/Kinematics.h"

// COP_ENGINE_PATCH: COP-ENG-265 - one native service implementation for configured cars.
namespace
{
constexpr float heavy_hit = .10f;
constexpr float part_damage = .25f;
constexpr float parked_speed = .5f;

enum Slot : u8 { Hull, Coolant, Battery, Plugs, Belt };

// COP_ENGINE_PATCH: COP-ENG-273 - shared wear rates are read once from the service add-on.
constexpr pcstr balance_section = "pf_vehicle_service_balance";

float service_wear_per_second(pcstr key, float default_minutes)
{
    const float minutes = READ_IF_EXISTS(pSettings, r_float, balance_section, key, default_minutes);
    const float rate = minutes > 0.f ? (1.f / 60.f) / minutes : 0.f;
    return std::isfinite(rate) && rate > 0.f ? rate : 1.f / (60.f * default_minutes);
}

struct ServiceWearRates
{
    float battery;
    float belt;
    float coolant;
};

const ServiceWearRates& service_wear_rates()
{
    static const ServiceWearRates rates{
        service_wear_per_second("battery_headlight_minutes", 240.f),
        service_wear_per_second("timing_belt_engine_minutes", 200.f),
        service_wear_per_second("coolant_engine_minutes", 120.f)
    };
    return rates;
}

// COP_ENGINE_PATCH: COP-ENG-275 - interpolate wear across each model's RPM range.
struct BeltWearCurve
{
    float idle;
    float low;
    float working;
    float high;
    float low_factor;
    float working_factor;
};

const BeltWearCurve& belt_wear_curve()
{
    static const BeltWearCurve curve = []
    {
        constexpr BeltWearCurve defaults{.05f, .25f, .55f, .85f, .40f, .70f};
        const auto setting = [](pcstr key, float fallback)
        {
            return READ_IF_EXISTS(pSettings, r_float, balance_section, key, fallback);
        };
        const BeltWearCurve configured{
            setting("timing_belt_idle_rpm_fraction", defaults.idle),
            setting("timing_belt_low_rpm_fraction", defaults.low),
            setting("timing_belt_working_rpm_fraction", defaults.working),
            setting("timing_belt_high_rpm_fraction", defaults.high),
            setting("timing_belt_low_wear_factor", defaults.low_factor),
            setting("timing_belt_working_wear_factor", defaults.working_factor)
        };
        if (0.f <= configured.idle && configured.idle < configured.low &&
            configured.low < configured.working && configured.working < configured.high &&
            configured.high <= 1.f && 0.f <= configured.low_factor &&
            configured.low_factor <= configured.working_factor && configured.working_factor <= 1.f)
            return configured;
        return defaults;
    }();
    return curve;
}

float belt_wear_factor(float rpm, float idling_rpm, float maximum_rpm)
{
    if (rpm <= idling_rpm || !(maximum_rpm > idling_rpm) || !std::isfinite(rpm))
        return 0.f;

    const auto& curve = belt_wear_curve();
    const float fraction = (rpm - idling_rpm) / (maximum_rpm - idling_rpm);
    if (fraction <= curve.idle)
        return 0.f;
    const auto interpolate = [fraction](float start_rpm, float end_rpm, float start_factor, float end_factor)
    {
        const float progress = (fraction - start_rpm) / (end_rpm - start_rpm);
        return start_factor + (end_factor - start_factor) * progress;
    };
    if (fraction <= curve.low)
        return interpolate(curve.idle, curve.low, 0.f, curve.low_factor);
    if (fraction <= curve.working)
        return interpolate(curve.low, curve.working, curve.low_factor, curve.working_factor);
    if (fraction < curve.high)
        return interpolate(curve.working, curve.high, curve.working_factor, 1.f);
    return 1.f;
}

void apply_service_time_wear(vehicle_service::State& state, float elapsed, bool lit, bool running, float belt_factor)
{
    const auto& rates = service_wear_rates();
    state.initialized = true;
    if (lit)
        state.battery = _max(0.f, state.battery - elapsed * rates.battery);
    if (!running)
        return;
    state.coolant = _max(0.f, state.coolant - elapsed * rates.coolant);
    if (belt_factor > 0.f)
        state.belt = _max(0.f, state.belt - elapsed * rates.belt * belt_factor);
}

float service_spark_plug_wear(pcstr profile)
{
    struct PlugsClass
    {
        pcstr name;
        pcstr balance_key;
        s32 default_starts;
    };
    constexpr PlugsClass classes[] = {
        {"passenger", "spark_plug_starts_passenger", 300},
        {"offroad", "spark_plug_starts_offroad", 250},
        {"heavy", "spark_plug_starts_heavy", 200},
        {"armored", "spark_plug_starts_armored", 150}
    };
    const pcstr group = READ_IF_EXISTS(pSettings, r_string, profile, "spark_plug_class", "passenger");
    const auto* plug_class = &classes[0];
    for (const auto& entry : classes)
    {
        if (group && !xr_strcmp(group, entry.name))
        {
            plug_class = &entry;
            break;
        }
    }
    const s32 starts = READ_IF_EXISTS(pSettings, r_s32, balance_section, plug_class->balance_key, plug_class->default_starts);
    return 1.f / static_cast<float>(starts > 0 ? starts : plug_class->default_starts);
}

bool can_run_service_engine(const vehicle_service::State& state, bool has_belt)
{
    return (!has_belt || state.belt > 0.f) &&
        (state.coolant > 0.f || state.dry_seconds < vehicle_service::dry_drive_limit);
}

// COP_ENGINE_PATCH: COP-ENG-270 - repair gains come from the vehicle-service add-on.
float service_repair_fraction(pcstr key, float fallback_percent)
{
    const float percent = READ_IF_EXISTS(pSettings, r_float, balance_section, key, fallback_percent);
    if (!std::isfinite(percent) || percent <= 0.f || percent > 100.f)
        return fallback_percent / 100.f;
    return percent / 100.f;
}

CSE_ALifeDynamicObject* server_object(u16 id)
{
    return ai().get_alife() ? ai().alife().objects().object(id, true) : nullptr;
}

CGameObject* carried_object(u16 id)
{
    if (!Actor() || !Actor()->g_Alive() || Actor()->Holder())
        return nullptr;
    auto* object = smart_cast<CGameObject*>(Level().Objects.net_Find(id));
    if (!object || object->H_Parent() != Actor() || object->getDestroy() || object->object_removed())
        return nullptr;
    return object;
}

bool charge_material(const char* section)
{
    luabind::functor<bool> allowed;
    return ai().script_engine().functor("pf_vehicle_service.can_charge", allowed) && allowed(section);
}

CSE_ALifeItem* canister(u16 id)
{
    auto* item = smart_cast<CSE_ALifeItem*>(server_object(id));
    return item && item->s_name == vehicle_service::canister_section ? item : nullptr;
}

void read_use_box(const char* profile, const char* center_key, const char* size_key, Fobb& box)
{
    if (!pSettings->line_exist(profile, center_key) || !pSettings->line_exist(profile, size_key))
        return;

    const Fvector center = pSettings->r_fvector3(profile, center_key);
    const Fvector half_size = pSettings->r_fvector3(profile, size_key);
    if (!_valid(center) || !_valid(half_size) || half_size.x <= 0.f || half_size.y <= 0.f || half_size.z <= 0.f)
    {
        Msg("! Invalid vehicle interaction bounds in [%s] (%s)", profile, size_key);
        return;
    }
    box.m_translate = center;
    box.m_halfsize = half_size;
}
}

void CCar::InitServiceProfile(IKinematics& model)
{
    // COP_ENGINE_PATCH: COP-ENG-266 - each profile supplies model-local use volumes.
    m_service_hood = BI_NONE;
    m_service_use_box.invalidate();
    m_trunk_use_box.invalidate();
    m_service_impact_damage = true;
    m_service_has_plugs = true;
    m_service_has_belt = true;
    m_service_hull_disabled = false;
    m_service_spark_plug_wear = 0.f;
    if (!IsGameTypeSingleCompatible() || !pSettings->line_exist(cNameSect(), vehicle_service::profile_line))
        return;

    const auto profile = pSettings->r_string(cNameSect(), vehicle_service::profile_line);
    if (!profile || !pSettings->section_exist(profile))
        return;

    read_use_box(profile, "trunk_center", "trunk_half_size", m_trunk_use_box);
    if (m_trunk_use_box.m_halfsize.x > 0.f && !m_dcp_white_niva_profile)
        m_trunk_use_distance = 3.f;

    if (pSettings->line_exist(profile, "hood_bone"))
    {
        const auto hood_name = pSettings->r_string(profile, "hood_bone");
        const u16 hood = hood_name ? model.LL_BoneID(hood_name) : BI_NONE;
        if (hood == BI_NONE || m_doors.find(hood) == m_doors.end())
        {
            Msg("! Invalid vehicle service hood in [%s]", profile);
            return;
        }
        m_service_hood = hood;
    }
    read_use_box(profile, "service_center", "service_half_size", m_service_use_box);
    m_service_impact_damage = READ_IF_EXISTS(pSettings, r_bool, profile, "module_impact_damage", true);
    m_service_has_plugs = READ_IF_EXISTS(pSettings, r_bool, profile, "has_spark_plugs", true);
    m_service_has_belt = READ_IF_EXISTS(pSettings, r_bool, profile, "has_timing_belt", true);
    if (m_service_has_plugs)
        m_service_spark_plug_wear = service_spark_plug_wear(profile);
}

CSE_ALifeCar* CCar::ServiceServerCar() const
{
    return UsesServiceProfile() ? smart_cast<CSE_ALifeCar*>(server_object(ID())) : nullptr;
}

bool CCar::ServiceElectricalPower() const
{
    if (ServiceDisabled())
        return false;
    const auto* car = ServiceServerCar();
    return !car || car->service_state.battery > 0.f;
}

bool CCar::CanRunServiceEngine() const
{
    if (ServiceDisabled())
        return false;
    const auto* car = ServiceServerCar();
    return !car || can_run_service_engine(car->service_state, m_service_has_belt);
}

bool CCar::CanStartServiceEngine() const
{
    if (ServiceDisabled())
        return false;
    const auto* car = ServiceServerCar();
    if (!car)
        return true;
    const auto& state = car->service_state;
    return state.battery > 0.f && (!m_service_has_plugs || state.plugs > 0.f) &&
        can_run_service_engine(state, m_service_has_belt);
}

void CCar::WearServiceSparkPlugs()
{
    if (m_service_spark_plug_wear <= 0.f)
        return;
    auto* car = ServiceServerCar();
    if (!car)
        return;
    auto& state = car->service_state;
    state.initialized = true;
    const float remaining = state.plugs - m_service_spark_plug_wear;
    // Snap away float roundoff after the final full-use start.
    state.plugs = remaining <= EPS ? 0.f : remaining;
}

Fvector CCar::ServiceCameraTarget() const
{
    Fvector target;
    XFORM().transform_tiny(target, m_service_use_box.m_translate);
    return target;
}

float CCar::ServiceUseDistance() const
{
    return m_service_hood == BI_NONE ? 2.5f : m_doors.at(m_service_hood).use_distance;
}

bool CCar::ServiceApproachable()
{
    // COP_ENGINE_PATCH: COP-ENG-267 - servicing is allowed while parked with the engine idling.
    if (!UsesServiceProfile() || !Actor() || !Actor()->g_Alive() ||
        Actor()->Holder() || Owner() || !m_pPhysicsShell || getDestroy())
        return false;
    const float reach = ServiceUseDistance() + .7f;
    if (Actor()->Position().distance_to_sqr(ServiceCameraTarget()) > reach * reach)
        return false;
    Fvector speed;
    m_pPhysicsShell->get_LinearVel(speed);
    return speed.square_magnitude() < parked_speed * parked_speed;
}

bool CCar::ServiceHoodReady()
{
    return UsesServiceProfile() && (m_service_hood == BI_NONE ||
        m_doors.at(m_service_hood).state == CCarDoor::opened) && ServiceApproachable();
}

CCar::UseArea CCar::PickUseArea(const Fvector& start, const Fvector& direction, float* distance)
{
    if (!IsGameTypeSingleCompatible() ||
        (!UsesServiceProfile() && !UsesProfileTrunkInteraction()) ||
        direction.square_magnitude() <= EPS_S)
        return UseArea::None;

    const auto& hit = HUD().GetCurrentRayQuery();
    const bool hit_car = hit.O == this;
    constexpr float surface_tolerance = .05f;
    const float visible_range = hit.range + surface_tolerance;
    UseArea area = UseArea::None;
    float nearest = flt_max;
    if (UsesServiceProfile())
    {
        const bool hood_occludes = hit_car && m_service_hood != BI_NONE && hit.element == m_service_hood;
        float reach = ServiceUseDistance();
        if (!hit_car || hood_occludes)
            reach = _min(reach, visible_range);
        Fobb box;
        box.transform(m_service_use_box, XFORM());
        if (box.intersect(start, direction, reach) && ServiceHoodReady())
        {
            area = UseArea::Service;
            nearest = reach;
        }
    }
    float trunk_range;
    if (TryProfileTrunk(start, direction, &trunk_range) &&
        trunk_range < nearest && (hit_car || trunk_range <= visible_range))
    {
        area = UseArea::Trunk;
        nearest = trunk_range;
    }
    if (distance && area != UseArea::None)
        *distance = nearest;
    return area;
}

// COP_ENGINE_PATCH: COP-ENG-268 - the vehicle profile determines which modules it has.
bool CCar::ServiceHasModule(u8 slot) const
{
    return UsesServiceProfile() && slot <= Belt &&
        (slot != Plugs || m_service_has_plugs) && (slot != Belt || m_service_has_belt);
}

float CCar::ServiceCondition(u8 slot) const
{
    if (slot == Hull)
        return vehicle_service::bounded(GetfHealth() / CDamagableItem::MaxHealth(), 1.f);
    if (!ServiceHasModule(slot))
        return 0.f;
    const auto* car = ServiceServerCar();
    if (!car)
        return 0.f;
    const auto& state = car->service_state;
    switch (slot)
    {
    case Coolant: return state.coolant;
    case Battery: return state.battery;
    case Plugs: return state.plugs;
    case Belt: return state.belt;
    default: return 0.f;
    }
}

void CCar::ApplyServiceHit(float hull_loss)
{
    if (!m_service_impact_damage || hull_loss + EPS_S < heavy_hit)
        return;
    auto* car = ServiceServerCar();
    if (!car)
        return;
    auto& state = car->service_state;
    float* eligible[3];
    u32 count = 0;
    float* const parts[] = {
        &state.coolant,
        ServiceHasModule(Plugs) ? &state.plugs : nullptr,
        ServiceHasModule(Belt) ? &state.belt : nullptr,
    };
    for (float* value : parts)
        if (value && *value > 0.f)
            eligible[count++] = value;
    if (!count)
        return;

    state.initialized = true;
    float& value = *eligible[::Random.randI(count)];
    value = _max(0.f, value - part_damage);
    const bool plugs_broken = &value == &state.plugs && state.plugs <= 0.f;
    if (b_engine_on && (plugs_broken || ServiceDisabled() || !can_run_service_engine(state, m_service_has_belt)))
        StopEngine();
}

void CCar::UpdateService()
{
    if (ServiceDisabled())
    {
        if (!m_service_hull_disabled)
        {
            StopEngine();
            ResetKeys();
            NeutralDrive();
            ReleaseBreaks();
            m_service_hull_disabled = true;
        }
        if (m_lights.IsOn())
            m_lights.TurnOffHeadLights();
        return;
    }

    m_service_hull_disabled = false;
    const bool lit = m_lights.IsOn();
    if (!lit && !b_engine_on)
        return;
    auto* car = ServiceServerCar();
    if (!car)
        return;

    auto& state = car->service_state;
    const float elapsed = Device.fTimeDelta;
    const float belt_factor = b_engine_on && m_service_has_belt
        ? belt_wear_factor(m_current_rpm, m_min_rpm, m_max_rpm) : 0.f;
    apply_service_time_wear(state, elapsed, lit, b_engine_on, belt_factor);
    if (lit && state.battery <= 0.f)
        m_lights.TurnOffHeadLights();

    if (!b_engine_on)
        return;
    if (state.coolant <= 0.f && m_pPhysicsShell)
    {
        Fvector speed;
        m_pPhysicsShell->get_LinearVel(speed);
        if (speed.x * speed.x + speed.z * speed.z >= parked_speed * parked_speed)
            state.dry_seconds = _min(vehicle_service::dry_drive_limit, state.dry_seconds + elapsed);
    }
    if (!can_run_service_engine(state, m_service_has_belt))
        StopEngine();
}

// COP_ENGINE_PATCH: COP-ENG-274 - settle elapsed vehicle wear after a completed move.
void CCar::ServiceApplyTravelWear(float game_minutes)
{
    if (game_minutes <= 0.f || !std::isfinite(game_minutes))
        return;
    const float time_factor = Level().GetGameTimeFactor();
    if (time_factor <= 0.f || !std::isfinite(time_factor))
        return;

    constexpr float default_multiplier = 1.10f;
    const float configured = READ_IF_EXISTS(pSettings, r_float, balance_section, "travel_wear_multiplier", default_multiplier);
    const float multiplier = std::isfinite(configured) && configured >= 0.f ? configured : default_multiplier;
    const float elapsed = game_minutes * (60.f / time_factor) * multiplier;
    if (elapsed <= 0.f || !std::isfinite(elapsed))
        return;

    if (auto* car = ServiceServerCar())
    {
        const float belt_factor = m_service_has_belt ? belt_wear_curve().working_factor : 0.f;
        apply_service_time_wear(car->service_state, elapsed, m_lights.IsOn(), true, belt_factor);
    }
}

bool CCar::ServiceApply(u8 slot, u16 item_id)
{
    if (!ServiceHasModule(slot) || !ServiceHoodReady() || ServiceCondition(slot) >= 1.f)
        return false;
    auto* car = ServiceServerCar();
    auto* item = carried_object(item_id);
    if (!car || !item)
        return false;

    const auto& section = item->cNameSect();
    const bool valid = (slot == Hull && section == "pf_hf_scrap") ||
        (slot == Coolant && (section == "flyaga" || section == "mineral_water")) ||
        (slot == Battery && charge_material(section.c_str())) ||
        (slot == Plugs && section == "pf_vehicle_spark_plugs") ||
        // COP_ENGINE_PATCH: COP-ENG-269 - either existing material can repair the timing belt.
        (slot == Belt && (section == "pf_hf_rubber" || section == "pf_hf_straps"));
    if (!valid)
        return false;

    if (slot == Coolant)
    {
        auto* water = item->cast_eatable_item();
        if (!water || !water->GetRemainingUses())
            return false;
        if (water->GetRemainingUses() > 1)
            water->SetRemainingUses(water->GetRemainingUses() - 1);
        else
            item->DestroyObject();
    }
    else
        item->DestroyObject();

    auto& state = car->service_state;
    state.initialized = true;
    switch (slot)
    {
    case Hull:
        ChangeCondition(service_repair_fraction("hull_repair_percent", 10.f) * CDamagableItem::MaxHealth());
        break;
    case Coolant:
        state.coolant = _min(1.f, state.coolant + service_repair_fraction("coolant_refill_percent", 50.f));
        state.dry_seconds = 0.f;
        break;
    case Battery: state.battery = 1.f; break;
    case Plugs: state.plugs = 1.f; break;
    case Belt:
        state.belt = _min(1.f, state.belt + service_repair_fraction("belt_repair_percent", 10.f));
        break;
    }
    return true;
}

float CCar::ServiceCanisterLitres(u16 item_id)
{
    const auto* item = canister(item_id);
    return item ? item->service_fuel_litres : -1.f;
}

float CScriptGameObject::GetFuelCanisterLitres() const
{
    return CCar::ServiceCanisterLitres(object().ID());
}

float CCar::ServiceTransferFuel(u16 item_id, float request, bool drain)
{
    // COP_ENGINE_PATCH: COP-ENG-271 - transfer the requested volume from a physical canister.
    if (!FuelSystemEnabled() || !std::isfinite(request) || request <= 0.f || m_fuel_tank <= 0.f)
        return 0.f;

    auto* carried = carried_object(item_id);
    auto* inventory_item = carried ? carried->cast_inventory_item() : nullptr;
    if (!inventory_item)
        return 0.f;

    if (drain ? !ServiceHoodReady() :
        (!g_Alive() || GetfHealth() <= 0.f ||
            Actor()->Position().distance_to(Position()) > FuelRefuelDistance() + 2.f))
        return 0.f;

    auto* item = canister(item_id);
    if (!item || item->ID_Parent != Actor()->ID())
        return 0.f;
    const float tank = vehicle_service::bounded(m_fuel, m_fuel_tank);
    const float litres = vehicle_service::bounded(item->service_fuel_litres, vehicle_service::canister_capacity);
    const float available = drain ? tank : litres;
    const float room = drain ? vehicle_service::canister_capacity - litres : m_fuel_tank - tank;
    const float moved = _min(request, _min(available, room));
    if (moved <= 0.f)
        return 0.f;

    const float tank_after = tank + (drain ? -moved : moved);
    const float canister_after = litres + (drain ? moved : -moved);
    if (tank_after == tank || canister_after == litres)
        return 0.f;
    m_fuel = tank_after;
    item->service_fuel_litres = canister_after;
    inventory_item->setWeight(canister_after);
    Actor()->inventory().CalcTotalWeight();
    return moved;
}
