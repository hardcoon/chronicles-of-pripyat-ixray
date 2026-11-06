#include "StdAfx.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "../xrEngine/GameMtlLib.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xrPhysics/IPHWorld.h"

namespace
{
struct SProfileCollisionSoundOwners
{
	xrCriticalSection lock;
	xr_map<IPhysicsShellHolder*, CCar*> cars;
};

SProfileCollisionSoundOwners& ProfileCollisionSoundOwners()
{
	static SProfileCollisionSoundOwners registry;
	return registry;
}

void CreateOptionalCarSound(CInifile* ini, LPCSTR key, ref_sound& sound)
{
	if (!ini->line_exist("car_sound", key))
		return;

	LPCSTR name = ini->r_string("car_sound", key);
	if (name && xr_strlen(name))
		sound.create(name, st_Effect, sg_SourceType);
}

void StartLoop(CCar* car, ref_sound& sound)
{
	if (!sound._handle() || sound._feedback())
		return;

	sound.play(car, sm_Looped);
	sound.set_volume(0.f);
}

void StopLoop(ref_sound& sound)
{
	if (sound._feedback())
		sound.stop();
}

float SmoothSoundValue(float current, float target, float speed)
{
	const float factor = clampr(Device.fTimeDelta * speed, 0.f, 1.f);
	return current + (target - current) * factor;
}
}

CCar::SCarSound::SCarSound(CCar* car)
{
	volume = 1.f;
	pcar = car;
	relative_pos.set(0.f, 0.5f, -1.f);
	relative_engine_pos.set(relative_pos);
	sound_rpm = 3250.f;
	engine_sound_scale_min = 0.5f;
	engine_sound_scale_max = 2.f;
	engine_sound_scale_factor = 1.f;
	whine_speed = 50.f;
	whine_reverse = true;
	previous_rpm = 0.f;
	idle_volume = 0.f;
	run_volume = 0.f;
	decel_volume = 0.f;
	whine_volume = 0.f;
	wheel_volume = 0.f;
	skid_volume = 0.f;
	collision_volume_scale = 1.f;
	collision_material = u16(-1);
}

CCar::SCarSound::~SCarSound()
{
}

void CCar::SCarSound::Init()
{
	CInifile* ini = smart_cast<IKinematics*>(pcar->Visual())->LL_UserData();
	if (ini->section_exist("car_sound") && ini->line_exist("car_sound", "snd_volume"))
	{
		volume = ini->r_float("car_sound", "snd_volume");

		snd_engine.create(ini->r_string("car_sound", "snd_name"), st_Effect, sg_SourceType);
		snd_engine_start.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_start", "car\\test_car_start"), st_Effect, sg_SourceType);
		snd_engine_stop.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_stop", "car\\test_car_stop"), st_Effect, sg_SourceType);
		float start_delay_factor = READ_IF_EXISTS(ini, r_float, "car_sound", "engine_sound_start_dellay", 0.25f);
		engine_start_delay = iFloor((snd_engine_start._handle() ? iFloor(snd_engine_start.get_length_sec() * 1000.0f) : 1.f) * start_delay_factor);
		if (ini->line_exist("car_sound", "relative_pos"))
			relative_pos.set(ini->r_fvector3("car_sound", "relative_pos"));
		// COP_ENGINE_PATCH: COP-ENG-198 - preserve the exact donor engine emitter.
		relative_engine_pos = pcar->m_dcp_white_niva_profile ?
			READ_IF_EXISTS(ini, r_fvector3, "car_sound", "relative_engine_pos", relative_pos) : relative_pos;
		if (ini->line_exist("car_sound", "transmission_switch"))
			snd_transmission.create(ini->r_string("car_sound", "transmission_switch"), st_Effect, sg_SourceType);

		if (pcar->m_dcp_white_niva_profile)
		{
			// COP_ENGINE_PATCH: COP-ENG-037
			// Original DCP keys are kept; the mix is opt-in through the vehicle profile.
			CreateOptionalCarSound(ini, "idle_sound", snd_idle);
			CreateOptionalCarSound(ini, "decel_sound", snd_decel);
			CreateOptionalCarSound(ini, "whine_sound", snd_whine);
			CreateOptionalCarSound(ini, "wheel_road", snd_wheel_road);
			CreateOptionalCarSound(ini, "wheel_dirt", snd_wheel_dirt);
			CreateOptionalCarSound(ini, "wheel_grass", snd_wheel_grass);
			CreateOptionalCarSound(ini, "wheel_water", snd_wheel_water);
			CreateOptionalCarSound(ini, "dirt_skid", snd_wheel_skid);

			sound_rpm = _max(READ_IF_EXISTS(ini, r_float, "car_definition", "sound_rpm", sound_rpm), 1.f);
			engine_sound_scale_max = _max(
				READ_IF_EXISTS(ini, r_float, "car_sound", "engine_sound_scale_max", engine_sound_scale_max), 0.01f);
			engine_sound_scale_min = clampr(
				READ_IF_EXISTS(ini, r_float, "car_sound", "engine_sound_scale_min", engine_sound_scale_min),
				0.01f, engine_sound_scale_max);
			engine_sound_scale_factor = _max(
				READ_IF_EXISTS(ini, r_float, "car_sound", "engine_sound_scale_factor", engine_sound_scale_factor), 0.01f);
			whine_speed = _max(READ_IF_EXISTS(ini, r_float, "car_sound", "whine_speed", whine_speed), 1.f);
			whine_reverse = !!READ_IF_EXISTS(ini, r_bool, "car_sound", "whine_reverse", true);

			IKinematics* kinematics = smart_cast<IKinematics*>(pcar->Visual());
			for (const auto& [bone_id, door] : pcar->m_doors)
			{
				LPCSTR bone_name = kinematics->LL_BoneName_dbg(bone_id);
				string128 open_key;
				string128 close_key;
				xr_sprintf(open_key, "%s_open", bone_name);
				xr_sprintf(close_key, "%s_close", bone_name);
				if (!ini->line_exist("car_sound", open_key) && !ini->line_exist("car_sound", close_key))
					continue;

				SDoorSound* door_sound = new SDoorSound();
				door_sound->bone_id = bone_id;
				CreateOptionalCarSound(ini, open_key, door_sound->open);
				CreateOptionalCarSound(ini, close_key, door_sound->close);
				door_sounds.push_back(door_sound);
			}
		}

		if (ini->line_exist("car_definition", "collision_sound_profile"))
		{
			LPCSTR profile = ini->r_string("car_definition", "collision_sound_profile");
			if (profile && pSettings->section_exist(profile))
			{
				// COP_ENGINE_PATCH: COP-ENG-071 - DCP used a dedicated car-cabin
				// material pair with two restrained body thumps and roughly half
				// the IX-Ray collision gain. Keep that behavior opt-in and data
				// driven so legacy physics materials retain their original sounds.
				collision_volume_scale = clampr(
					READ_IF_EXISTS(pSettings, r_float, profile, "volume_scale", 0.5f), 0.f, 1.f);
				LPCSTR material = READ_IF_EXISTS(
					pSettings, r_string, profile, "material", "objects\\car_cabine");
				collision_material = GMLib.GetMaterialIdx(material);

				for (u32 index = 1; index <= 8; ++index)
				{
					string32 key;
					xr_sprintf(key, "sound_%u", index);
					if (!pSettings->line_exist(profile, key))
						continue;

					ref_sound sound;
					sound.create(pSettings->r_string(profile, key), st_Effect, sg_SourceType);
					if (sound._handle())
						collision_sounds.push_back(sound);
				}
			}
		}
	}
	else
	{
		Msg("! Car doesn't contain sound params");
	}
	eCarSound = sndOff;
}

void CCar::SCarSound::SetSoundPosition(ref_sound& snd)
{
	VERIFY(!physics_world()->Processing());
	if (snd._feedback())
	{
		Fvector pos;
		// COP_ENGINE_PATCH: COP-ENG-198 - move engine/start/stop/idle/deceleration
		// together, keeping transmission, whine and wheel emitters at their own origin.
		const bool engine = &snd == &snd_engine || &snd == &snd_engine_start ||
			&snd == &snd_engine_stop || &snd == &snd_idle || &snd == &snd_decel;
		pcar->XFORM().transform_tiny(pos, engine ? relative_engine_pos : relative_pos);
		snd.set_position(pos);
	}
}

void CCar::SCarSound::StartDcpLoops()
{
	StartLoop(pcar, snd_engine);
	StartLoop(pcar, snd_idle);
	StartLoop(pcar, snd_decel);
	StartLoop(pcar, snd_whine);
}

void CCar::SCarSound::StopDcpLoops()
{
	StopLoop(snd_engine);
	StopLoop(snd_idle);
	StopLoop(snd_decel);
	StopLoop(snd_whine);
	idle_volume = 0.f;
	run_volume = 0.f;
	decel_volume = 0.f;
	whine_volume = 0.f;
}

void CCar::SCarSound::QueueDoorSound(u16 bone_id, bool opening)
{
	if (pcar->m_dcp_white_niva_profile)
		pending_door_sounds.emplace_back(bone_id, opening);
}

float CCar::SCarSound::InteriorVolumeFactor() const
{
	if (!pcar->m_dcp_white_niva_profile || !pcar->OwnerActor() || !pcar->active_camera ||
		pcar->active_camera->tag != ectFirst)
	{
		return 1.f;
	}

	return clampr(pcar->m_interior_volume, 0.f, 1.f);
}

void CCar::SCarSound::SetInteriorOneShotVolume(ref_sound& snd)
{
	if (snd._feedback())
		snd.set_volume(InteriorVolumeFactor());
}

bool CCar::SCarSound::PlayCollisionSound(const Fvector& position, float impact_volume, u16 material)
{
	if (collision_sounds.empty() || material != collision_material)
		return false;

	float adjusted_volume = _max(
		impact_volume * collision_volume_scale * InteriorVolumeFactor(), 0.f);
	Fvector sound_position = position;
	ref_sound& sound = collision_sounds[Random.randI((int)collision_sounds.size())];
	sound.play_no_feedback(pcar, 0, 0, &sound_position, &adjusted_volume);
	return true;
}

bool CCar::PlayProfileCollisionSound(const Fvector& position, float impact_volume, u16 material)
{
	return m_car_sound && m_car_sound->PlayCollisionSound(position, impact_volume, material);
}

void CCar::RegisterProfileCollisionSoundOwner()
{
	SProfileCollisionSoundOwners& registry = ProfileCollisionSoundOwners();
	xrCriticalSectionGuard guard(registry.lock);
	registry.cars[static_cast<IPhysicsShellHolder*>(this)] = this;
}

void CCar::UnregisterProfileCollisionSoundOwner()
{
	SProfileCollisionSoundOwners& registry = ProfileCollisionSoundOwners();
	xrCriticalSectionGuard guard(registry.lock);
	const auto owner = registry.cars.find(static_cast<IPhysicsShellHolder*>(this));
	if (owner != registry.cars.end() && owner->second == this)
		registry.cars.erase(owner);
}

bool CCar::PlayProfileCollisionSoundForHolder(
	IPhysicsShellHolder* holder, const Fvector& position, float impact_volume, u16 material)
{
	if (!holder)
		return false;

	SProfileCollisionSoundOwners& registry = ProfileCollisionSoundOwners();
	xrCriticalSectionGuard guard(registry.lock);
	const auto owner = registry.cars.find(holder);
	if (owner == registry.cars.end())
		return false;

	// COP_ENGINE_PATCH: COP-ENG-076 - the physics callback treats holder only as
	// an opaque address. Holding the registry lock through playback prevents a
	// matching car from entering net_Destroy while its sound state is in use.
	return owner->second->PlayProfileCollisionSound(position, impact_volume, material);
}

void CCar::SCarSound::SetDoorSoundPosition(ref_sound& snd, u16 bone_id)
{
	if (!snd._feedback())
		return;

	IKinematics* kinematics = smart_cast<IKinematics*>(pcar->Visual());
	const SBoneShape& shape = kinematics->LL_GetData(bone_id).shape;
	Fvector shape_center;
	shape_center.set(0.f, 0.f, 0.f);
	switch (shape.type)
	{
	case SBoneShape::stBox:
		shape_center.set(shape.box.m_translate);
		break;
	case SBoneShape::stSphere:
		shape_center.set(shape.sphere.P);
		break;
	case SBoneShape::stCylinder:
		shape_center.set(shape.cylinder.m_center);
		break;
	}

	Fvector position;
	auto door = pcar->m_doors.find(bone_id);
	if (door != pcar->m_doors.end() && door->second.Joint && door->second.Joint->bActive)
	{
		Fmatrix transform;
		door->second.Joint->PSecond_element()->InterpolateGlobalTransform(&transform);
		transform.transform_tiny(position, shape_center);
	}
	else
	{
		kinematics->CalculateBones();
		Fvector model_position;
		kinematics->LL_GetBoneInstance(bone_id).mTransform.transform_tiny(model_position, shape_center);
		pcar->XFORM().transform_tiny(position, model_position);
	}
	// COP_ENGINE_PATCH: COP-ENG-045 - door effects follow their own physical
	// bone. Reusing relative_pos placed every door sample at the engine under
	// the hood.
	// COP_ENGINE_PATCH: COP-ENG-046 - use the physical shape centre rather than
	// the hinge/pivot origin, so the one-shot is emitted by the door panel.
	snd.set_position(position);
}

void CCar::SCarSound::UpdateDcpProfile()
{
	VERIFY(!physics_world()->Processing());
	// COP_ENGINE_PATCH: COP-ENG-056 - interior_volume attenuates only sounds
	// emitted by the occupied car. The global effects mix stays untouched, so
	// quiet ambience, anomalies, NPCs and UI do not disappear in first person.
	const float interior_volume = InteriorVolumeFactor();
	SetInteriorOneShotVolume(snd_engine_start);
	SetInteriorOneShotVolume(snd_engine_stop);
	SetInteriorOneShotVolume(snd_transmission);
	for (SDoorSound* door_sound : door_sounds)
	{
		SetInteriorOneShotVolume(door_sound->open);
		SetInteriorOneShotVolume(door_sound->close);
	}

	for (const auto& pending : pending_door_sounds)
	{
		for (SDoorSound* door_sound : door_sounds)
		{
			if (door_sound->bone_id != pending.first)
				continue;
			ref_sound& sound = pending.second ? door_sound->open : door_sound->close;
			if (sound._handle())
			{
				sound.play(pcar);
				sound.set_volume(interior_volume);
				SetDoorSoundPosition(sound, door_sound->bone_id);
			}
			break;
		}
	}
	pending_door_sounds.clear();

	Fvector velocity;
	velocity.set(0.f, 0.f, 0.f);
	if (pcar->m_pPhysicsShell)
		pcar->m_pPhysicsShell->get_LinearVel(velocity);
	const float speed = velocity.magnitude();

	const bool engine_ready = pcar->b_engine_on &&
		(eCarSound != sndStarting || time_state_start + engine_start_delay < Device.dwTimeGlobal);
	if (engine_ready)
		StartDcpLoops();

	// COP_ENGINE_PATCH: COP-ENG-049 - use DCP's audible RPM transition during a
	// gear change without feeding that interpolation back into vehicle physics.
	const float rpm = pcar->DcpSoundEngineSpeed() * (60.f / (2.f * M_PI));
	const float rpm_scale = rpm / sound_rpm;
	const float rpm_factor = clampr(rpm_scale, 0.f, 1.6f);
	const float idle_rpm = pcar->m_min_rpm * (60.f / (2.f * M_PI));
	const float idle_factor = engine_ready ? 1.f - clampr((rpm - idle_rpm) / sound_rpm, 0.f, 1.f) : 0.f;
	const bool decelerating = engine_ready && speed > 1.f && (pcar->b_breaks || rpm + 15.f < previous_rpm);
	const float target_decel = decelerating ? clampr(speed / 12.f, 0.25f, 1.f) : 0.f;
	const float target_run = engine_ready ? clampr(0.18f + rpm_factor * 0.82f, 0.f, 1.f) * (1.f - target_decel * 0.55f) : 0.f;
	const float target_whine = engine_ready && speed > 2.f && (whine_reverse || pcar->CurrentTransmission() != 0) ?
		clampr(speed * 3.6f / whine_speed, 0.f, 1.f) : 0.f;

	idle_volume = SmoothSoundValue(idle_volume, idle_factor * volume * interior_volume, 5.f);
	run_volume = SmoothSoundValue(run_volume, target_run * volume * interior_volume, 6.f);
	decel_volume = SmoothSoundValue(decel_volume, target_decel * volume * interior_volume, 8.f);
	whine_volume = SmoothSoundValue(whine_volume, target_whine * volume * interior_volume * 0.55f, 4.f);
	previous_rpm = rpm;

	// COP_ENGINE_PATCH: COP-ENG-042 - DCP scales the engine loop directly from
	// physical RPM.  The former offset curve compressed the audible range and
	// made consecutive gears sound like one continuous drone.  Keeping the
	// original keys restores the clear pitch climb and the real RPM drop after
	// a shift without altering power, ratios or the gearbox simulation.
	const float engine_pitch = clampr(rpm_scale * engine_sound_scale_factor,
		engine_sound_scale_min, engine_sound_scale_max);
	const float idle_pitch = clampr(0.82f + rpm_factor * 0.2f, 0.75f, 1.15f);
	const float whine_pitch = clampr(0.72f + speed * 3.6f / whine_speed * 0.45f, 0.72f, 1.45f);
	snd_engine.set_frequency(engine_pitch);
	snd_engine.set_volume(run_volume);
	snd_idle.set_frequency(idle_pitch);
	snd_idle.set_volume(idle_volume);
	snd_decel.set_frequency(engine_pitch);
	snd_decel.set_volume(decel_volume);
	snd_whine.set_frequency(whine_pitch);
	snd_whine.set_volume(whine_volume);
	SetSoundPosition(snd_engine);
	SetSoundPosition(snd_idle);
	SetSoundPosition(snd_decel);
	SetSoundPosition(snd_whine);

	EDcpWheelSurface surface = dcpSurfaceRoad;
	u32 newest_contact = 0;
	for (const auto& [bone_id, wheel] : pcar->m_wheels_map)
	{
		if (wheel.collision_params.last_contact_time >= newest_contact)
		{
			newest_contact = wheel.collision_params.last_contact_time;
			surface = wheel.collision_params.surface;
		}
	}
	const bool has_contact = newest_contact + 250 >= Device.dwTimeGlobal;
	const float target_wheel = has_contact ? clampr((speed - 0.4f) / 12.f, 0.f, 0.9f) : 0.f;
	wheel_volume = SmoothSoundValue(wheel_volume, target_wheel * volume * interior_volume, 7.f);
	const float steer_factor = clampr(_abs(pcar->m_steer_angle), 0.f, 1.f);
	const float target_skid = has_contact && surface != dcpSurfaceWater && speed > 4.f &&
		(pcar->b_breaks || steer_factor > 0.45f) ? clampr(speed / 18.f, 0.f, 1.f) : 0.f;
	skid_volume = SmoothSoundValue(skid_volume, target_skid * volume * interior_volume, 9.f);

	if (speed > 0.2f)
	{
		StartLoop(pcar, snd_wheel_road);
		StartLoop(pcar, snd_wheel_dirt);
		StartLoop(pcar, snd_wheel_grass);
		StartLoop(pcar, snd_wheel_water);
		StartLoop(pcar, snd_wheel_skid);
	}

	ref_sound* wheel_sounds[] = {&snd_wheel_road, &snd_wheel_dirt, &snd_wheel_grass, &snd_wheel_water};
	for (u32 index = 0; index < std::size(wheel_sounds); ++index)
	{
		ref_sound& sound = *wheel_sounds[index];
		sound.set_volume(index == u32(surface) ? wheel_volume : 0.f);
		sound.set_frequency(clampr(0.8f + speed / 35.f, 0.8f, 1.35f));
		SetSoundPosition(sound);
	}
	snd_wheel_skid.set_volume(skid_volume);
	snd_wheel_skid.set_frequency(clampr(0.9f + speed / 45.f, 0.9f, 1.3f));
	SetSoundPosition(snd_wheel_skid);
}

void CCar::SCarSound::UpdateStarting()
{
	VERIFY(!physics_world()->Processing());
	SetSoundPosition(snd_engine_start);
	if (pcar->m_dcp_white_niva_profile)
	{
		if (time_state_start + engine_start_delay < Device.dwTimeGlobal)
			StartDcpLoops();
	}
	else if (snd_engine._feedback())
	{
		UpdateDrive();
	}
	else if (time_state_start + engine_start_delay < Device.dwTimeGlobal)
	{
		snd_engine.play(pcar, sm_Looped);
		UpdateDrive();
	}

	if (!snd_engine_start._feedback())
		Drive();
}

void CCar::SCarSound::UpdateStoping()
{
	VERIFY(!physics_world()->Processing());
	SetSoundPosition(snd_engine_stop);
	if (!snd_engine_stop._feedback())
		SwitchOff();
}

void CCar::SCarSound::UpdateStalling()
{
	SetSoundPosition(snd_engine_stop);
	if (!snd_engine_stop._feedback())
		SwitchOff();
}

void CCar::SCarSound::UpdateDrive()
{
	VERIFY(!physics_world()->Processing());
	if (pcar->m_dcp_white_niva_profile)
		return;
	float scale = 0.5f + 0.5f * pcar->m_current_rpm / pcar->m_torque_rpm;
	clamp(scale, 0.5f, 1.25f);
	snd_engine.set_frequency(scale);
	SetSoundPosition(snd_engine);
}

void CCar::SCarSound::SwitchState(ESoundState new_state)
{
	eCarSound = new_state;
	time_state_start = Device.dwTimeGlobal;
}

void CCar::SCarSound::Update()
{
	VERIFY(!physics_world()->Processing());
	if (pcar->m_dcp_white_niva_profile)
	{
		switch (eCarSound)
		{
		case sndOff: break;
		case sndStarting: UpdateStarting(); break;
		case sndDrive: UpdateDrive(); break;
		case sndStalling: UpdateStalling(); break;
		case sndStoping: UpdateStoping(); break;
		}
		UpdateDcpProfile();
		return;
	}

	if (eCarSound == sndOff)
		return;
	switch (eCarSound)
	{
	case sndStarting: UpdateStarting(); break;
	case sndDrive: UpdateDrive(); break;
	case sndStalling: UpdateStalling(); break;
	case sndStoping: UpdateStoping(); break;
	}
}

void CCar::SCarSound::SwitchOn()
{
	pcar->processing_activate();
}

void CCar::SCarSound::Destroy()
{
	SwitchOff();
	StopLoop(snd_wheel_road);
	StopLoop(snd_wheel_dirt);
	StopLoop(snd_wheel_grass);
	StopLoop(snd_wheel_water);
	StopLoop(snd_wheel_skid);
	snd_engine.destroy();
	snd_transmission.destroy();
	snd_engine_stop.destroy();
	snd_engine_start.destroy();
	snd_idle.destroy();
	snd_decel.destroy();
	snd_whine.destroy();
	snd_wheel_road.destroy();
	snd_wheel_dirt.destroy();
	snd_wheel_grass.destroy();
	snd_wheel_water.destroy();
	snd_wheel_skid.destroy();
	for (ref_sound& sound : collision_sounds)
		sound.destroy();
	collision_sounds.clear();
	for (SDoorSound* door_sound : door_sounds)
	{
		door_sound->open.destroy();
		door_sound->close.destroy();
		xr_delete(door_sound);
	}
	door_sounds.clear();
	pending_door_sounds.clear();
}

void CCar::SCarSound::SwitchOff()
{
	if (pcar->m_dcp_white_niva_profile)
		StopDcpLoops();
	eCarSound = sndOff;
	pcar->processing_deactivate();
}

void CCar::SCarSound::Start()
{
	VERIFY(!physics_world()->Processing());
	if (eCarSound == sndOff)
		SwitchOn();
	SwitchState(sndStarting);
	previous_rpm = 0.f;
	snd_engine_start.play(pcar);
	SetInteriorOneShotVolume(snd_engine_start);
	SetSoundPosition(snd_engine_start);
}

void CCar::SCarSound::Stall()
{
	VERIFY(!physics_world()->Processing());
	if (eCarSound == sndOff)
		return;
	SwitchState(sndStalling);
	if (!pcar->m_dcp_white_niva_profile)
		snd_engine.stop_deffered();
	snd_engine_stop.play(pcar);
	SetInteriorOneShotVolume(snd_engine_stop);
	SetSoundPosition(snd_engine_stop);
}

void CCar::SCarSound::Stop()
{
	VERIFY(!physics_world()->Processing());
	if (eCarSound == sndOff)
		return;
	SwitchState(sndStoping);
	if (!pcar->m_dcp_white_niva_profile)
		snd_engine.stop_deffered();
	snd_engine_stop.play(pcar);
	SetInteriorOneShotVolume(snd_engine_stop);
	SetSoundPosition(snd_engine_stop);
}

void CCar::SCarSound::Drive()
{
	VERIFY(!physics_world()->Processing());
	if (eCarSound == sndOff)
		SwitchOn();
	SwitchState(sndDrive);
	if (pcar->m_dcp_white_niva_profile)
		StartDcpLoops();
	else if (!snd_engine._feedback())
		snd_engine.play(pcar, sm_Looped);
	SetSoundPosition(snd_engine);
}

void CCar::SCarSound::TransmissionSwitch()
{
	VERIFY(!physics_world()->Processing());
	if (snd_transmission._handle() && eCarSound != sndOff)
	{
		snd_transmission.play(pcar);
		SetInteriorOneShotVolume(snd_transmission);
		SetSoundPosition(snd_transmission);
	}
}
