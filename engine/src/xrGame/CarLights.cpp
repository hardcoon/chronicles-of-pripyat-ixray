#include "StdAfx.h"
#include "CarLights.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xrPhysics/IPHWorld.h"

SCarLight::SCarLight()
{
	light_render = nullptr;
	glow_render = nullptr;
	bone_id = BI_NONE;
	m_holder = nullptr;
	local_position.set(0.f, 0.f, 0.f);
	local_direction.set(0.f, 0.f, 0.f);
	has_glow = false;
}

SCarLight::~SCarLight()
{
	light_render.destroy();
	glow_render.destroy();
	bone_id = BI_NONE;
}

void SCarLight::Init(CCarLights* holder)
{
	m_holder = holder;
}

void SCarLight::ParseDefinitions(LPCSTR section)
{
	IKinematics* kinematics = smart_cast<IKinematics*>(m_holder->PCar()->Visual());
	CInifile* ini = kinematics->LL_UserData();

	light_render = ::Render->light_create();
	const u32 light_type = READ_IF_EXISTS(ini, r_u32, section, "type", u32(IRender_Light::SPOT));
	light_render->set_type(IRender_Light::LT(clampr(light_type, u32(IRender_Light::POINT), u32(IRender_Light::OMNIPART))));
	// COP_ENGINE_PATCH: COP-ENG-208 - tiny marker lamps can opt out of shadow maps.
	light_render->set_shadow(READ_IF_EXISTS(ini, r_bool, section, "shadow", true));

	Fcolor color;
	color.set(ini->r_fcolor(section, "color"));
	light_render->set_range(ini->r_float(section, "range"));
	light_render->set_color(color);
	if (ini->line_exist(section, "cone_angle"))
		light_render->set_cone(deg2rad(ini->r_float(section, "cone_angle")));
	if (ini->line_exist(section, "spot_texture"))
		light_render->set_texture(ini->r_string(section, "spot_texture"));

	if (ini->line_exist(section, "volumetric"))
	{
		light_render->set_volumetric(!!ini->r_bool(section, "volumetric"));
		light_render->set_volumetric_distance(READ_IF_EXISTS(ini, r_float, section, "volumetric_distance", 1.f));
		light_render->set_volumetric_intensity(READ_IF_EXISTS(ini, r_float, section, "volumetric_intensity", 0.f));
		light_render->set_volumetric_quality(READ_IF_EXISTS(ini, r_float, section, "volumetric_quality", 1.f));
	}

	has_glow = ini->line_exist(section, "glow_texture") && ini->line_exist(section, "glow_radius");
	if (has_glow)
	{
		glow_render = ::Render->glow_create();
		glow_render->set_texture(ini->r_string(section, "glow_texture"));
		glow_render->set_color(color);
		glow_render->set_radius(ini->r_float(section, "glow_radius"));
		glow_render->set_active(false);
	}

	local_position = READ_IF_EXISTS(ini, r_fvector3, section, "position", Fvector().set(0.f, 0.f, 0.f));
	local_direction = READ_IF_EXISTS(ini, r_fvector3, section, "direction", Fvector().set(0.f, 0.f, 0.f));
	bone_id = kinematics->LL_BoneID(ini->r_string(section, "bone"));
	light_render->set_active(false);
	kinematics->LL_SetBoneVisible(bone_id, FALSE, TRUE);
}

void SCarLight::Switch()
{
	VERIFY(!physics_world()->Processing());
	if (isOn())
		TurnOff();
	else
		TurnOn();
}

void SCarLight::TurnOn()
{
	VERIFY(!physics_world()->Processing());
	if (isOn())
		return;
	// COP_ENGINE_PATCH: COP-ENG-199 - visibility belongs to the aggregate lens.
	light_render->set_active(true);
	m_holder->SyncBoneVisibility(bone_id);
	if (has_glow)
		glow_render->set_active(true);
	light_render->set_active(true);
	Update();
}

void SCarLight::TurnOff()
{
	VERIFY(!physics_world()->Processing());
	if (!isOn())
		return;
	if (has_glow)
		glow_render->set_active(false);
	light_render->set_active(false);
	m_holder->SyncBoneVisibility(bone_id);
}

bool SCarLight::isOn()
{
	VERIFY(!physics_world()->Processing());
	if (has_glow)
		VERIFY(Device.IsEditorMode() || light_render->get_active() == glow_render->get_active());
	return light_render->get_active();
}

void SCarLight::Update()
{
	VERIFY(!physics_world()->Processing());
	if (!isOn())
		return;

	CCar* car = m_holder->PCar();
	CBoneInstance& bone = smart_cast<IKinematics*>(car->Visual())->LL_GetBoneInstance(bone_id);
	Fmatrix transform;
	transform.mul(car->XFORM(), bone.mTransform);
	Fvector position;
	transform.transform_tiny(position, local_position);
	Fvector direction = transform.k;
	if (local_direction.square_magnitude() > EPS_S)
	{
		transform.transform_dir(direction, local_direction);
		direction.normalize_safe();
	}
	light_render->set_rotation(direction, transform.i);
	light_render->set_position(position);
	if (has_glow)
	{
		glow_render->set_direction(direction);
		glow_render->set_position(position);
	}
}

CCarLights::CCarLights()
{
	m_pcar = nullptr;
}

void CCarLights::Init(CCar* pcar)
{
	m_pcar = pcar;
	for (SCarLight* light : m_lights)
		xr_delete(light);
	m_lights.clear();
	m_head_lights.clear();
	m_stop_lights.clear();
	m_reverse_lights.clear();
	m_indoor_lights.clear();
	m_indoor_light_doors.clear();
}

void CCarLights::ParseGroup(CInifile* ini, LPCSTR key, LIGHTS_STORAGE& group)
{
	if (!ini->line_exist("lights", key))
		return;

	LPCSTR definitions = ini->r_string("lights", key);
	string64 section;
	const int count = _GetItemCount(definitions);
	for (int index = 0; index < count; ++index)
	{
		_GetItem(definitions, index, section);
		if (!ini->section_exist(section))
			continue;
		// COP_ENGINE_PATCH: COP-ENG-199 - reject absent donor bones before Update.
		IKinematics* kinematics = smart_cast<IKinematics*>(m_pcar->Visual());
		if (!ini->line_exist(section, "bone") ||
			kinematics->LL_BoneID(ini->r_string(section, "bone")) == BI_NONE)
		{
			Msg("! Vehicle [%s] light [%s] has no valid bone", m_pcar->cNameSect().c_str(), section);
			continue;
		}
		SCarLight* light = new SCarLight();
		light->Init(this);
		light->ParseDefinitions(section);
		m_lights.push_back(light);
		group.push_back(light);
	}
}

void CCarLights::ParseDefinitions()
{
	CInifile* ini = smart_cast<IKinematics*>(m_pcar->Visual())->LL_UserData();
	if (!ini->section_exist("lights"))
		return;

	ParseGroup(ini, "headlights", m_head_lights);
	if (!m_pcar->m_dcp_white_niva_profile)
		return;

	ParseGroup(ini, "stoplights", m_stop_lights);
	ParseGroup(ini, "reverselights", m_reverse_lights);
	ParseGroup(ini, "indoorlights", m_indoor_lights);
	if (ini->line_exist("lights", "indoorlightdoors"))
	{
		IKinematics* kinematics = smart_cast<IKinematics*>(m_pcar->Visual());
		LPCSTR definitions = ini->r_string("lights", "indoorlightdoors");
		string64 bone_name;
		const int count = _GetItemCount(definitions);
		for (int index = 0; index < count; ++index)
		{
			_GetItem(definitions, index, bone_name);
			const u16 bone_id = kinematics->LL_BoneID(bone_name);
			if (bone_id != BI_NONE)
				m_indoor_light_doors.push_back(bone_id);
		}
	}
}

void CCarLights::SetGroupState(LIGHTS_STORAGE& group, bool enabled)
{
	for (SCarLight* light : group)
	{
		if (enabled)
			light->TurnOn();
		else
			light->TurnOff();
	}
}

void CCarLights::UpdateAutomaticProfileLights()
{
	if (!m_pcar->m_dcp_white_niva_profile)
		return;

	SetGroupState(m_stop_lights, m_pcar->b_breaks);
	SetGroupState(m_reverse_lights, m_pcar->b_engine_on && m_pcar->CurrentTransmission() == 0);
	bool door_open = false;
	for (u16 bone_id : m_indoor_light_doors)
	{
		auto door = m_pcar->m_doors.find(bone_id);
		if (door != m_pcar->m_doors.end() && door->second.state != CCarDoor::closed)
		{
			door_open = true;
			break;
		}
	}
	SetGroupState(m_indoor_lights, door_open);
}

void CCarLights::Update()
{
	VERIFY(!physics_world()->Processing());
	UpdateAutomaticProfileLights();
	// COP_ENGINE_PATCH: COP-ENG-199 - an offscreen/cull-hidden car still emits
	// light from its actual skeleton, never stale identity bone transforms.
	if (std::any_of(m_lights.begin(), m_lights.end(), [](SCarLight* light) { return light->isOn(); }))
		smart_cast<IKinematics*>(m_pcar->Visual())->CalculateBones(TRUE);
	for (SCarLight* light : m_lights)
		light->Update();
}

void CCarLights::SyncBoneVisibility(u16 bone_id)
{
	// COP_ENGINE_PATCH: COP-ENG-199 - releasing the brake must not hide an
	// already-lit running lamp using the same original lens geometry.
	if (bone_id == BI_NONE)
		return;
	const bool visible = std::any_of(m_lights.begin(), m_lights.end(), [bone_id](SCarLight* light)
	{
		return light->bone_id == bone_id && light->isOn();
	});
	IKinematics* kinematics = smart_cast<IKinematics*>(m_pcar->Visual());
	if (kinematics->LL_GetBoneVisible(bone_id) == visible)
		return;
	kinematics->LL_SetBoneVisible(bone_id, visible, TRUE);
	kinematics->CalculateBones_Invalidate();
	kinematics->CalculateBones(TRUE);
}

void CCarLights::SwitchHeadLights()
{
	VERIFY(!physics_world()->Processing());
	if (m_head_lights.empty())
		return;
	SetGroupState(m_head_lights, !m_head_lights.front()->isOn());
}

void CCarLights::TurnOnHeadLights()
{
	VERIFY(!physics_world()->Processing());
	SetGroupState(m_head_lights, true);
}

void CCarLights::TurnOffHeadLights()
{
	VERIFY(!physics_world()->Processing());
	SetGroupState(m_head_lights, false);
}

bool CCarLights::IsLight(u16 bone_id)
{
	SCarLight* light = nullptr;
	return findLight(bone_id, light);
}

bool CCarLights::findLight(u16 bone_id, SCarLight*& light)
{
	SCarLight find_light;
	find_light.bone_id = bone_id;
	auto iterator = std::find_if(m_lights.begin(), m_lights.end(), SFindLightPredicate(&find_light));
	if (iterator == m_lights.end())
	{
		light = nullptr;
		return false;
	}
	light = *iterator;
	return true;
}

CCarLights::~CCarLights()
{
	for (SCarLight* light : m_lights)
		xr_delete(light);
	m_lights.clear();
	m_head_lights.clear();
	m_stop_lights.clear();
	m_reverse_lights.clear();
	m_indoor_lights.clear();
	m_indoor_light_doors.clear();
}

bool CCarLights::IsOn() const
{
	return !m_head_lights.empty() && m_head_lights.front()->isOn();
}
