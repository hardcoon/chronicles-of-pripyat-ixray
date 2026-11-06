#include "StdAfx.h"
#include "Car.h"

#ifdef DEBUG
#	include "../xrEngine/StatGraph.h"
#	include "PHDebug.h"
#endif // DEBUG
#include "Inventory.h"
//#include "Hit.h"
#include "PHDestroyable.h"

#include "cameralook.h"
#include "CameraFirstEye.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "math.h"
#include "script_entity_action.h"
#include "Inventory.h"
#include "xrServer_Objects_ALife_Items.h"
#include "../Include/xrRender/Kinematics.h"
#include "Level.h"
#include "ui/UIMainIngameWnd.h"
#include "CarWeapon.h"
#include "game_object_space.h"
#include "../xrEngine/GameMtlLib.h"
#include "../xrSound/Sound.h"

#include "CharacterPhysicsSupport.h"
#include "car_memory.h"
#include "../xrPhysics/IPHWorld.h"
#include "../xrPhysics/IActivationShape.h"
#include "../xrPhysics/PhysicsExternalCommon.h"
#include "../xrPhysics/phvalide.h"
#include "../xrPhysics/ExtendedGeom.h"
#include "CharacterPhysicsSupport.h"
#include "UIGameSP.h"

CCar::CCar()
{
	m_bone_trunk = BI_NONE;
	m_bone_steer = BI_NONE;
	m_dcp_white_niva_profile = false;
	m_trunk_open_inventory = false;
	m_trunk_close_on_inventory_exit = false;
	m_trunk_max_weight = 0.f;
	m_trunk_use_distance = 1.f;
	m_trunk_interaction_padding.set(0.f, 0.f, 0.f);
	m_trunk_interaction_offset.set(0.f, 0.f, 0.f);
	m_fuel_system_enabled = false;
	m_part_damage_body_only = false;
	m_fuel_consumption_scale = 1.f;
	m_fuel_canister_amount = 20.f;
	m_fuel_refuel_min_free = 10.f;
	m_fuel_refuel_distance = 3.5f;
	m_durability_factor = 1.f;
	m_condition_warning_health = 0.5f;
	m_condition_critical_health = 0.1f;
	m_condition_smoke_health = -1.f;
	m_condition_damage_particles_active = false;
	m_dcp_body_roll_damping_enabled = false;
	m_dcp_body_roll_damping = 0.f;
	m_dcp_body_roll_min_grounded_wheels = 2;
	m_dcp_body_roll_contact_grace_ms = 120;
	m_dcp_starter_drive_delay = 1.f;
	m_dcp_engine_start_time = 0;
	m_dcp_drive_pending = false;
	m_interior_volume = 1.f;
	m_collision_hit_threshold = 5.f;
	m_hit_stall_threshold = flt_max;
	m_transmission_switch_delay = 0.f;
	m_transmission_settle_delay = 0.f;
	m_transmission_clutch_factor = 1.f;
	m_transmission_previous_rpm = 0.f;
	m_transmission_switch_time = 0;
	m_rpm_brp_increment_factor = 1.f;
	m_rpm_brp_decrement_factor = 1.f;
	m_steer_nonlinear = false;
	m_steer_invert = false;
	m_steering_limit_factor = 1.f;
	m_steering_start_time = 0.f;
	m_dcp_current_slip = 0.f;
	m_slip_sliding_threshold = 0.6f;
	m_vel_sliding_threshold = 5.f;
	m_wheels_spin_factor_start = 0.75f;
	m_wheels_spin_factor_stop = 0.8f;
	m_friction_slip_factor = 1.f;
	m_friction2_slip_factor = 1.f;
	m_friction_spin_factor = 1.f;
	m_friction2_spin_factor = 1.f;
	m_friction_slip_speed = 0.005f;
	m_friction2_steering_wheels = 1.f;
	m_friction2_nonsteering_wheels = 1.f;
	m_friction2_brp_boost = 1.f;
	m_contact_slip_scale = 0.f;
	m_handbrake_rpm_threshold = 2.f;
	m_handbrake_rpm_decrease = 1.f;
	m_drift_start_boost = 0.f;
	m_drift_boost_time = 0.25f;
	m_steer_spin_boost = 0.f;
	m_steer_speed_boost = 0.f;
	m_steer_turn_factor = 1.f;
	m_steer_counter_turn_factor = 0.5f;
	m_steer_turn_compensation = 0.5f;
	m_steer_turn_compensation_max = 0.3f;
	m_steer_turn_delta = 0.2f;
	m_brake_boost = 1.f;
	m_dcp_dashboard = false;
	// COP_ENGINE_PATCH: COP-ENG-151 - preserve existing gauges unless their model opts in.
	m_dcp_speedometer_body_profile = false;
	m_dcp_speedometer_min = 0.f;
	m_dcp_speedometer_max = 180.f;
	m_dcp_speedometer_factor = 1.f;
	m_dcp_speedometer_offset = 0.f;
	m_dcp_speedometer_direction = -1.f;
	m_dcp_tachometer_factor = 1.f;
	m_dcp_tachometer_min_rpm = 0.f;
	m_dcp_tachometer_max_rpm = 0.f;
	m_dcp_tachometer_offset = 0.f;
	m_dcp_tachometer_direction = -1.f;
	m_dcp_fuel_angle = 0.f;
	m_dcp_fuel_offset = 0.f;
	m_dcp_fan_bone = BI_NONE;
	m_dcp_fan_speed = 0.f;
	m_dcp_fan_current_speed = 0.f;
	m_dcp_fan_angle = 0.f;
	m_dcp_fan_engine = false;
	m_dcp_fan_update_frame = u32(-1);
	m_camera_inertion_n = 0.f;
	m_camera_inertion_k = 0.f;
	m_camera_angle_factor = 1.f;
	m_vehicle_camera_dynamics = false;
	m_camera_motion_strength = 0.35f;
	m_camera_horizon_limit = deg2rad(15.f);
	m_camera_motion_response = 0.14f;
	m_camera_motion_return = 0.24f;
	m_camera_acceleration_response = 0.08f;
	m_camera_max_lateral_acceleration = 12.f;
	m_camera_max_longitudinal_acceleration = 10.f;
	m_camera_lateral_offset_per_accel = 0.004f;
	m_camera_longitudinal_offset_per_accel = 0.0045f;
	m_camera_max_lateral_offset = 0.05f;
	m_camera_max_longitudinal_offset = 0.045f;
	m_camera_pitch_per_accel = deg2rad(0.18f);
	m_camera_max_pitch = deg2rad(1.8f);
	m_camera_previous_local_velocity.set(0.f, 0.f, 0.f);
	m_camera_filtered_local_acceleration.set(0.f, 0.f, 0.f);
	m_camera_local_offset.set(0.f, 0.f, 0.f);
	m_camera_local_angles.set(0.f, 0.f, 0.f);
	m_camera_dynamics_valid = false;
	m_door_air_resistance_scale = 0.001f;

	m_rpm_b = BI_NONE;
	m_speed_b = BI_NONE;
	m_fuel_b = BI_NONE;
	m_rpm_offsets.set(0, 0, 0);
	m_speed_offsets.set(0, 0, 0);

	m_memory = nullptr;
	m_driver_anim_type = 0;
	active_camera = 0;
	camera[ectFirst] = new CCameraFirstEye(this, CCameraBase::flRelativeLink | CCameraBase::flPositionRigid);
	camera[ectFirst]->tag = ectFirst;
	camera[ectFirst]->Load("car_firsteye_cam");
	// COP_ENGINE_PATCH: COP-ENG-203 - keep the driver's view/angles intact.
	m_weapon_camera = new CCameraFirstEye(this, CCameraBase::flRelativeLink | CCameraBase::flPositionRigid);
	m_weapon_camera->tag = ectFirst;
	m_weapon_camera->Load("car_firsteye_cam");
	// COP_ENGINE_PATCH: COP-ENG-205 - separate operator orbit, preserving every driver camera.
	m_weapon_external_camera = new CCameraLook(this, CCameraBase::flRelativeLink);
	m_weapon_external_camera->tag = ectChase;
	m_weapon_external_camera->Load("car_look_cam");

	camera[ectChase] = new CCameraLook(this, CCameraBase::flRelativeLink);
	camera[ectChase]->tag = ectChase;
	camera[ectChase]->Load("car_look_cam");

	camera[ectFree] = new CCameraLook(this);
	camera[ectFree]->tag = ectFree;
	camera[ectFree]->Load("car_free_cam");
	OnCameraChange(ectFirst);

	///////////////////////////////
	b_wheels_limited = false;
	b_engine_on = false;
	e_state_steer = idle;
	e_state_drive = neutral;
	m_current_gear_ratio = phInfinity;
	rsp = false; lsp = false; fwp = false; bkp = false; brp = false;
	///////////////////////////////

	m_exhaust_particles = "vehiclefx\\exhaust_1";
	m_car_sound = new SCarSound(this);

	//у машины слотов в инвентаре нет
	m_doors_torque_factor = 2.f;
	m_power_increment_factor = 0.5f;
	m_rpm_increment_factor = 0.5f;
	m_power_decrement_factor = 0.5f;
	m_rpm_decrement_factor = 0.5f;
	b_breaks = false;
	m_break_start = 0.f;
	m_break_time = 1.;
	m_breaks_to_back_rate = 1.f;

	b_exploded = false;
	m_car_weapon = nullptr;
	m_power_neutral_factor = 0.25f;
	m_steer_angle = 0.f;
#ifdef DEBUG
	InitDebug();
#endif
}

CCar::~CCar(void)
{
	ClearDcpGaugeVisuals();
    ClearGrassSpheres();
	UnregisterProfileCollisionSoundOwner();
	ClearDcpSuspensionVisuals();
	xr_delete(camera[0]);
	xr_delete(camera[1]);
	xr_delete(camera[2]);
	xr_delete(m_weapon_camera);
	xr_delete(m_weapon_external_camera);
	xr_delete(m_car_sound);
	ClearExhausts();
	xr_delete(m_car_weapon);
	xr_delete(m_memory);
}

void CCar::reinit()
{
	CEntity::reinit();
	CScriptEntity::reinit();
	if (m_memory)
		m_memory->reinit();
}

void CCar::reload(LPCSTR section)
{
	CEntity::reload(section);
	if (m_memory)
		m_memory->reload(section);
}

bool CCar::IsMyCar() const
{
	if (!Actor())
		return false;

	return Actor()->Holder() == this;
}

void CCar::cb_Steer(CBoneInstance* B)
{
	VERIFY2(fsimilar(DET(B->mTransform), 1.f, DET_CHECK_EPS), "Bones receive returns 0 matrix");
	CCar* C = static_cast<CCar*>(B->callback_param());
	Fmatrix m;


	m.rotateZ(C->m_steer_angle);

	B->mTransform.mulB_43(m);
#ifdef DEBUG
	if (!fsimilar(DET(B->mTransform), 1.f, DET_CHECK_EPS)) {

		Msg("RotatingZ angle=%f", C->m_steer_angle);
		VERIFY2(0, "Bones callback returns BAD!!! matrix");
	}
#endif
}

// Core events
void	CCar::Load(LPCSTR section)
{
	inherited::Load(section);
	SpatialComponent->spatial.type |= STYPE_VISIBLEFORAI;

	CInventoryOwner::Load(section);
	inventory().m_pOwner = this;

	// COP_ENGINE_PATCH: COP-ENG-084 - vehicle trunks may opt into a dedicated
	// weight limit. A missing key or zero keeps the legacy unlimited behavior;
	// ordinary inventory boxes never pass through this car-only setting.
	m_trunk_max_weight = _max(0.f, READ_IF_EXISTS(pSettings, r_float, section, "trunk_max_weight", 0.f));
}

bool CCar::CanStoreInTrunk(float additional_weight) const
{
	if (!HasTrunkWeightLimit())
		return true;

	return inventory().TotalWeight() + _max(0.f, additional_weight) <= m_trunk_max_weight + EPS;
}

BOOL CCar::net_Spawn(CSE_Abstract* DC)
{
#ifdef DEBUG
	InitDebug();
#endif

	CSE_Abstract* e = (CSE_Abstract*)(DC);
	CSE_ALifeCar* co = smart_cast<CSE_ALifeCar*>(e);
	// COP_ENGINE_PATCH: COP-ENG-059 - CPHSkeleton::Spawn consumes and clears
	// flSavedData while restoring the physical shell. Capture the versioned DCP
	// runtime payload before that happens so fuel and the running-engine bit can
	// be applied after ParseDefinitions/Init have created all car subsystems.
	const bool restore_dcp_runtime = co->_flags.test(CSE_PHSkeleton::flSavedData) &&
		(co->vehicle_runtime_version == 1 || co->vehicle_runtime_version == SVehiclePoseSnapshot::runtime_version);
	const float saved_vehicle_fuel = co->vehicle_fuel;
	const u8 saved_vehicle_runtime_flags = co->vehicle_runtime_flags;
	BOOL							R = inherited::net_Spawn(DC);

	PKinematics(Visual())->CalculateBones_Invalidate();
	PKinematics(Visual())->CalculateBones(TRUE);

	CPHSkeleton::Spawn(e);
	setEnabled(TRUE);
	setVisible(TRUE);
	PKinematics(Visual())->CalculateBones_Invalidate();
	PKinematics(Visual())->CalculateBones(TRUE);

	SetfHealth(co->health);

	if (!g_Alive())					b_exploded = true;
	else							b_exploded = false;

	// COP_ENGINE_PATCH: COP-ENG-057 - ParseDefinitions/Init must run before a
	// saved engine can be restored: they create the transmission, exhausts and
	// sound layers and initialize the fuel tank. The donor already restores fuel
	// and physical doors from CSE_ALifeCar; the profile additionally restores a
	// stable running/idle state without replaying the starter one-shot.
	if (m_dcp_white_niva_profile && restore_dcp_runtime)
	{
		m_fuel = clampr(saved_vehicle_fuel, 0.f, m_fuel_tank);
		if ((saved_vehicle_runtime_flags & 0x01) && m_fuel > EPS && g_Alive())
		{
			b_engine_on = true;
			b_starting = false;
			b_clutch = false;
			m_current_rpm = m_min_rpm;
			m_current_engine_power = Parabola(m_current_rpm);
			PlayExhausts();
			m_car_sound->Drive();
		}
		// COP_ENGINE_PATCH: COP-ENG-058 - keep the first server sync aligned
		// with the restored stable runtime state. Otherwise its stale Engine
		// bit immediately toggles the successfully restored engine back off.
		co->Engine = b_engine_on ? 1 : 0;
	}

	CDamagableItem::RestoreEffect();
	m_condition_damage_particles_active = false;


	CInifile* pUserData = PKinematics(Visual())->LL_UserData();
	if (pUserData != nullptr)
	{
		if (pUserData->section_exist("destroyed"))
			CPHDestroyable::Load(pUserData, "destroyed");

		if (pUserData->section_exist("mounted_weapon_definition"))
		{
			// COP_ENGINE_PATCH: COP-ENG-201 - template sections with absent bones
			// must not construct a phantom weapon or dereference BI_NONE.
			IKinematics* K = PKinematics(Visual());
			bool valid = true;
			for (LPCSTR key : { "rotate_x_bone", "rotate_y_bone", "fire_bone" })
				valid = valid && pUserData->line_exist("mounted_weapon_definition", key) &&
					K->LL_BoneID(pUserData->r_string("mounted_weapon_definition", key)) != BI_NONE;
			if (valid)
				m_car_weapon = new CCarWeapon(this);
			else
				Msg("! Vehicle [%s] has an incomplete mounted weapon skeleton", cNameSect().c_str());
		}

		if (pUserData->line_exist("car_definition", "trunk_bone"))
			m_bone_trunk = PKinematics(Visual())->LL_BoneID(pUserData->r_string("car_definition", "trunk_bone"));

		if (pUserData->section_exist("visual_memory_definition"))
		{
			m_memory = new car_memory(this);
			m_memory->reload(pUserData->r_string("visual_memory_definition", "section"));
		}
	}

	const BOOL spawned = CScriptEntity::net_Spawn(DC) && R;
	if (spawned)
    {
        LoadGrassSpheres(pUserData);
		RegisterProfileCollisionSoundOwner();
    }
	return spawned;
}

void CCar::ActorObstacleCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
	if (!do_colide)
	{
		if (material_1 && material_1->Flags.test(SGameMtl::flActorObstacle))do_colide = true;
		if (material_2 && material_2->Flags.test(SGameMtl::flActorObstacle))do_colide = true;
	}
}

void CCar::CharacterContactCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl*, SGameMtl*)
{
	// COP_ENGINE_PATCH: COP-ENG-123 - notify the actual AI controller on the
	// opposite side of a two-body vehicle contact, including wheels and doors.
	if (do_colide && dGeomGetBody(c.geom.g1) && dGeomGetBody(c.geom.g2))
	{
		const dxGeomUserData* other = retrieveGeomUserData(bo1 ? c.geom.g2 : c.geom.g1);
		if (other)
			NotifyCharacterVehicleContact(other->ph_object);
	}
}

void CCar::SpawnInitPhysics(CSE_Abstract* D)
{
	CSE_PHSkeleton* so = smart_cast<CSE_PHSkeleton*>(D);
	R_ASSERT(so);
	ParseDefinitions();//parse ini filling in m_driving_wheels,m_steering_wheels,m_breaking_wheels
	CreateSkeleton(D);//creates m_pPhysicsShell & fill in bone_map
	IKinematics* K = smart_cast<IKinematics*>(Visual());
	K->CalculateBones_Invalidate();//this need to call callbacks
	K->CalculateBones(TRUE);
	Init();
	// COP_ENGINE_PATCH: COP-ENG-123 - applies to occupied and parked vehicles;
	// actor attach/detach must not remove the AI contact notification.
	PPhysicsShell()->add_ObjectContactCallback(CharacterContactCallback);

	SetDefaultNetState(so);
	CPHUpdateObject::Activate();
}

void CCar::net_Destroy()
{
    ClearGrassSpheres();
	// COP_ENGINE_PATCH: COP-ENG-076 - remove the physics-holder address before
	// any shell, sound or game-object state can be torn down. A collision task
	// already using the entry holds the same registry lock until playback ends.
	UnregisterProfileCollisionSoundOwner();
#ifdef DEBUG
	DBgClearPlots();
#endif

	IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
	if (m_bone_steer != BI_NONE)
	{
		pKinematics->LL_GetBoneInstance(m_bone_steer).reset_callback();
	}
	if (m_rpm_b != BI_NONE)
	{
		pKinematics->LL_GetBoneInstance(m_rpm_b).reset_callback();
	}
	if (m_speed_b != BI_NONE)
	{
		pKinematics->LL_GetBoneInstance(m_speed_b).reset_callback();
	}
	if (m_fuel_b != BI_NONE)
	{
		pKinematics->LL_GetBoneInstance(m_fuel_b).reset_callback();
	}
	if (m_dcp_fan_bone != BI_NONE)
	{
		pKinematics->LL_GetBoneInstance(m_dcp_fan_bone).reset_callback();
	}
	for (SDcpSuspensionVisual* visual : m_dcp_suspension_visuals)
		pKinematics->LL_GetBoneInstance(visual->bone_id).reset_callback();
	ClearDcpSuspensionVisuals();
	for (SDcpGaugeVisual* visual : m_dcp_gauge_visuals)
		pKinematics->LL_GetBoneInstance(visual->bone_id).reset_callback();
	ClearDcpGaugeVisuals();

	CScriptEntity::net_Destroy();
	inherited::net_Destroy();
	CExplosive::net_Destroy();
	if (m_pPhysicsShell)
	{
		m_pPhysicsShell->Deactivate();
		m_pPhysicsShell->ZeroCallbacks();
		xr_delete(m_pPhysicsShell);
	}
	CHolderCustom::detach_Actor();
	ClearExhausts();
	m_wheels_map.clear();
	m_steering_wheels.clear();
	m_driving_wheels.clear();
	m_exhausts.clear();
	m_breaking_wheels.clear();
	m_doors.clear();
	m_gear_ratious.clear();
	m_dcp_gear_min_speed.clear();
	m_car_sound->Destroy();
	CPHUpdateObject::Deactivate();
	CPHSkeleton::RespawnInit();
	m_damage_particles.Clear();
	CPHDestroyable::RespawnInit();
	CPHCollisionDamageReceiver::Clear();
	b_breaks = false;
}

void CCar::net_Save(NET_Packet& P)
{
	inherited::net_Save(P);
	SaveNetState(P);
}

BOOL CCar::net_SaveRelevant()
{
	return TRUE;
}

void CCar::SaveNetState(NET_Packet& P)
{
	CPHSkeleton::SaveNetState(P);
	P.w_vec3(Position());

	Fvector Angle;
	XFORM().getXYZ(Angle);
	P.w_vec3(Angle);

	P.w_u16(u16(m_doors.size()));
	for (auto& [ID, CarObj] : m_doors)
		CarObj.SaveNetState(P);

	P.w_u16(u16(m_wheels_map.size()));
	for (auto& [ID, WhellData] : m_wheels_map)
		WhellData.SaveNetState(P);

	P.w_float(GetfHealth());
	// COP_ENGINE_PATCH: COP-ENG-057 - keep the donor's server-side fuel
	// placement and append one versioned stable-state bit. Transient starter,
	// clutch and RPM values are deliberately not persisted.
	P.w_float(m_fuel);
	// COP_ENGINE_PATCH: COP-ENG-159 - keep the old q8 block for legacy loading,
	// but persist an authoritative, full-float root-relative pose for this car.
	// Recreate exactly the root rotation represented by the saved Euler angles.
	Fmatrix saved_root, inverse_root;
	saved_root.setXYZ(Angle);
	saved_root.c.set(Position());
	inverse_root.invert(saved_root);
	SVehiclePoseSnapshot pose;
	IKinematics* kinematics = PKinematics(Visual());
	pose.root = kinematics->LL_GetBoneRoot();
	pose.mask = kinematics->LL_GetBonesVisible();
	const u16 count = PHGetSyncItemsNumber();
	if (count <= SVehiclePoseSnapshot::max_elements && PPhysicsShell() && PPhysicsShell()->isActive())
	{
		pose.elements.reserve(count);
		for (u16 i = 0; i < count; ++i)
		{
			SPHNetState state;
			PHGetSyncItem(i)->get_State(state);
			if (!pose.append(PPhysicsShell()->get_ElementByStoreOrder(i)->m_SelfID, state, inverse_root, saved_root.c))
				break;
		}
	}
	const bool exact_pose = pose.elements.size() == count && pose.valid();
	if (exact_pose)
		P.w_u8(SVehiclePoseSnapshot::runtime_version);
	else
		P.w_u8(1);
	P.w_u8(b_engine_on ? 0x01 : 0x00);
	if (exact_pose)
		pose.write(P);
}

void CCar::RestoreNetState(CSE_PHSkeleton* po)
{
	if (!po->_flags.test(CSE_PHSkeleton::flSavedData))
		return;

	CSE_ALifeCar* co = smart_cast<CSE_ALifeCar*>(po);
	// COP_ENGINE_PATCH: COP-ENG-159 - replace the decoded q8 values before the
	// ordinary skeleton restore resets interpolation, velocities and sleep bits.
	// No physics step, waking pass or correction of legacy saves is performed.
	bool exact_pose = co->vehicle_pose.matches_legacy(co->saved_bones) &&
		PPhysicsShell() && co->vehicle_pose.elements.size() == PHGetSyncItemsNumber();
	if (exact_pose)
	{
		for (u16 i = 0; i < PHGetSyncItemsNumber(); ++i)
			if (PPhysicsShell()->get_ElementByStoreOrder(i)->m_SelfID != co->vehicle_pose.elements[i].bone)
				exact_pose = false;
	}
	if (exact_pose)
	{
		Fmatrix saved_root;
		saved_root.setXYZ(co->o_Angle);
		saved_root.c.set(co->o_Position);
		PHNETSTATE_VECTOR states;
		states.reserve(PHGetSyncItemsNumber());
		for (u16 i = 0; i < PHGetSyncItemsNumber(); ++i)
		{
			SPHNetState state = co->vehicle_pose.world_state(i, saved_root);
			if (!_valid(state.position) || !valid_pos(state.position) || !_valid(state.quaternion))
				exact_pose = false;
			states.push_back(state);
		}
		if (exact_pose)
			co->saved_bones.bones.swap(states);
	}
	if (!exact_pose && !co->vehicle_pose.elements.empty())
		Msg("! [vehicle pose] model/pose mismatch, using legacy state: %s", cName().c_str());
	co->vehicle_pose.clear();
	CPHSkeleton::RestoreNetState(po);

	size_t StackIterator = 0;
	for (auto& [ID, CarObj] : m_doors)
	{
		CSE_ALifeCar::SDoorState door_state = *(co->door_states.begin() + StackIterator);
		if (m_part_damage_body_only)
		{
			door_state.health = CarObj.MaxHealth();
			if (door_state.open_state == u8(CCarDoor::broken))
				door_state.open_state = u8(CCarDoor::opened);
		}
		CarObj.RestoreNetState(door_state);
		StackIterator++;
	}

	StackIterator = 0;
	for (auto& [ID, WhellData] : m_wheels_map)
	{
		CSE_ALifeCar::SWheelState wheel_state = *(co->wheel_states.begin() + StackIterator);
		if (m_part_damage_body_only)
			wheel_state.health = WhellData.MaxHealth();
		WhellData.RestoreNetState(wheel_state);
		StackIterator++;
	}

	if (exact_pose)
	{
		// Bodies already have their authoritative full pose. The legacy root
		// realignment would round-trip it again and must not be applied here.
		m_pPhysicsShell->GetGlobalTransformDynamic(&XFORM());
		PKinematics(Visual())->CalculateBones_Invalidate();
		PKinematics(Visual())->CalculateBones(TRUE);
		return;
	}

	//as later may kill diable/enable state save it;
	bool enable = PPhysicsShell()->isEnabled();

	Fmatrix restored_form;
	m_pPhysicsShell->GetGlobalTransformDynamic(&restored_form);

	Fmatrix inv, replace, sof;
	sof.setXYZ(co->o_Angle.x, co->o_Angle.y, co->o_Angle.z);
	sof.c.set(co->o_Position);
	inv.set(restored_form);
	inv.invert();
	replace.mul(sof, inv);

	PKinematics(Visual())->CalculateBones_Invalidate();
	PKinematics(Visual())->CalculateBones(TRUE);
	m_pPhysicsShell->DisableCollision();

	Fvector center; Center(center);
	Fvector obj_size; BoundingBox().getsize(obj_size);

	get_box(m_pPhysicsShell, restored_form, obj_size, center);

	Fvector ap = Fvector().set(0, 0, 0);

	ActivateShapePhysShellHolder(this, XFORM(), obj_size, center, ap);

	m_pPhysicsShell->EnableCollision();

	replace.mul(sof, inv);
	m_pPhysicsShell->TransformPosition(replace, mh_clear);

	if (enable)
		m_pPhysicsShell->Enable();
	else
		m_pPhysicsShell->Disable();

	m_pPhysicsShell->GetGlobalTransformDynamic(&XFORM());
}

void CCar::SetDefaultNetState(CSE_PHSkeleton* po)
{
	if (po->_flags.test(CSE_PHSkeleton::flSavedData))
		return;

	for (auto& [ID, CarObj] : m_doors)
	{
		CarObj.SetDefaultNetState();
	}
}

void CCar::save(NET_Packet& output_packet)
{
	inherited::save(output_packet);
}

void CCar::load(IReader& input_packet)
{
	inherited::load(input_packet);
}

void CCar::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
	if (CPHDestroyable::Destroyed())CPHDestroyable::SheduleUpdate(dt);
	else	CPHSkeleton::Update(dt);

	if (CDelayedActionFuse::isActive() && CDelayedActionFuse::Update(GetfHealth()))
	{
		//CarExplode();
	}
	if (b_exploded && !m_explosion_flags.test(flExploding) && !getEnabled())//!m_bExploding
		setEnabled(TRUE);
#ifdef DEBUG
	DbgSheduleUpdate();
#endif
}

void CCar::UpdateEx(float fov)
{
#ifdef DEBUG
	DbgUbdateCl();
#endif

	VisualUpdate(fov);

	CActor* OwnerLocal = OwnerActor();
	if (OwnerLocal && OwnerLocal->IsMyCamera() && OwnerLocal->HasCameraEffector())
	{
		cam_Update(Device.fTimeDelta, fov);
		OwnerLocal->Cameras().UpdateFromCamera(Camera());
		OwnerLocal->Cameras().ApplyDevice(Device.fViewportNear);
	}
}

BOOL CCar::AlwaysTheCrow()
{
	return (m_car_weapon && m_car_weapon->IsActive());
}

void CCar::UpdateCL()
{
	inherited::UpdateCL();
	CExplosive::UpdateCL();
    UpdateGrassSpheres();
	if (m_car_weapon)
	{
		if (m_car_weapon->UsesProfileInput() && m_car_weapon->IsActive() && OwnerActor() && IsMyCar())
		{
			if (IsWeaponExternalView())
			{
				// COP_ENGINE_PATCH: COP-ENG-205 - converge on the external camera's
				// centre ray, instead of firing a parallel ray metres below it.
				collide::rq_result hit;
				float range = 1000.f;
				if (Level().ObjectSpace.RayPick(active_camera->vPosition, active_camera->vDirection,
					range, collide::rqtBoth, hit, this)) range = hit.range;
				Fvector target;
				target.mad(active_camera->vPosition, active_camera->vDirection, range);
				m_car_weapon->SetParam(CCarWeapon::eWpnDesiredPos, target);
			}
			else m_car_weapon->SetParam(CCarWeapon::eWpnDesiredDir, active_camera->vDirection);
		}
		m_car_weapon->UpdateCL();
		if (m_memory)
			m_memory->set_camera(m_car_weapon->ViewCameraPos(), m_car_weapon->ViewCameraDir(), m_car_weapon->ViewCameraNorm());
	}
	ASCUpdate();

	if (Owner())
	{
		UpdateEx(Owner()->cast_actor()->currentFOV());
		return;
	}

	VisualUpdate(90);
	if (GetScriptControl())
		ProcessScripts();
}

void CCar::VisualUpdate(float fov)
{
	Fvector lin_vel = zero_vel;
	if(m_pPhysicsShell)
	{
		if (IsMyCar() || IsGameTypeSingle())
		{
			m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());
		}
		else
		{
			Interpolate();
		}
		m_pPhysicsShell->get_LinearVel(lin_vel);
	}

	m_car_sound->Update();
	if (Owner())
	{
		if (m_pPhysicsShell->isEnabled() || OnClient())
		{
			Owner()->XFORM().mul_43(XFORM(), m_sits_transforms[0]);
		}

		if (CurrentGameUI() != nullptr && CurrentGameUI()->UIMainIngameWnd != nullptr && IsMyCar())
		{
			CUICarPanel& Panel = CurrentGameUI()->UIMainIngameWnd->CarPanel();
			Panel.SetEngineLamp(b_engine_on);
			Panel.SetLightLamp(m_lights.IsOn());

			Panel.SetCarHealth(GetfHealth(), m_condition_warning_health, m_condition_critical_health);
			Panel.SetCarFuel(m_fuel, m_fuel_tank, m_fuel_system_enabled);
			// COP_ENGINE_PATCH: COP-ENG-206 - inventory-backed ammunition is visible only to the gunner.
			const bool show_ammo = IsWeaponMode() && m_car_weapon && m_car_weapon->UsesInventoryAmmo();
			Panel.SetWeaponAmmo(show_ammo, show_ammo ? m_car_weapon->InventoryAmmoName() : "",
				show_ammo ? m_car_weapon->InventoryAmmoCount() : 0);

			Panel.SetSpeed(lin_vel.magnitude() / 1000.f * 3600.f / 100.f);
			Panel.SetRPM(m_current_rpm / m_max_rpm / 2.f);
			Panel.SetSpeedMode(xr_string::ToString((int)m_current_transmission_num));
		}
	}

	UpdateConditionDamageParticles();
	UpdateExhausts();
	m_lights.Update();
}

void CCar::renderable_Render()
{
	inherited::renderable_Render();
	if (m_car_weapon)
		m_car_weapon->Render_internal();
}

float CCar::InterpolateStates(u32 element, SCarNetUpdate const& first, SCarNetUpdate const& last, SPHNetState& current)
{
	u32 CurTime = Device.dwTimeGlobal;

	const u32 denom = last.TimeStamp - first.TimeStamp;
	if (denom == 0)
	{
		const Fvector& pos2 = last.StateVec[element].position;
		current.position = pos2;
		current.quaternion = last.StateVec[element].quaternion;
		return 1.f;
	}

	float factor = float(CurTime - first.TimeStamp) / float(denom);
	float result = factor;
	clamp(factor, 0.f, 1.f);

	const Fvector& pos1 = first.StateVec[element].position;
	const Fvector& pos2 = last.StateVec[element].position;

	current.position.x = pos1.x + (factor * (pos2.x - pos1.x));
	current.position.y = pos1.y + (factor * (pos2.y - pos1.y));
	current.position.z = pos1.z + (factor * (pos2.z - pos1.z));

	current.quaternion.slerp(first.StateVec[element].quaternion, last.StateVec[element].quaternion, factor);
	return result;
}


void CCar::Interpolate()
{
	if (!getVisible() || !m_pPhysicsShell || m_CarNetUpdates.empty())
		return;

	// simple linear interpolation...
	if (m_CarNetUpdates.size() >= 2)
	{
		auto& first = m_CarNetUpdates.front();
		auto& last = m_CarNetUpdates.back();
		u32 elementsCnt = first.StateVec.size();

		for (u32 i = 0; i < elementsCnt; i++)
		{
			SPHNetState newState = m_CarNetUpdates.back().StateVec[i];
			float f = InterpolateStates(i, first, last, newState);

			if (f == 0)
				return;

			CPHSynchronize* pSyncObj = this->PHGetSyncItem(i);
			SPHNetState oldState;
			pSyncObj->get_State(oldState);

			newState.previous_position = oldState.position;
			newState.previous_quaternion = oldState.quaternion;
			pSyncObj->set_State(newState);
		}
	}

	m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());
}

void CCar::SyncRead(NET_Packet& P)
{
	u8 flags = P.r_u8();
	std::bitset<8> BitsIn(flags);

	u8 engine = BitsIn.test(0);
	u8 light = BitsIn.test(1);

	u16 owner;
	P.r_u16(owner);
	m_current_transmission_num = P.r_u32();

	if (owner != u16(-1))
	{
		if (!(IsMyCar()))
		{
			if (auto* act = smart_cast<CActor*>(Level().Objects.net_Find(owner)))
				act->attach_Vehicle(this);
		}
	}
	else
	{
		if (OwnerActor())
			detach_Actor();
	}

	if (!IsMyCar())
	{
		bool e = !!engine;

		if (e != b_engine_on)
			SwitchEngine();

		bool l = !!light;

		if (l != m_lights.IsOn())
			m_lights.SwitchHeadLights();
	}

	u16 cnt;
	P.r_u16(cnt);

	SCarNetUpdate update;
	update.TimeStamp = Device.dwTimeGlobal;

	for (int i = 0; i < cnt; i++)
	{
		SPHNetState state{ 0 };

		P.r_vec3(state.position);
		P.r_float_q8(state.quaternion.x, -1.0, 1.0);
		P.r_float_q8(state.quaternion.y, -1.0, 1.0);
		P.r_float_q8(state.quaternion.z, -1.0, 1.0);
		P.r_float_q8(state.quaternion.w, -1.0, 1.0);

		update.StateVec.push_back(state);
	}

	if (!IsMyCar())
		m_CarNetUpdates.push_back(update);

	while (m_CarNetUpdates.size() > 2)
		m_CarNetUpdates.pop_front();
}

void CCar::SyncWrite(NET_Packet& P)
{
	std::bitset<8> BitsOut;
	BitsOut.set(0, b_engine_on);
	BitsOut.set(1, m_lights.IsOn());

	P.w_u8(static_cast<u8>(BitsOut.to_ulong()));

	if (OwnerActor())
		P.w_u16(OwnerActor()->ID());
	else
		P.w_u16(u16(-1));

	P.w_u32((u32)m_current_transmission_num);

	if (OnClient() || m_CarNetUpdates.empty())
	{
		P.w_u16(PHGetSyncItemsNumber());

		for (int i = 0; i < PHGetSyncItemsNumber(); i++)
		{
			auto item = PHGetSyncItem(i);

			SPHNetState State;
			item->get_State(State);

			P.w_vec3(State.position);
			P.w_float_q8(State.quaternion.x, -1.0, 1.0);
			P.w_float_q8(State.quaternion.y, -1.0, 1.0);
			P.w_float_q8(State.quaternion.z, -1.0, 1.0);
			P.w_float_q8(State.quaternion.w, -1.0, 1.0);
		}
	}
	else
	{
		auto& vec = m_CarNetUpdates.back().StateVec;
		P.w_u16(vec.size());

		for (int i = 0; i < vec.size(); i++)
		{
			SPHNetState State = vec[i];

			P.w_vec3(State.position);
			P.w_float_q8(State.quaternion.x, -1.0, 1.0);
			P.w_float_q8(State.quaternion.y, -1.0, 1.0);
			P.w_float_q8(State.quaternion.z, -1.0, 1.0);
			P.w_float_q8(State.quaternion.w, -1.0, 1.0);
		}
	}
}

void CCar::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
}

void CCar::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
}

void	CCar::OnHUDDraw(CCustomHUD* /**hud*/)
{
#ifdef DEBUG
	Fvector velocity;
	m_pPhysicsShell->get_LinearVel(velocity);
	UI().Font().pFontStat->SetColor(0xffffffff);
	UI().Font().pFontStat->OutSet(120, 530);
	UI().Font().pFontStat->OutNext("Position:      [%3.2f, %3.2f, %3.2f]", VPUSH(Position()));
	UI().Font().pFontStat->OutNext("Velocity:      [%3.2f]", velocity.mul(3.6f).magnitude());
#endif
}

void	CCar::Hit(SHit* pHDS)
{

	SHit	HDS = *pHDS;
	// COP_ENGINE_PATCH: COP-ENG-061 - body_only keeps DCP wheel suspension and
	// door hinges mechanically intact while preserving ordinary hull damage.
	// Cars without the opt-in mode continue through the legacy part-damage path.
	if (!m_part_damage_body_only)
		WheelHit(HDS.damage(), HDS.bone(), HDS.hit_type);
	DoorHit(HDS.damage(), HDS.bone(), HDS.hit_type);
	float hitScale = 1.f, woundScale = 1.f;
	if (HDS.hit_type != ALife::eHitTypeStrike) CDamageManager::HitScale(HDS.bone(), hitScale, woundScale);
	HDS.power *= GetHitImmunity(HDS.hit_type) * hitScale;
	if (m_dcp_white_niva_profile)
	{
		if (b_engine_on && HDS.damage() >= m_hit_stall_threshold)
			Stall();
		HDS.power /= m_durability_factor;
	}

	inherited::Hit(&HDS);
	if (!CDelayedActionFuse::isActive())
	{
		CDelayedActionFuse::CheckCondition(GetfHealth());
	}
	CDamagableItem::HitEffect();
}

void CCar::ChangeCondition(float fDeltaCondition)
{

	CEntity::CalcCondition(-fDeltaCondition);
	CDamagableItem::HitEffect();
	if (Local() && !g_Alive() && !AlreadyDie())
		KillEntity(Initiator());
}

bool CCar::BuildProfileTrunkInteractionObb(u16 bone_id, Fobb& interaction_obb)
{
	if (!UsesProfileTrunkInteraction() || Visual() == nullptr)
		return false;

	IKinematics* kinematics = PKinematics(Visual());
	if (kinematics == nullptr || bone_id == BI_NONE || bone_id >= kinematics->LL_BoneCount())
		return false;

	// COP_ENGINE_PATCH: COP-ENG-051
	// Prefer the visual bone OBB: it follows the physical hinge but covers the
	// complete visible lid instead of only its narrow collision primitive.
	Fobb local_obb = kinematics->LL_GetBox(bone_id);
	Fmatrix shape_transform;
	if (local_obb.m_halfsize.square_magnitude() > EPS_S)
	{
		local_obb.xform_get(shape_transform);
	}
	else
	{
		const SBoneShape& shape = kinematics->LL_GetData(bone_id).shape;
		shape_transform.identity();

		switch (shape.type)
		{
		case SBoneShape::stBox:
			local_obb = shape.box;
			local_obb.xform_get(shape_transform);
			break;
		case SBoneShape::stSphere:
			shape_transform.c = shape.sphere.P;
			local_obb.m_halfsize.set(shape.sphere.R, shape.sphere.R, shape.sphere.R);
			break;
		case SBoneShape::stCylinder:
		{
			Fvector axis = shape.cylinder.m_direction;
			if (axis.square_magnitude() <= EPS_S)
				return false;
			axis.normalize();
			Fvector first_axis, second_axis;
			Fvector::generate_orthonormal_basis(axis, first_axis, second_axis);
			shape_transform.i = first_axis;
			shape_transform.j = second_axis;
			shape_transform.k = axis;
			shape_transform.c = shape.cylinder.m_center;
			local_obb.m_halfsize.set(shape.cylinder.m_radius, shape.cylinder.m_radius,
				shape.cylinder.m_height * 0.5f);
			break;
		}
		default:
			return false;
		}
	}

	Fvector local_offset;
	shape_transform.transform_dir(local_offset, m_trunk_interaction_offset);
	shape_transform.c.add(local_offset);

	Fmatrix bone_transform, model_shape, world_shape;
	kinematics->LL_GetBoneLocalTransform(bone_id, bone_transform);
	model_shape.mul_43(bone_transform, shape_transform);
	world_shape.mul_43(XFORM(), model_shape);
	interaction_obb.xform_set(world_shape);
	interaction_obb.m_halfsize.set(
		_max(local_obb.m_halfsize.x + m_trunk_interaction_padding.x, EPS_L),
		_max(local_obb.m_halfsize.y + m_trunk_interaction_padding.y, EPS_L),
		_max(local_obb.m_halfsize.z + m_trunk_interaction_padding.z, EPS_L));
	return true;
}

bool CCar::TryProfileTrunk(const Fvector& ray_start, const Fvector& ray_direction, float* hit_distance)
{
	if (ray_direction.square_magnitude() <= EPS_S)
		return false;

	// COP_ENGINE_PATCH: COP-ENG-081 - every physical leaf of a compound DCP
	// trunk owns an equivalent interaction volume. Aiming at either rear door
	// therefore dispatches the same atomic open-inventory action instead of
	// toggling the auxiliary door independently through the generic door path.
	bool hit = false;
	float nearest_range = m_trunk_use_distance;
	auto test_bone = [&](u16 bone_id)
	{
		Fobb interaction_obb;
		if (!BuildProfileTrunkInteractionObb(bone_id, interaction_obb))
			return;
		float range = m_trunk_use_distance;
		if (interaction_obb.intersect(ray_start, ray_direction, range) && (!hit || range < nearest_range))
		{
			hit = true;
			nearest_range = range;
		}
	};

	test_bone(m_bone_trunk);
	for (u16 bone_id : m_trunk_aux_bones)
		test_bone(bone_id);
	if (!hit)
		return false;

	if (hit_distance != nullptr)
		*hit_distance = nearest_range;
	return true;
}

bool CCar::IsProfileTrunkInRange(const Fvector& world_position)
{
	auto bone_in_range = [&](u16 bone_id)
	{
		Fobb interaction_obb;
		if (!BuildProfileTrunkInteractionObb(bone_id, interaction_obb))
			return false;

		Fvector relative;
		relative.sub(world_position, interaction_obb.m_translate);
		Fvector local;
		local.set(relative.dotproduct(interaction_obb.m_rotate.i), relative.dotproduct(interaction_obb.m_rotate.j),
			relative.dotproduct(interaction_obb.m_rotate.k));

		Fvector outside;
		outside.set(_max(_abs(local.x) - interaction_obb.m_halfsize.x, 0.f),
			_max(_abs(local.y) - interaction_obb.m_halfsize.y, 0.f),
			_max(_abs(local.z) - interaction_obb.m_halfsize.z, 0.f));
		return outside.square_magnitude() <= _sqr(m_trunk_use_distance);
	};

	if (bone_in_range(m_bone_trunk))
		return true;
	for (u16 bone_id : m_trunk_aux_bones)
	{
		if (bone_in_range(bone_id))
			return true;
	}
	return false;
}

bool CCar::TryTrunk()
{
	if (m_bone_trunk == BI_NONE)
		return false;
	if (UsesProfileTrunkInteraction())
		return TryProfileTrunk(Device.vCameraPosition, Device.vCameraDirection);

	RQR.r_clear();
	const float trunk_query_distance = m_dcp_white_niva_profile ? m_trunk_use_distance : 3.f;
	collide::ray_defs Q(Device.vCameraPosition, Device.vCameraDirection, trunk_query_distance, CDB::OPT_CULL, collide::rqtObject);

	if (g_pGameLevel->ObjectSpace.RayQuery(RQR, collidable.model, Q))
	{

		collide::rq_results& R = RQR;
		int y = R.r_count();

		for (int k = 0; k < y; ++k)
		{
			collide::rq_result* I = R.r_begin() + k;
			if (IsGameTypeSingle() && m_bone_trunk == (u16)I->element &&
				(!m_dcp_white_niva_profile || I->range <= m_trunk_use_distance))
			{
				return true;
			}
		}
	}

	return false;
}

bool CCar::TryUsableBones()
{
	if (UsableBones.empty())
		return false;

	UsableBonesActive = BI_NONE;

	RQR.r_clear();
	collide::ray_defs Q(Device.vCameraPosition, Device.vCameraDirection, 3.f, CDB::OPT_CULL, collide::rqtObject);

	if (g_pGameLevel->ObjectSpace.RayQuery(RQR, collidable.model, Q))
	{
		collide::rq_results& R = RQR;
		int y = R.r_count();

		for (int k = 0; k < y; ++k)
		{
			collide::rq_result* I = R.r_begin() + k;
			if (IsGameTypeSingle() && UsableBones.contains(I->element))
			{
				UsableBonesActive = I->element;
				return true;
			}
		}
	}


	return false;
}

void CCar::PHHit(SHit& H)
{
	if (!m_pPhysicsShell)	return;
	if (m_bone_steer == H.bone()) return;
	if (CPHUpdateObject::IsActive())
	{
		Fvector vimpulse; vimpulse.set(H.direction());
		vimpulse.mul(H.phys_impulse());
		vimpulse.y *= GravityFactorImpulse();
		float mag = vimpulse.magnitude();
		if (!fis_zero(mag))
		{
			vimpulse.mul(1.f / mag);
			m_pPhysicsShell->applyHit(H.bone_space_position(), vimpulse, mag, H.bone(), H.type());
		}

	}
	else
	{
		m_pPhysicsShell->applyHit(H.bone_space_position(), H.direction(), H.phys_impulse(), H.bone(), H.type());
	}
}


void CCar::ApplyDamage(u16 level)
{
	CDamagableItem::ApplyDamage(level);
	switch (level)
	{
	case 1:
		if (m_condition_smoke_health < 0.f)
			m_damage_particles.Play1(this);
		break;
	case 2:
	{
		if (m_condition_smoke_health < 0.f && !CDelayedActionFuse::isActive())
		{
			CDelayedActionFuse::CheckCondition(GetfHealth());
		}
		if (m_condition_smoke_health < 0.f)
			m_damage_particles.Play2(this);
	}
	break;
	case 3: m_fuel = 0.f;

	}

}
void CCar::detach_Actor()
{
	if (!Owner()) return;
	// COP_ENGINE_PATCH: COP-ENG-205 - either operator view returns to the driver on detach.
	if (active_camera == m_weapon_camera || active_camera == m_weapon_external_camera) SwitchVehicleRole(false);
	// COP_ENGINE_PATCH: COP-ENG-201 - leaving the holder cannot leave a firing motor running.
	if (m_car_weapon && m_car_weapon->UsesProfileInput())
		m_car_weapon->Action(CCarWeapon::eWpnActivate, 0);
	Owner()->setVisible(1);
	CHolderCustom::detach_Actor();
	PPhysicsShell()->remove_ObjectContactCallback(ActorObstacleCallback);
	NeutralDrive();
	Unclutch();
	ResetKeys();
	m_current_rpm = m_min_rpm;

	HandBreak();
	processing_deactivate();
#ifdef DEBUG
	DBgClearPlots();
#endif

	if (!g_dedicated_server)
	{
		CUICarPanel& Panel = CurrentGameUI()->UIMainIngameWnd->CarPanel();
		Panel.Show(false);
	}
}

bool CCar::attach_Actor(CGameObject* actor)
{
	if (Owner() || CPHDestroyable::Destroyed()) return false;
	CHolderCustom::attach_Actor(actor);

	IKinematics* K = smart_cast<IKinematics*>(Visual());
	CInifile* ini = K->LL_UserData();
	int id;

	Fmatrix	driver_xform;
	if (ini->line_exist("car_definition", "driver_position") && ini->line_exist("car_definition", "driver_direction"))
	{
		driver_xform.c.set(ini->r_fvector3("car_definition", "driver_position"));
		driver_xform.k.set(ini->r_fvector3("car_definition", "driver_direction"));
	}
	else if (ini->line_exist("car_definition", "driver_place")) {

		id = K->LL_BoneID(ini->r_string("car_definition", "driver_place"));
		CBoneInstance& instance = K->LL_GetBoneInstance(u16(id));
		driver_xform.set(instance.mTransform);
	}
	else {
		Owner()->setVisible(0);
		id = K->LL_GetBoneRoot();
		CBoneInstance& instance = K->LL_GetBoneInstance(u16(id));
		driver_xform.set(instance.mTransform);
	}

	m_sits_transforms.push_back(driver_xform);
	// COP_ENGINE_PATCH: COP-ENG-202 - scripts can force detach without first
	// calling Use/Exit. Keep a panel ID, not a world point that goes stale while driving.
	if (m_dcp_white_niva_profile)
	{
		m_exit_bone = BI_NONE;
		for (auto& [bone, door] : m_doors)
			if (!door.non_enterable && bone != m_bone_trunk)
			{
				m_exit_bone = bone;
				break;
			}
	}
	OnCameraChange(ectFirst);
	PPhysicsShell()->Enable();
	PPhysicsShell()->add_ObjectContactCallback(ActorObstacleCallback);

	processing_activate();
	ReleaseHandBreak();

	if (!g_dedicated_server)
	{
		CUICarPanel& Panel = CurrentGameUI()->UIMainIngameWnd->CarPanel();
		Panel.Show(true);
	}
	return true;
}

bool CCar::is_Door(u16 id, xr_map<u16, CCarDoor>::iterator& i)
{
	const u16 door_bone = resolve_door_bone(id);
	if (door_bone != BI_NONE)
	{
		i = m_doors.find(door_bone);
		if (i->second.Joint)
			return true;
	}
	return false;
}

bool CCar::is_Door(u16 id)
{
	return resolve_door_bone(id) != BI_NONE;
}

u16 CCar::resolve_door_bone(u16 id) const
{
	if (m_doors.contains(id))
		return id;

	if (!m_dcp_white_niva_profile || Visual() == nullptr)
		return BI_NONE;

	// COP_ENGINE_PATCH: COP-ENG-083 - some donor cabins keep physical window
	// frames outside the door hierarchy. Allow a profiled LTX to map an exact
	// picked helper bone to its real hinged panel without changing the OGF.
	const auto alias = m_door_use_aliases.find(id);
	if (alias != m_door_use_aliases.end() && m_doors.contains(alias->second))
		return alias->second;

	if (m_recursive_door_bones.empty())
		return BI_NONE;

	IKinematics* kinematics = smart_cast<IKinematics*>(Visual());
	if (kinematics == nullptr || id >= kinematics->LL_BoneCount())
		return BI_NONE;

	// COP_ENGINE_PATCH: COP-ENG-073 - donor doors_use_recursive names physical
	// panel roots whose child meshes must operate the same panel. Resolve the
	// hierarchy only for profiled vehicles; legacy cars keep exact-bone picks.
	u16 current = id;
	for (u16 depth = 0; current != BI_NONE && depth < kinematics->LL_BoneCount(); ++depth)
	{
		if (std::find(m_recursive_door_bones.begin(), m_recursive_door_bones.end(), current) !=
			m_recursive_door_bones.end() && m_doors.contains(current))
		{
			return current;
		}

		current = kinematics->LL_GetData(current).GetParentID();
	}

	return BI_NONE;
}

bool CCar::Enter(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos)
{
	Fvector enter_pos;
	enter_pos.add(pos, foot_pos);
	enter_pos.mul(0.5f);

	for (auto& [ID, CarObj] : m_doors)
	{
		if (CarObj.CanEnter(pos, dir, enter_pos))
		{
			//m_doors[ID].Open();
			return true;
		}
	}
	return false;
}

bool CCar::Exit(const Fvector& pos, const Fvector& dir)
{
	for (auto& [ID, CarObj] : m_doors)
	{
		if (CarObj.CanExit(pos, dir))
		{
			m_exit_bone = ID;
			CarObj.GetExitPosition(m_exit_position);
			return true;
		}
	}
	return false;

}

void CCar::cb_Speed(CBoneInstance* B)
{
	CCar* car = static_cast<CCar*>(B->callback_param()); Fmatrix m;	Fvector Vel;
	car->m_pPhysicsShell->get_LinearVel(Vel);
	if (car->m_dcp_dashboard)
	{
		// COP_ENGINE_PATCH: COP-ENG-062 - donor dashboard factors keep their
		// scale while the model-specific direction and usable range stay in LTX.
		const float speed_kmh = clampr(Vel.magnitude() * 3.6f, 0.f, _max(car->m_dcp_speedometer_max, 0.f));
		if (car->m_dcp_speedometer_body_profile)
		{
			// COP_ENGINE_PATCH: COP-ENG-151 - donor factors are radians/30 and start at min_speed.
			// Use actual chassis speed so wheelspin does not exaggerate the displayed speed.
			const float calibrated_speed = _max(speed_kmh - car->m_dcp_speedometer_min, 0.f);
			const float rotation = car->m_dcp_speedometer_direction *
				(deg2rad(car->m_dcp_speedometer_offset) +
					_abs(car->m_dcp_speedometer_factor) * calibrated_speed / 30.f);
			m.rotateZ(rotation);
			B->mTransform.mulB_43(m);
			return;
		}
		const float rotation = car->m_dcp_speedometer_direction *
			(car->m_dcp_speedometer_offset + speed_kmh * car->m_dcp_speedometer_factor);
		m.rotateZ(deg2rad(rotation));
		B->mTransform.mulB_43(m);
		return;
	}
	float m_speed = Vel.magnitude() * 0.036f;

	float current_rotation = car->m_speed_offsets.x + (car->m_speed_offsets.y - car->m_speed_offsets.x) * m_speed;
	m.rotateZ(deg2rad(clampr(current_rotation, car->m_speed_offsets.z, car->m_speed_offsets.w)));

	B->mTransform.mulB_43(m);
}

void CCar::cb_Rpm(CBoneInstance* B)
{
	CCar* car = static_cast<CCar*>(B->callback_param()); Fmatrix m;
	if (car->m_dcp_dashboard)
	{
		const float rpm = car->m_current_rpm * (60.f / (2.f * M_PI));
		const float min_rpm = _max(car->m_dcp_tachometer_min_rpm, 0.f);
		const float max_rpm = _max(car->m_dcp_tachometer_max_rpm, min_rpm);
		const float clamped_rpm = clampr(rpm, min_rpm, max_rpm);
		const float rotation = car->m_dcp_tachometer_direction *
			(car->m_dcp_tachometer_offset +
				(clamped_rpm - min_rpm) / 1000.f * 30.f * car->m_dcp_tachometer_factor);
		m.rotateZ(deg2rad(rotation));
		B->mTransform.mulB_43(m);
		return;
	}

	auto local_rpm = car->m_current_rpm / car->m_max_rpm;
	float current_rotation = car->m_rpm_offsets.x + (car->m_rpm_offsets.y - car->m_rpm_offsets.x) * local_rpm;
	m.rotateZ(deg2rad(clampr(current_rotation, car->m_rpm_offsets.z, car->m_rpm_offsets.w)));

	B->mTransform.mulB_43(m);
}

void CCar::cb_Fuel(CBoneInstance* B)
{
	CCar* car = static_cast<CCar*>(B->callback_param()); Fmatrix m;
	if (car->m_dcp_dashboard)
	{
		const float local_fuel = car->m_fuel_tank > EPS ? clampr(car->m_fuel / car->m_fuel_tank, 0.f, 1.f) : 1.f;
		m.rotateZ(deg2rad(car->m_dcp_fuel_offset + car->m_dcp_fuel_angle * local_fuel));
		B->mTransform.mulB_43(m);
		return;
	}

	auto local_fuel = car->m_fuel / car->m_fuel_tank;
	float current_rotation = car->m_fuel_offsets.x + (car->m_fuel_offsets.y - car->m_fuel_offsets.x) * local_fuel;
	m.rotateZ(deg2rad(clampr(current_rotation, car->m_fuel_offsets.z, car->m_fuel_offsets.w)));

	B->mTransform.mulB_43(m);
}

void CCar::cb_DcpFan(CBoneInstance* B)
{
	CCar* car = static_cast<CCar*>(B->callback_param());
	if (car->m_dcp_fan_update_frame != Device.dwFrame)
	{
		// COP_ENGINE_PATCH: COP-ENG-050 - exact DCP fan ramp: sixty percent
		// of configured speed per second, clamped to the requested magnitude.
		// fan_engine scales the final angular velocity by normalized engine RPM.
		const float speed_limit = _abs(car->m_dcp_fan_speed);
		const float ramp = speed_limit * 0.6f * Device.fTimeDelta;
		car->m_dcp_fan_current_speed += car->b_engine_on ? ramp : -ramp;
		clamp(car->m_dcp_fan_current_speed, 0.f, speed_limit);

		float engine_factor = 1.f;
		if (car->m_dcp_fan_engine && car->m_max_rpm > EPS)
			engine_factor = _max(car->m_current_rpm, car->m_min_rpm) / car->m_max_rpm;
		car->m_dcp_fan_angle += car->m_dcp_fan_current_speed * 3.f * Device.fTimeDelta * engine_factor;
		car->m_dcp_fan_update_frame = Device.dwFrame;
	}

	Fmatrix rotation;
	const float direction = car->m_dcp_fan_speed < 0.f ? -1.f : 1.f;
	rotation.rotateZ(car->m_dcp_fan_angle * direction);
	B->mTransform.mulB_43(rotation);
}

void CCar::ParseDefinitions()
{
	bone_map.clear();

	IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
	bone_map.insert(std::make_pair(pKinematics->LL_GetBoneRoot(), physicsBone()));
	CInifile* ini = pKinematics->LL_UserData();

	if (ini == nullptr && Device.IsEditorMode())
		return;

	R_ASSERT2(ini, "Car has no description !!! See ActorEditor Object - UserData");

	// COP_ENGINE_PATCH: COP-ENG-036
	// The enhanced vehicle path is opt-in through UserData. No section-name
	// checks are used, so every existing IX-Ray vehicle keeps the old path.
	m_dcp_white_niva_profile = false;
	m_fuel_system_enabled = false;
	m_part_damage_body_only = false;
	m_fuel_consumption_scale = 1.f;
	m_fuel_canister_amount = 20.f;
	m_fuel_refuel_min_free = 10.f;
	m_fuel_refuel_distance = 3.5f;
	m_condition_warning_health = 0.5f;
	m_condition_critical_health = 0.1f;
	m_condition_smoke_health = -1.f;
	m_condition_damage_particles_active = false;
	m_dcp_body_roll_damping_enabled = false;
	m_dcp_body_roll_damping = 0.f;
	m_dcp_body_roll_min_grounded_wheels = 2;
	m_dcp_body_roll_contact_grace_ms = 120;
	m_dcp_starter_drive_delay = 1.f;
	m_dcp_engine_start_time = 0;
	m_dcp_drive_pending = false;
	LPCSTR vehicle_feature_profile = nullptr;
	if (ini->line_exist("car_definition", "vehicle_feature_profile"))
	{
		// COP_ENGINE_PATCH: COP-ENG-040 - keep the shipped white-Niva name as a
		// compatibility alias while exposing one reusable opt-in profile for
		// later DCP vehicles. Product section names are still never inspected.
		vehicle_feature_profile = ini->r_string("car_definition", "vehicle_feature_profile");
		m_dcp_white_niva_profile = xr_strcmp(vehicle_feature_profile, "dcp_white_niva") == 0 ||
			xr_strcmp(vehicle_feature_profile, "dcp_vehicle_v1") == 0;
	}

	if (m_dcp_white_niva_profile)
	{
		// COP_ENGINE_PATCH: COP-ENG-080 - the donor has a short, fixed
		// transition before wheel drive becomes usable; it is not tied to the
		// complete starter OGG. Keep that shared delay data-driven so all DCP
		// vehicles use one value without per-section C++ checks.
		if (pSettings && vehicle_feature_profile && pSettings->section_exist(vehicle_feature_profile))
		{
			m_dcp_starter_drive_delay = clampr(READ_IF_EXISTS(pSettings, r_float,
				vehicle_feature_profile, "starter_drive_delay", 1.f), 0.f, 5.f);
		}

		// COP_ENGINE_PATCH: COP-ENG-072 - body-roll damping is a shared named
		// profile attached to the reusable DCP feature profile. It removes only
		// angular velocity around the vehicle's longitudinal axis while enough
		// wheels are grounded; donor suspension, yaw and pitch remain untouched.
		if (pSettings && vehicle_feature_profile && pSettings->section_exist(vehicle_feature_profile) &&
			pSettings->line_exist(vehicle_feature_profile, "body_roll_profile"))
		{
			LPCSTR body_roll_profile = pSettings->r_string(vehicle_feature_profile, "body_roll_profile");
			if (pSettings->section_exist(body_roll_profile))
			{
				m_dcp_body_roll_damping_enabled = READ_IF_EXISTS(pSettings, r_bool,
					body_roll_profile, "enabled", true);
				m_dcp_body_roll_damping = clampr(READ_IF_EXISTS(pSettings, r_float,
					body_roll_profile, "roll_damping", 0.f), 0.f, 10.f);
				m_dcp_body_roll_min_grounded_wheels = _max(1u, _min(READ_IF_EXISTS(pSettings, r_u32,
					body_roll_profile, "min_grounded_wheels", 2u), 8u));
				m_dcp_body_roll_contact_grace_ms = _min(READ_IF_EXISTS(pSettings, r_u32,
					body_roll_profile, "contact_grace_ms", 120u), 1000u);
			}
			else
			{
				Msg("! DCP vehicle feature profile [%s] references missing body-roll profile [%s]",
					vehicle_feature_profile, body_roll_profile);
			}
		}

		// COP_ENGINE_PATCH: COP-ENG-061 - fuel and body-only damage are reusable,
		// named opt-in profiles. No product section name is inspected in C++.
		m_part_damage_body_only = ini->line_exist("car_definition", "part_damage_mode") &&
			xr_strcmp(ini->r_string("car_definition", "part_damage_mode"), "body_only") == 0;
		if (pSettings && pSettings->line_exist(cNameSect(), "vehicle_condition_profile"))
		{
			// COP_ENGINE_PATCH: COP-ENG-068 - HUD severity and damage smoke share
			// one named profile, reusable by every future DCP vehicle.
			LPCSTR condition_profile = pSettings->r_string(cNameSect(), "vehicle_condition_profile");
			if (pSettings->section_exist(condition_profile))
			{
				m_condition_warning_health = clampr(READ_IF_EXISTS(pSettings, r_float,
					condition_profile, "warning_health", 0.5f), 0.f, 1.f);
				m_condition_critical_health = clampr(READ_IF_EXISTS(pSettings, r_float,
					condition_profile, "critical_health", 0.1f), 0.f, m_condition_warning_health);
				m_condition_smoke_health = m_condition_critical_health;
			}
			else
			{
				Msg("! DCP vehicle [%s] references missing condition profile [%s]",
					cNameSect().c_str(), condition_profile);
			}
		}
		if (ini->line_exist("car_definition", "fuel_profile"))
		{
			LPCSTR fuel_profile = ini->r_string("car_definition", "fuel_profile");
			if (pSettings && pSettings->section_exist(fuel_profile))
			{
				m_fuel_system_enabled = true;
				m_fuel_consumption_scale =
					_max(READ_IF_EXISTS(pSettings, r_float, fuel_profile, "consumption_scale", 1.f), 0.f);
				m_fuel_canister_amount =
					_max(READ_IF_EXISTS(pSettings, r_float, fuel_profile, "canister_amount", 20.f), 0.f);
				m_fuel_refuel_min_free =
					_max(READ_IF_EXISTS(pSettings, r_float, fuel_profile, "refuel_min_free", 10.f), 0.f);
				m_fuel_refuel_distance =
					_max(READ_IF_EXISTS(pSettings, r_float, fuel_profile, "refuel_distance", 3.5f), 0.f);
			}
			else
			{
				Msg("! DCP vehicle [%s] references missing fuel profile [%s]", cNameSect().c_str(), fuel_profile);
			}
		}

		m_trunk_open_inventory = ini->line_exist("car_definition", "trunk_use_mode") &&
			xr_strcmp(ini->r_string("car_definition", "trunk_use_mode"), "open_inventory") == 0;
		m_trunk_use_distance = READ_IF_EXISTS(ini, r_float, "car_definition", "trunk_use_distance", 2.f);
		m_trunk_interaction_padding = READ_IF_EXISTS(ini, r_fvector3, "car_definition",
			"trunk_interaction_padding", Fvector().set(0.05f, 0.05f, 0.05f));
		m_trunk_interaction_padding.x = _max(m_trunk_interaction_padding.x, 0.f);
		m_trunk_interaction_padding.y = _max(m_trunk_interaction_padding.y, 0.f);
		m_trunk_interaction_padding.z = _max(m_trunk_interaction_padding.z, 0.f);
		m_trunk_interaction_offset = READ_IF_EXISTS(ini, r_fvector3, "car_definition",
			"trunk_interaction_offset", Fvector().set(0.f, 0.f, 0.f));
		m_trunk_close_on_inventory_exit =
			!!READ_IF_EXISTS(ini, r_bool, "car_definition", "trunk_close_on_inventory_exit", true);
		m_durability_factor = _max(READ_IF_EXISTS(ini, r_float, "car_definition", "durability_factor", 1.f), 1.f);
		m_interior_volume =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "interior_volume", 1.f), 0.f, 1.f);
		m_collision_hit_threshold =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "collision_hit_threshold", 5.f), 0.f);
		m_hit_stall_threshold =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "hit_stall_threshold", 0.1f), 0.f);
		m_transmission_switch_delay =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "transmission_switch_delay", 0.f), 0.f);
		m_transmission_settle_delay =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "transmission_settle_delay",
				0.f), 0.f);
		m_transmission_clutch_factor =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "transmission_clutch_factor", 1.f), 0.f, 1.f);
		m_rpm_brp_increment_factor =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "rpm_brp_increment_factor", 1.f), 0.f, 1.f);
		m_rpm_brp_decrement_factor =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "rpm_brp_decrement_factor", 1.f), 0.f, 1.f);
		m_steer_nonlinear = !!READ_IF_EXISTS(ini, r_bool, "car_definition", "steer_nonlinear", false);
		m_steer_invert = !!READ_IF_EXISTS(ini, r_bool, "car_definition", "steer_invert", false);
		m_slip_sliding_threshold =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "slip_sliding_threshold", 0.6f), 0.f);
		m_vel_sliding_threshold =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "vel_sliding_threshold", 5.f), EPS);
		m_wheels_spin_factor_start =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "wheels_spin_factor_start", 0.75f), 0.f, 1.f);
		m_wheels_spin_factor_stop =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "wheels_spin_factor_stop", 0.8f), 0.f, 1.f);
		m_friction_slip_factor =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction_slip_factor", 1.f), 0.f);
		m_friction2_slip_factor =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction2_slip_factor", 1.f), 0.f);
		m_friction_spin_factor =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction_spin_factor", 1.f), 0.f);
		m_friction2_spin_factor =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction2_spin_factor", 1.f), 0.f);
		m_friction_slip_speed =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction_slip_speed", 0.005f), 0.f);
		m_friction2_steering_wheels =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction2_steering_wheels", 1.f), 0.f);
		m_friction2_nonsteering_wheels =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction2_nonsteering_wheels", 1.f), 0.f);
		m_friction2_brp_boost =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "friction2_brp_boost", 1.f), 0.f);
		m_contact_slip_scale =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "contact_slip_scale", 0.f), 0.f, 1.f);
		m_handbrake_rpm_threshold =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "handbrake_rpm_threshold", 2.f), 0.f);
		m_handbrake_rpm_decrease =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "handbrake_rpm_decrease", 1.f), 0.f);
		m_drift_start_boost =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "drift_start_boost", 0.f), 0.f);
		m_drift_boost_time =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "drift_boost_time", 0.25f), EPS);
		m_steer_spin_boost =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "steer_spin_boost", 0.f), 0.f);
		m_steer_speed_boost =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "steer_speed_boost", 0.f), 0.f);
		m_steer_turn_factor =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "steer_turn_factor", 1.f), 0.f);
		m_steer_counter_turn_factor =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "steer_counter_turn_factor", 0.5f), 0.f);
		m_steer_turn_compensation =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "steer_turn_compensation", 0.5f), 0.f);
		m_steer_turn_compensation_max =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "steer_turn_compensation_max", 0.3f), 0.f);
		m_steer_turn_delta =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "steer_turn_delta", 0.2f), 0.f);
		m_brake_boost = _max(READ_IF_EXISTS(ini, r_float, "car_definition", "brake_boost", 1.f), 0.f);
		m_camera_inertion_n =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "camera_inertion_n", 0.f), 0.f, 0.9999f);
		m_camera_inertion_k =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "camera_inertion_k", 0.f), -0.9999f, 0.9999f);
		m_camera_angle_factor =
			_max(READ_IF_EXISTS(ini, r_float, "car_definition", "camera_angle_factor", 1.f), 0.01f);
		// COP_ENGINE_PATCH: COP-ENG-205 - opt-in headroom for overhead hatches;
		// ordinary vehicle cameras retain their existing configured limits.
		if (ini->line_exist("car_definition", "camera_firsteye_pitch_up_deg"))
		{
			const float up = ini->r_float("car_definition", "camera_firsteye_pitch_up_deg");
			camera[ectFirst]->bClampPitch = TRUE;
			camera[ectFirst]->lim_pitch.x = -deg2rad(clampr(up, 0.f, 89.f));
		}
		// COP_ENGINE_PATCH: COP-ENG-207 - full head turn is opt-in for armored cabins.
		if (READ_IF_EXISTS(ini, r_bool, "car_definition", "camera_firsteye_yaw_unlimited", false))
			camera[ectFirst]->bClampYaw = FALSE;
		m_door_air_resistance_scale =
			clampr(READ_IF_EXISTS(ini, r_float, "car_definition", "door_air_resistance_scale", 0.001f), 0.f, 1.f);
	}

	// COP_ENGINE_PATCH: COP-ENG-052 - opt-in vehicle camera comfort is
	// independent from a concrete car section and from the DCP physics profile.
	// The shared profile keeps the seat attached in vehicle-local space, adds a
	// small acceleration response, and limits horizon compensation to a bounded
	// angle so rollovers still rotate the camera with the chassis.
	// COP_ENGINE_PATCH: COP-ENG-053 - every tuning value lives in the named
	// global pSettings section, so camera feel can be adjusted without rebuilding
	// xrGame. The vehicle UserData contains only the reusable profile name.
	m_vehicle_camera_dynamics = false;
	LPCSTR camera_profile = nullptr;
	if (ini->line_exist("car_definition", "camera_dynamics_profile"))
	{
		camera_profile = ini->r_string("car_definition", "camera_dynamics_profile");
		if (pSettings->section_exist(camera_profile))
		{
			LPCSTR implementation =
				READ_IF_EXISTS(pSettings, r_string, camera_profile, "implementation", "");
			m_vehicle_camera_dynamics = xr_strcmp(implementation, "vehicle_comfort_v1") == 0;
		}
	}
	if (m_vehicle_camera_dynamics)
	{
		m_camera_motion_strength =
			clampr(READ_IF_EXISTS(pSettings, r_float, camera_profile, "motion_strength", 0.35f), 0.f, 1.f);
		m_camera_horizon_limit = deg2rad(clampr(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "horizon_limit", 15.f), 0.f, 45.f));
		m_camera_motion_response =
			clampr(READ_IF_EXISTS(pSettings, r_float, camera_profile, "motion_response", 0.14f), 0.04f, 1.f);
		m_camera_motion_return =
			clampr(READ_IF_EXISTS(pSettings, r_float, camera_profile, "motion_return", 0.24f), 0.04f, 1.f);
		m_camera_acceleration_response = clampr(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "acceleration_response", 0.08f), 0.01f, 1.f);
		m_camera_max_lateral_acceleration = _max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "max_lateral_acceleration", 12.f), 0.f);
		m_camera_max_longitudinal_acceleration = _max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "max_longitudinal_acceleration", 10.f), 0.f);
		m_camera_lateral_offset_per_accel = _max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "lateral_offset_per_accel", 0.004f), 0.f);
		m_camera_longitudinal_offset_per_accel = _max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "longitudinal_offset_per_accel", 0.0045f), 0.f);
		m_camera_max_lateral_offset = _max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "max_lateral_offset", 0.05f), 0.f);
		m_camera_max_longitudinal_offset = _max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "max_longitudinal_offset", 0.045f), 0.f);
		m_camera_pitch_per_accel = deg2rad(_max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "pitch_per_accel", 0.18f), 0.f));
		m_camera_max_pitch = deg2rad(_max(
			READ_IF_EXISTS(pSettings, r_float, camera_profile, "max_pitch", 1.8f), 0.f));
	}
	ResetVehicleCameraDynamics();

	CExplosive::Load(ini, "explosion");
	//CExplosive::SetInitiator(ID());
	m_camera_position = m_dcp_white_niva_profile ?
		READ_IF_EXISTS(ini, r_fvector3, "car_definition", "camera_pos_firsteye",
			ini->r_fvector3("car_definition", "camera_pos")) :
		ini->r_fvector3("car_definition", "camera_pos");
	m_camera_position_2 = m_dcp_white_niva_profile ?
		READ_IF_EXISTS(ini, r_fvector3, "car_definition", "camera_pos_lookat", m_camera_position) :
		READ_IF_EXISTS(ini, r_fvector3, "car_definition", "camera_pos2", m_camera_position);
	m_camera_position_3 = m_dcp_white_niva_profile ?
		READ_IF_EXISTS(ini, r_fvector3, "car_definition", "camera_pos_free", m_camera_position_2) :
		m_camera_position_2;
	m_camera_current_position = &m_camera_position;
	///////////////////////////car definition///////////////////////////////////////////////////
	fill_wheel_vector(ini->r_string("car_definition", "driving_wheels"), m_driving_wheels);
	fill_wheel_vector(ini->r_string("car_definition", "steering_wheels"), m_steering_wheels);
	fill_wheel_vector(ini->r_string("car_definition", "breaking_wheels"), m_breaking_wheels);
	fill_exhaust_vector(ini->r_string("car_definition", "exhausts"), m_exhausts);
	fill_doors_map(ini->r_string("car_definition", "doors"), m_doors);

	if (ini->line_exist("car_definition", "usable_bones"))
	{
		const char* Bones = ini->r_string("car_definition", "usable_bones");
		const char* Callbacks = ini->r_string("car_definition", "usable_bones_callback");

		int BonesCount = _GetItemCount(Bones);
		R_ASSERT2(BonesCount == _GetItemCount(Callbacks), "Bones list should be equal to callback list!");

		string32 BoneName = {};
		string64 CallbackName = {};
		
		for (int Iter = 0; Iter < BonesCount; Iter++)
		{
			_GetItem(Bones, Iter, BoneName);
			_GetItem(Callbacks, Iter, CallbackName);

			u16 BoneID = pKinematics->LL_BoneID(BoneName);

			UsableBones[BoneID] = Callbacks;
		}
	}

	if (ini->line_exist("car_definition", "trunk_bone"))
	{
		u16 bone_id = pKinematics->LL_BoneID(ini->r_string("car_definition", "trunk_bone"));
		CCarDoor door(this);
		door.bone_id = bone_id;
		m_doors.insert(std::make_pair(bone_id, door));

		if (!bone_map.contains(bone_id))
		{
			bone_map.insert(std::make_pair(bone_id, physicsBone()));
		}
	}

	if (m_dcp_white_niva_profile && ini->line_exist("car_definition", "trunk_aux_bones"))
	{
		fill_bone_vector(ini->r_string("car_definition", "trunk_aux_bones"), m_trunk_aux_bones);
		for (u16 bone_id : m_trunk_aux_bones)
		{
			if (!m_doors.contains(bone_id))
			{
				CCarDoor door(this);
				door.bone_id = bone_id;
				m_doors.insert(std::make_pair(bone_id, door));
			}
			if (!bone_map.contains(bone_id))
				bone_map.insert(std::make_pair(bone_id, physicsBone()));
		}
	}

	InitLinkedDoorGroups(ini);
	if (m_dcp_white_niva_profile && ini->line_exist("car_definition", "doors_use_recursive"))
		fill_bone_vector(ini->r_string("car_definition", "doors_use_recursive"), m_recursive_door_bones);

	m_door_use_aliases.clear();
	if (m_dcp_white_niva_profile)
	{
		const bool has_aliases = ini->line_exist("car_definition", "door_use_alias_bones");
		const bool has_targets = ini->line_exist("car_definition", "door_use_alias_targets");
		R_ASSERT2(has_aliases == has_targets,
			"door_use_alias_bones and door_use_alias_targets must be declared together");
		if (has_aliases)
		{
			LPCSTR aliases = ini->r_string("car_definition", "door_use_alias_bones");
			LPCSTR targets = ini->r_string("car_definition", "door_use_alias_targets");
			const int count = _GetItemCount(aliases);
			R_ASSERT2(count == _GetItemCount(targets),
				"door use alias and target lists must contain the same number of bones");

			string64 alias_name = {};
			string64 target_name = {};
			for (int index = 0; index < count; ++index)
			{
				_GetItem(aliases, index, alias_name);
				_GetItem(targets, index, target_name);
				const u16 alias_id = pKinematics->LL_BoneID(alias_name);
				const u16 target_id = pKinematics->LL_BoneID(target_name);
				R_ASSERT3(alias_id != BI_NONE, "Car model has no door use alias bone", alias_name);
				R_ASSERT3(target_id != BI_NONE && m_doors.contains(target_id),
					"Door use alias target is not a registered door", target_name);
				m_door_use_aliases[alias_id] = target_id;
			}
		}
	}

	if (m_dcp_white_niva_profile)
	{
		m_dcp_dashboard = true;
		if (ini->section_exist("tachometer") && ini->line_exist("tachometer", "tachometer"))
		{
			m_rpm_b = pKinematics->LL_BoneID("tachometer");
			R_ASSERT4(m_rpm_b != BI_NONE, "Car model has no DCP rpm bone", cNameSect().c_str(), "tachometer");
			m_dcp_tachometer_factor = READ_IF_EXISTS(ini, r_float, "tachometer", "tachometer_factor", 1.f);
			m_dcp_tachometer_min_rpm = READ_IF_EXISTS(ini, r_float, "tachometer", "tachometer_min_rpm", 0.f);
			m_dcp_tachometer_max_rpm = READ_IF_EXISTS(ini, r_float, "tachometer", "tachometer_max_rpm",
				ini->r_float("car_definition", "max_engine_rpm"));
			m_dcp_tachometer_offset = READ_IF_EXISTS(ini, r_float, "tachometer", "tachometer_offset", 0.f);
			m_dcp_tachometer_direction =
				READ_IF_EXISTS(ini, r_float, "tachometer", "tachometer_direction", -1.f) < 0.f ? -1.f : 1.f;
			pKinematics->LL_GetBoneInstance(m_rpm_b).set_callback(bctPhysics, cb_Rpm, this);
		}
		if (ini->section_exist("fuel") && ini->line_exist("fuel", "fuel"))
		{
			m_fuel_b = pKinematics->LL_BoneID("fuel");
			R_ASSERT4(m_fuel_b != BI_NONE, "Car model has no DCP fuel bone", cNameSect().c_str(), "fuel");
			m_dcp_fuel_angle = READ_IF_EXISTS(ini, r_float, "fuel", "fuel_angle", 0.f);
			m_dcp_fuel_offset = READ_IF_EXISTS(ini, r_float, "fuel", "fuel_offset", 0.f);
			pKinematics->LL_GetBoneInstance(m_fuel_b).set_callback(bctPhysics, cb_Fuel, this);
		}
		if (ini->section_exist("speedometer") && ini->line_exist("speedometer", "speedometer"))
		{
			m_speed_b = pKinematics->LL_BoneID("speedometer");
			R_ASSERT4(m_speed_b != BI_NONE, "Car model has no DCP speed bone", cNameSect().c_str(), "speedometer");
			m_dcp_speedometer_max = READ_IF_EXISTS(ini, r_float, "speedometer", "speedometer_max_speed", 180.f);
			m_dcp_speedometer_factor = READ_IF_EXISTS(ini, r_float, "speedometer", "speedometer_factor", 1.f);
			m_dcp_speedometer_offset = READ_IF_EXISTS(ini, r_float, "speedometer", "speedometer_offset", 0.f);
			m_dcp_speedometer_direction =
				READ_IF_EXISTS(ini, r_float, "speedometer", "speedometer_direction", -1.f) < 0.f ? -1.f : 1.f;
			// COP_ENGINE_PATCH: COP-ENG-151 - model data selects the corrected calibration.
			m_dcp_speedometer_body_profile = xr_strcmp(
				READ_IF_EXISTS(ini, r_string, "speedometer", "speedometer_profile", ""), "dcp_body_speed_v1") == 0;
			m_dcp_speedometer_min = m_dcp_speedometer_body_profile ? clampr(
				READ_IF_EXISTS(ini, r_float, "speedometer", "speedometer_min_speed", 0.f),
				0.f, _max(m_dcp_speedometer_max, 0.f)) : 0.f;
			pKinematics->LL_GetBoneInstance(m_speed_b).set_callback(bctPhysics, cb_Speed, this);
		}
		if (ini->section_exist("fan") && ini->line_exist("fan", "fan"))
		{
			// COP_ENGINE_PATCH: COP-ENG-050 - DCP uses the section name as the
			// visual bone name; the empty `fan` marker merely enables animation.
			m_dcp_fan_bone = pKinematics->LL_BoneID("fan");
			R_ASSERT4(m_dcp_fan_bone != BI_NONE, "Car model has no DCP fan bone", cNameSect().c_str(), "fan");
			m_dcp_fan_speed = READ_IF_EXISTS(ini, r_float, "fan", "fan_speed", 0.f);
			m_dcp_fan_engine = !!READ_IF_EXISTS(ini, r_bool, "fan", "fan_engine", false);
			m_dcp_fan_current_speed = 0.f;
			m_dcp_fan_angle = 0.f;
			m_dcp_fan_update_frame = u32(-1);
			pKinematics->LL_GetBoneInstance(m_dcp_fan_bone).set_callback(bctPhysics, cb_DcpFan, this);
		}
	}

	else if (ini->section_exist("dashboard"))
	{
		if (ini->line_exist("dashboard", "rpm_bone")) 
		{
			auto bonename = ini->r_string("dashboard", "rpm_bone");
			m_rpm_b = pKinematics->LL_BoneID(bonename);

			R_ASSERT4(m_rpm_b != BI_NONE, "Car model has no rpm bone", cNameSect().c_str(), bonename);

			auto temp = m_rpm_offsets = ini->r_fvector4("dashboard", "rpm_angle");
			m_rpm_offsets.z = std::min(temp.z, temp.w);
			m_rpm_offsets.w = std::max(temp.z, temp.w);

			pKinematics->LL_GetBoneInstance(m_rpm_b).set_callback(bctPhysics, cb_Rpm, this);
		}
		if (ini->line_exist("dashboard", "fuel_bone")) 
		{
			auto bonename = ini->r_string("dashboard", "fuel_bone");
			m_fuel_b = pKinematics->LL_BoneID(bonename);

			R_ASSERT4(m_fuel_b != BI_NONE, "Car model has no fuel bone", cNameSect().c_str(), bonename);

			auto temp = m_fuel_offsets = ini->r_fvector4("dashboard", "fuel_angle");
			m_fuel_offsets.z = std::min(temp.z, temp.w);
			m_fuel_offsets.w = std::max(temp.z, temp.w);

			pKinematics->LL_GetBoneInstance(m_fuel_b).set_callback(bctPhysics, cb_Fuel, this);
		}
		if (ini->line_exist("dashboard", "speed_bone")) 
		{
			auto bonename = ini->r_string("dashboard", "speed_bone");
			m_speed_b = pKinematics->LL_BoneID(bonename);

			R_ASSERT4(m_speed_b != BI_NONE, "Car model has no speed bone", cNameSect().c_str(), bonename);

			auto temp = m_speed_offsets = ini->r_fvector4("dashboard", "speed_angle");
			m_speed_offsets.z = std::min(temp.z, temp.w);
			m_speed_offsets.w = std::max(temp.z, temp.w);

			pKinematics->LL_GetBoneInstance(m_speed_b).set_callback(bctPhysics, cb_Speed, this);
		}
	}

	///////////////////////////car properties///////////////////////////////
	m_max_power = ini->r_float("car_definition", "engine_power");
	m_max_power *= (0.8f * 1000.f);

	m_max_rpm = ini->r_float("car_definition", "max_engine_rpm");
	m_max_rpm *= (1.f / 60.f * 2.f * M_PI);


	m_min_rpm = ini->r_float("car_definition", "idling_engine_rpm");
	m_min_rpm *= (1.f / 60.f * 2.f * M_PI);

	m_power_rpm = ini->r_float("car_definition", "max_power_rpm");
	m_power_rpm *= (1.f / 60.f * 2.f * M_PI);//

	m_torque_rpm = ini->r_float("car_definition", "max_torque_rpm");
	m_torque_rpm *= (1.f / 60.f * 2.f * M_PI);//

	m_power_increment_factor = READ_IF_EXISTS(ini, r_float, "car_definition", "power_increment_factor", m_power_increment_factor);
	m_rpm_increment_factor = READ_IF_EXISTS(ini, r_float, "car_definition", "rpm_increment_factor", m_rpm_increment_factor);
	m_power_decrement_factor = READ_IF_EXISTS(ini, r_float, "car_definition", "power_decrement_factor", m_power_increment_factor);
	m_rpm_decrement_factor = READ_IF_EXISTS(ini, r_float, "car_definition", "rpm_decrement_factor", m_rpm_increment_factor);
	m_power_neutral_factor = READ_IF_EXISTS(ini, r_float, "car_definition", "power_neutral_factor", m_power_neutral_factor);
	R_ASSERT2(m_power_neutral_factor > 0.1f && m_power_neutral_factor < 1.f, "power_neutral_factor must be 0 - 1 !!");
	if (ini->line_exist("car_definition", "exhaust_particles"))
	{
		m_exhaust_particles = ini->r_string("car_definition", "exhaust_particles");
	}

	b_auto_switch_transmission = !!ini->r_bool("car_definition", "auto_transmission");

	InitParabola();

	m_axle_friction = ini->r_float("car_definition", "axle_friction");
	m_steering_speed = ini->r_float("car_definition", "steering_speed");

	if (ini->line_exist("car_definition", "break_time"))
	{
		m_break_time = ini->r_float("car_definition", "break_time");
	}
	// transmission
	float main_gear_ratio = ini->r_float("car_definition", "main_gear_ratio");

	R_ASSERT2(ini->section_exist("transmission_gear_ratio"), "no section transmission_gear_ratio");
	m_gear_ratious.push_back(ini->r_fvector3("transmission_gear_ratio", "R"));
	m_dcp_gear_min_speed.push_back(0.f);
	m_gear_ratious[0][0] = -m_gear_ratious[0][0] * main_gear_ratio;
	string32 rat_num;
	for (int i = 1; true; ++i)
	{
		xr_sprintf(rat_num, "N%d", i);
		if (!ini->line_exist("transmission_gear_ratio", rat_num)) break;
		Fvector gear_rat = ini->r_fvector3("transmission_gear_ratio", rat_num);
		gear_rat[0] *= main_gear_ratio;
		gear_rat[1] *= (1.f / 60.f * 2.f * M_PI);
		gear_rat[2] *= (1.f / 60.f * 2.f * M_PI);
		m_gear_ratious.push_back(gear_rat);

		float min_shift_speed = 0.f;
		if (m_dcp_white_niva_profile)
		{
			LPCSTR raw_ratio = ini->r_string("transmission_gear_ratio", rat_num);
			if (_GetItemCount(raw_ratio) > 3)
			{
				string32 speed_item;
				_GetItem(raw_ratio, 3, speed_item);
				min_shift_speed = _max(float(atof(speed_item)), 0.f);
			}
		}
		m_dcp_gear_min_speed.push_back(min_shift_speed);
	}

	// sound
	m_car_sound->Init();

	// fuel
	m_fuel_tank = ini->r_float("car_definition", "fuel_tank");
	m_fuel = m_fuel_tank;

	m_fuel_consumption = ini->r_float("car_definition", "fuel_consumption");
	m_fuel_consumption /= 100000.f;
	if (m_fuel_system_enabled)
	{
		m_fuel_consumption *= m_fuel_consumption_scale;
		// COP_ENGINE_PATCH: COP-ENG-065 - the drivetrain stores angular speed
		// in radians per second. Convert it to completed revolutions directly;
		// treating this value as RPM made profiled fuel consumption 60/(2*pi)
		// times too small.
		m_fuel_consumption /= (2.f * M_PI);
	}
	else if (m_dcp_white_niva_profile)
		m_fuel_consumption = 0.f;
	if (ini->line_exist("car_definition", "exhaust_particles"))
		m_exhaust_particles = ini->r_string("car_definition", "exhaust_particles");

	// lights
	m_lights.Init(this);
	m_lights.ParseDefinitions();

	if (ini->section_exist("animations"))
	{
		m_driver_anim_type = ini->r_u16("animations", "driver_animation_type");
	}
	if (m_dcp_white_niva_profile)
		m_driver_anim_type = 0;


	if (ini->section_exist("doors"))
	{
		m_doors_torque_factor = ini->r_u16("doors", "open_torque_factor");
	}

	m_damage_particles.Init(this);
}

void CCar::CreateSkeleton(CSE_Abstract* po)
{
	if (!Visual()) return;
	IRenderVisual *pVis = Visual();
	IKinematics* pK = smart_cast<IKinematics*>(pVis);
	IKinematicsAnimated* pKA = smart_cast<IKinematicsAnimated*>(pVis);
	if(pKA)
	{
		pKA->PlayCycle		("idle");
		pK->CalculateBones	(TRUE);
	}

	m_pPhysicsShell		= P_build_Shell(this, false, &bone_map);
	m_pPhysicsShell->SetPrefereExactIntegration();
	m_pPhysicsShell->Activate(true);
	m_pPhysicsShell->applyForce({ 0.f, 0.f, 0.f }, 0.00001f);
	ApplySpawnIniToPhysicShell(&po->spawn_ini(),m_pPhysicsShell,false);
	ApplySpawnIniToPhysicShell(pK->LL_UserData(),m_pPhysicsShell,false);
	pK->CalculateBones(TRUE);
}

void CCar::Init()
{
	CPHCollisionDamageReceiver::Init();
	if (m_dcp_white_niva_profile)
		SetCollisionHitThreshold(m_collision_hit_threshold);

	//get reference wheel radius
	IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
	CInifile* ini = pKinematics->LL_UserData();

	if (ini == nullptr && Device.IsEditorMode())
		return;

	R_ASSERT2(ini, "Car has no description !!! See ActorEditor Object - UserData");

	if (ini->section_exist("air_resistance"))
	{
		PPhysicsShell()->SetAirResistance(default_k_l * ini->r_float("air_resistance", "linear_factor"), default_k_w * ini->r_float("air_resistance", "angular_factor"));
	}

	if (ini->line_exist("car_definition", "steer"))
	{
		m_bone_steer = pKinematics->LL_BoneID(ini->r_string("car_definition", "steer"));
		VERIFY2(fsimilar(DET(pKinematics->LL_GetTransform(m_bone_steer)), 1.f, EPS_L), "BBADD MTX");
		pKinematics->LL_GetBoneInstance(m_bone_steer).set_callback(bctPhysics, cb_Steer, this);
	}

	m_steer_angle = 0.f;
	m_ref_radius = ini->r_float("car_definition", "reference_radius");//ref_wheel.radius;
	b_exploded = false;
	b_engine_on = false;
	b_clutch = false;
	b_starting = false;
	m_dcp_engine_start_time = 0;
	m_dcp_drive_pending = false;
	b_transmission_switching = false;
	m_transmission_previous_rpm = 0.f;
	m_transmission_switch_time = 0;
	m_root_transform.set(bone_map.find(pKinematics->LL_GetBoneRoot())->second.element->mXFORM);
	m_current_transmission_num = 0;
	m_pPhysicsShell->set_DynamicScales(1.f, 1.f);

	CDamagableItem::Init(GetfHealth(), 3);
	float l_time_to_explosion = READ_IF_EXISTS(ini, r_float, "car_definition", "time_to_explosion", 120.f);
	const float fuse_health = m_condition_smoke_health >= 0.f ?
		CDamagableItem::MaxHealth() * m_condition_smoke_health : CDamagableItem::DamageLevelToHealth(2);
	CDelayedActionFuse::Initialize(l_time_to_explosion, fuse_health);

	for (auto& [ID, WhellData] : m_wheels_map)
	{
		WhellData.Init();
		WhellData.CDamagableHealthItem::Init(100.f, 2);
	}
	for (SWheelDrive& Wheel : m_driving_wheels)
		Wheel.Init();

	for (SWheelBreak& Wheel : m_breaking_wheels)
		Wheel.Init();

	for (SWheelSteer& WheelSteer : m_steering_wheels)
		WheelSteer.Init();

	for (SExhaust& Exhaust : m_exhausts)
		Exhaust.Init();

	for (auto& [ID, CarObj] : m_doors)
	{
		CarObj.Init();
		CarObj.CDamagableHealthItem::Init(100, 1);
	}

	InitDcpSuspensionVisuals(ini);
	InitDcpGaugeVisuals(ini);

	if (ini->section_exist("damage_items"))
	{
		CInifile::Sect& data = ini->r_section("damage_items");
		for (CInifile::SectCIt I = data.Data.begin(); I != data.Data.end(); I++)
		{
			const CInifile::Item& item = *I;
			u16 index = pKinematics->LL_BoneID(*item.first);
			R_ASSERT3(index != BI_NONE, "Wrong bone name", *item.first);
			xr_map <u16, SWheel>::iterator i = m_wheels_map.find(index);

			if (i != m_wheels_map.end())
			{
				i->second.CDamagableHealthItem::Init(float(atof(*item.second)), 2);
			}
			else
			{
				xr_map   <u16, CCarDoor>::iterator i_ = m_doors.find(index);
				R_ASSERT3(i_ != m_doors.end(), "only wheel and doors bones allowed for damage defs", *item.first);
				i_->second.CDamagableHealthItem::Init(float(atof(*item.second)), 1);
			}
		}
	}

	if (ini->section_exist("immunities"))
	{
		LoadImmunities("immunities", ini);
	}

	CDamageManager::reload("car_definition", "damage", ini);

	HandBreak();
	Transmission(1);
}

void CCar::NeutralDrive()
{
	for (SWheelDrive& Wheel : m_driving_wheels)
		Wheel.Neutral();

	m_dcp_drive_pending = false;
	e_state_drive = neutral;
}

void CCar::ReleaseHandBreak()
{
	for (SWheelBreak& Wheel : m_breaking_wheels)
		Wheel.Neutral();

	if (e_state_drive == drive)
		Drive();
}

void CCar::Drive()
{
	if (!b_clutch)
		return;

	// COP_ENGINE_PATCH: COP-ENG-078
	// COP_ENGINE_PATCH: COP-ENG-080 - queue held throttle while the engine is
	// off or within the donor's short startup transition. The old gate returned
	// before recording the drive state and waited for the entire OGG, which both
	// made long-starter vehicles feel unresponsive and required W to be pressed
	// again. After the shared delay, PhDataUpdate retries Drive automatically.
	if (m_dcp_white_niva_profile && (!b_engine_on ||
		(m_car_sound && m_car_sound->eCarSound == SCarSound::sndStarting &&
			Device.dwTimeGlobal - m_dcp_engine_start_time <
				u32(m_dcp_starter_drive_delay * 1000.f))))
	{
		m_dcp_drive_pending = true;
		e_state_drive = drive;
		return;
	}
	if (!b_engine_on)
		return;

	m_dcp_drive_pending = false;
	m_pPhysicsShell->Enable();
	m_current_rpm = EngineDriveSpeed();
	m_current_engine_power = EnginePower();

	for (SWheelDrive& Wheel : m_driving_wheels)
		Wheel.Drive();

	e_state_drive = drive;

}

void CCar::StartEngine()
{
	if (m_fuel < EPS || b_engine_on)
		return;

	PlayExhausts();
	m_car_sound->Start();
	b_engine_on = true;
	m_current_rpm = 0.f;
	b_starting = true;
	if (m_dcp_white_niva_profile)
	{
		m_dcp_engine_start_time = Device.dwTimeGlobal;
		if (fwp || bkp || e_state_drive == drive)
		{
			m_dcp_drive_pending = true;
			e_state_drive = drive;
		}
	}
}

void CCar::StopEngine()
{
	if (!b_engine_on)
		return;

	AscCall(ascSndStall);
	AscCall(ascExhoustStop);

	NeutralDrive();//set zero speed
	b_engine_on = false;

	UpdatePower();//set engine friction;
	m_current_rpm = 0.f;
}

void CCar::Stall()
{
	AscCall(ascSndStall);
	AscCall(ascExhoustStop);

	NeutralDrive();//set zero speed
	b_engine_on = false;

	UpdatePower();//set engine friction;
	m_current_rpm = 0.f;

}
void CCar::ReleasePedals()
{
	Clutch();
	NeutralDrive();//set zero speed
	UpdatePower();//set engine friction;
}

void CCar::SwitchEngine()
{
	if (b_engine_on) StopEngine();
	else			StartEngine();
}

void CCar::Clutch()
{
	b_clutch = true;
}

void CCar::Unclutch()
{
	b_clutch = false;
}

void CCar::Starter()
{
	b_starting = true;
	m_dwStartTime = Device.dwTimeGlobal;
}

void CCar::UpdatePower()
{
	// COP_ENGINE_PATCH: COP-ENG-049 - the DCP profile uses the donor's
	// wheel-coupled RPM and automatic-shift state machine. There is no synthetic
	// clutch RPM, settling timer or per-gear threshold workaround on this path.
	m_current_rpm = EngineDriveSpeed();
	m_current_engine_power = EnginePower();
	if (b_auto_switch_transmission && (m_dcp_white_niva_profile || !b_transmission_switching))
	{
		VERIFY2(CurrentTransmission() < m_gear_ratious.size(), "wrong transmission");
		if (m_current_rpm < m_gear_ratious[CurrentTransmission()][1]) TransmissionDown();

		bool can_shift_up = true;
		if (m_dcp_white_niva_profile && CurrentTransmission() < m_dcp_gear_min_speed.size())
		{
			const float minimum_speed = m_dcp_gear_min_speed[CurrentTransmission()];
			if (minimum_speed > 0.f && m_pPhysicsShell)
			{
				Fvector velocity;
				m_pPhysicsShell->get_LinearVel(velocity);
				can_shift_up = velocity.magnitude() * 3.6f >= minimum_speed;
			}
		}
		if (can_shift_up && m_current_rpm > m_gear_ratious[CurrentTransmission()][2]) TransmissionUp();
	}
	for (SWheelDrive& Wheel : m_driving_wheels)
		Wheel.UpdatePower();
}

float CCar::DcpSteeringVelocity(float current_angle, float target_direction) const
{
	// COP_ENGINE_PATCH: COP-ENG-039
	// COP_ENGINE_PATCH: COP-ENG-043 - reproduce veh_niva_u_02's actual donor
	// routine: the wheel starts at five percent response, follows a sine ramp
	// over steering_speed seconds, and snaps to the completed ramp with a 1.5x
	// velocity only while counter-steering. The advanced LTX steering fields are
	// loaded for binary compatibility, but this donor path does not consume them.
	if (!m_dcp_white_niva_profile || !m_steer_nonlinear)
		return m_steering_speed;

	const float duration = _max(m_steering_speed, EPS);
	float turn_time = clampr((Device.fTimeGlobal - m_steering_start_time) / duration, 0.f, 1.f);
	float direction_factor = 1.f;
	if (!fis_zero(current_angle) && !fis_zero(target_direction) && current_angle * target_direction < 0.f)
	{
		turn_time = 1.f;
		direction_factor = 1.5f;
	}
	const float response = 0.05f + 1.15f * sinf(_sqrt(turn_time) * (M_PI * 0.5f));

	return response * direction_factor * m_steering_speed;
}

void CCar::SteerRight()
{
	b_wheels_limited = true;  //no need to limit wheels when stiring
	m_pPhysicsShell->Enable();
	if (m_dcp_white_niva_profile && e_state_steer != right)
		m_steering_start_time = Device.fTimeGlobal;

	for (SWheelSteer& Wheel : m_steering_wheels)
	{
		if (m_dcp_white_niva_profile && m_steer_invert)
			Wheel.SteerLeft();
		else
			Wheel.SteerRight();
	}

	e_state_steer = right;

}
void CCar::SteerLeft()
{
	b_wheels_limited = true; //no need to limit wheels when stiring
	m_pPhysicsShell->Enable();
	if (m_dcp_white_niva_profile && e_state_steer != left)
		m_steering_start_time = Device.fTimeGlobal;

	for (SWheelSteer& Wheel : m_steering_wheels)
	{
		if (m_dcp_white_niva_profile && m_steer_invert)
			Wheel.SteerRight();
		else
			Wheel.SteerLeft();
	}

	e_state_steer = left;
}

void CCar::SteerIdle()
{
	b_wheels_limited = false;
	m_pPhysicsShell->Enable();

	for (SWheelSteer& Wheel : m_steering_wheels)
		Wheel.SteerIdle();

	e_state_steer = idle;
}

void CCar::LimitWheels()
{
	if (b_wheels_limited)
		return;

	b_wheels_limited = true;

	for (SWheelSteer& Wheel : m_steering_wheels)
		Wheel.Limit();
}

void CCar::HandBreak()
{
	for (SWheelBreak& Wheel : m_breaking_wheels)
		Wheel.HandBreak();
}

void CCar::StartBreaking()
{
	if (!b_breaks)
	{
		b_breaks = true;
		m_break_start = Device.fTimeGlobal;
	}
}

void CCar::StopBreaking()
{
	for (SWheelBreak& Wheel : m_breaking_wheels)
		Wheel.Neutral();

	if (e_state_drive == drive)
		Drive();

	b_breaks = false;
}

void CCar::PressRight()
{
	if (lsp)
	{
		if (!fwp)SteerIdle();
	}
	else
		SteerRight();

	rsp = true;
}

void CCar::PressLeft()
{
	if (rsp)
	{
		if (!fwp)SteerIdle();
	}
	else
		SteerLeft();

	lsp = true;
}

void CCar::PressForward()
{
	if (bkp)
	{
		Unclutch();
		NeutralDrive();
	}
	else
	{
		DriveForward();
	}
	fwp = true;
}

void CCar::PressBack()
{
	if (fwp)
	{
		Unclutch();
		NeutralDrive();
	}
	else
	{
		Unclutch();
		NeutralDrive();
		StartBreaking();
	}
	bkp = true;
}

void CCar::PressBreaks()
{
	HandBreak();
	brp = true;
}

void CCar::DriveBack()
{
	Clutch();
	Transmission(0);

	if (0 == CurrentTransmission())
		Starter();

	Drive();
}

void CCar::DriveForward()
{
	Clutch();
	if (0 == CurrentTransmission()) 
		Transmission(1);

	if (1 == CurrentTransmission())
		Starter();

	Drive();
}

void CCar::ReleaseRight()
{
	if (lsp)
		SteerLeft();
	else
		SteerIdle();

	rsp = false;
}

void CCar::ReleaseLeft()
{
	if (rsp)
		SteerRight();
	else
		SteerIdle();

	lsp = false;
}

void CCar::ReleaseForward()
{
	if (bkp)
	{
		Clutch();
		Transmission(0);
		Starter();

		Drive();
	}
	else
	{
		Unclutch();
		NeutralDrive();
	}

	fwp = false;
}

void CCar::ReleaseBack()
{
	if (b_breaks)
	{
		StopBreaking();
	}
	if (fwp)
	{
		Clutch();

		if (0 == CurrentTransmission())
			Transmission(1);

		if (1 == CurrentTransmission()) 
			Starter();

		Drive();
	}
	else
	{
		Unclutch();
		NeutralDrive();
	}
	bkp = false;
}

void CCar::ReleaseBreaks()
{
	ReleaseHandBreak();
	brp = false;
}

void CCar::Transmission(size_t num)
{
	if (num < m_gear_ratious.size())
	{
		if (CurrentTransmission() != num)
		{
			if (m_dcp_white_niva_profile)
			{
				// COP_ENGINE_PATCH: COP-ENG-049 - preserve the pre-shift RPM for
				// DCP's audible half-sine transition. Physical RPM follows the new
				// ratio on the next update and is never replaced by this value.
				m_transmission_previous_rpm = m_current_rpm;
				m_transmission_switch_time = Device.dwTimeGlobal;
			}
			AscCall(ascSndTransmission);
			m_current_transmission_num = num;
			m_current_gear_ratio = m_gear_ratious[num][0];
			b_transmission_switching = !m_dcp_white_niva_profile;
			if (!m_dcp_white_niva_profile)
				m_transmission_switch_time = Device.dwTimeGlobal;
			Drive();
		}
	}
}

void CCar::TransmissionUp()
{
	if (0 == CurrentTransmission())
		return;

	size_t transmission = 1 + CurrentTransmission();
	size_t max_transmition_num = m_gear_ratious.size() - 1;
	transmission > max_transmition_num ? transmission = max_transmition_num : transmission;
	Transmission(transmission);

}

void CCar::TransmissionDown()
{
	if (0 == CurrentTransmission())
		return;

	size_t transmission = CurrentTransmission() - 1;
	transmission < 1 ? transmission = 1 : transmission;
	Transmission(transmission);
}

void CCar::PhTune(float step)
{
	for (u16 i = PPhysicsShell()->get_ElementsNumber(); i != 0; i--)
	{
		CPhysicsElement* e = PPhysicsShell()->get_ElementByStoreOrder(i - 1);
		if (e->isActive() && e->isEnabled())
			e->applyForce(0, e->getMass() * AntiGravityAccel(), 0);
	}
}

float CCar::EffectiveGravity()
{
	float g = physics_world()->Gravity();
	if (CPHUpdateObject::IsActive())g *= 0.5f;
	return g;
}

float CCar::AntiGravityAccel()
{
	return physics_world()->Gravity() - EffectiveGravity();
}

float CCar::GravityFactorImpulse()
{
	return _sqrt(EffectiveGravity() / physics_world()->Gravity());
}

void CCar::UpdateBack()
{
	if (b_breaks)
	{
		float k = 1.f;
		float time = (Device.fTimeGlobal - m_break_start);
		if (time < m_break_time)
		{
			k *= (time / m_break_time);
		}

		for (SWheelBreak& Wheel : m_breaking_wheels)
			Wheel.Break(k);

		Fvector v;
		m_pPhysicsShell->get_LinearVel(v);

		if (v.dotproduct(XFORM().k) < EPS)
		{
			StopBreaking();
			DriveBack();
		}
	}
}

void CCar::PlayExhausts()
{
	for (SExhaust& Exhaust : m_exhausts)
		Exhaust.Play();
}

void CCar::StopExhausts()
{
	for (SExhaust& Exhaust : m_exhausts)
		Exhaust.Stop();
}

void CCar::UpdateExhausts()
{
	if (!b_engine_on)
		return;

	for (SExhaust& Exhaust : m_exhausts)
		Exhaust.Update();
}

void CCar::ClearExhausts()
{
	for (SExhaust& Exhaust : m_exhausts)
		Exhaust.Clear();
}

void CCar::UpdateConditionDamageParticles()
{
	if (m_condition_smoke_health < 0.f || CDamagableItem::MaxHealth() <= EPS)
		return;

	const float health_ratio = clampr(GetfHealth() / CDamagableItem::MaxHealth(), 0.f, 1.f);
	const bool should_smoke = g_Alive() && health_ratio <= m_condition_smoke_health;
	if (should_smoke == m_condition_damage_particles_active)
		return;

	if (should_smoke)
		m_damage_particles.Play2(this);
	else
		m_damage_particles.Stop2(this);
	m_condition_damage_particles_active = should_smoke;
}

void CCar::DoExit()
{
	if (CActor* A = OwnerActor())
	{
		if (!m_doors.empty())m_doors.begin()->second.GetExitPosition(m_exit_position);
		else m_exit_position.set(Position());

		A->detach_Vehicle();

		if (A->g_Alive() <= 0.f)
		{
			A->character_physics_support()->movement()->DestroyCharacter();
		}
	}
}

bool CCar::Use(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos)
{
	if (UsableBonesActive != BI_NONE)
	{
		luabind::functor<bool> Callback;
		ai().script_engine().functor(UsableBones[UsableBonesActive].c_str(), Callback);
		bool CheckComplete = Callback(this);

		if (CheckComplete)
		{
			// FX: Если скрипт вернул true, то будем считать, 
			// что Use был выполнен и посадка в машину не требуется 

			return false;
		}
	}

	// COP_ENGINE_PATCH: COP-ENG-051 - the prompt and F use the exact same
	// bone-attached interaction volume. This prevents a visible prompt from
	// dispatching the legacy enter-car path and avoids duplicate trunk sounds.
	if (UsesProfileTrunkInteraction() && TryProfileTrunk(pos, dir))
	{
		// COP_ENGINE_PATCH: COP-ENG-073 - compound donor trunks open every
		// registered auxiliary panel with the primary inventory hatch.
		for (u16 bone_id : m_trunk_aux_bones)
		{
			auto auxiliary = m_doors.find(bone_id);
			if (auxiliary != m_doors.end())
				auxiliary->second.Open();
		}
		auto trunk = m_doors.find(m_bone_trunk);
		if (trunk != m_doors.end())
			trunk->second.Open();
		CurrentGameUI()->StartCarBody(Actor(), this);
		return false;
	}

	// COP_ENGINE_PATCH: COP-ENG-060 - explicit panel use has priority in every
	// DCP context. Outside, aiming at an entry door or the hood operates that
	// panel; entering is only the no-panel fallback. Inside, the same rule lets
	// the actor operate a door before the no-panel fallback exits the vehicle.
	// The profile trunk remains on its dedicated interaction path above.

	xr_map<u16, CCarDoor>::iterator i;

	RQR.r_clear();
	float use_query_distance = m_dcp_white_niva_profile ? m_trunk_use_distance : 2.f;
	if (m_dcp_white_niva_profile)
	{
		for (const auto& [bone, door] : m_doors)
			use_query_distance = _max(use_query_distance, door.use_distance);
	}
	collide::ray_defs	Q(pos, dir, use_query_distance, CDB::OPT_CULL, collide::rqtObject);

	VERIFY(!fis_zero(Q.dir.square_magnitude()));
	if (g_pGameLevel->ObjectSpace.RayQuery(RQR, collidable.model, Q))
	{
		collide::rq_results& R = RQR;
		for (int k = 0; k < R.r_count(); ++k)
		{
			collide::rq_result* I = R.r_begin() + k;

			if (IsGameTypeSingleCompatible() && m_bone_trunk == (u16)I->element)
			{
				bool IsDoorBone = is_Door((u16)I->element, i);
				if (m_dcp_white_niva_profile && m_trunk_open_inventory)
				{
					// The profile path was handled above by the shared volume. A direct
					// collision hit outside that volume must not fall through to entry.
					return false;
				}
				if (I->range < 1.f)
				{
					if (IsDoorBone)
					{
						CCarDoor& TrunkDoor = m_doors.at(m_bone_trunk);
						if (TrunkDoor.state != CCarDoor::eState::opened)
							continue;
					}

					CurrentGameUI()->StartCarBody(Actor(), this);
				}
				else if (IsDoorBone)
				{
					i->second.Use();
				}

				return false;
			}

			if (is_Door((u16)I->element, i))
			{
				// COP_ENGINE_PATCH: COP-ENG-050 - DCP panels keep their own use
				// radii (2.3 m doors, 2.5 m hood, 3.5 m trunk).
				if (m_dcp_white_niva_profile && I->range > i->second.use_distance)
					continue;
				bool front = i->second.IsFront(pos, dir);
				if (m_dcp_white_niva_profile)
				{
					// Donor DCP only operates the panel from its reachable side.
					if ((Owner() && !front) || (!Owner() && front))
						i->second.Use();
				}
				else if ((Owner()) || (!Owner() && front))
				{
					i->second.Use();
				}
				if (i->second.state == CCarDoor::broken) break;
				return false;
			}
		}
	}
	if (m_dcp_white_niva_profile)
	{
		if (Owner())
			return Exit(pos, dir);
		return Enter(pos, dir, foot_pos);
	}

	if (Owner())
	{
		return Exit(pos, dir);
	}

	return Enter(pos, dir, foot_pos);
}

void CCar::cb_DcpSuspension(CBoneInstance* bone)
{
	SDcpSuspensionVisual* visual = static_cast<SDcpSuspensionVisual*>(bone->callback_param());
	CCar* car = visual->car;
	IKinematics* kinematics = PKinematics(car->Visual());
	float vertical_offset = 0.f;
	float steer_angle = 0.f;
	u32 wheel_count = 0;

	auto sample_wheel = [&](u16 wheel_id)
	{
		if (wheel_id == BI_NONE || !car->m_wheels_map.contains(wheel_id))
			return;
		const Fmatrix& current = kinematics->LL_GetBoneInstance(wheel_id).mTransform;
		const Fmatrix& bind = kinematics->LL_GetData(wheel_id).bind_transform;
		vertical_offset += current.c.y - bind.c.y;
		for (SWheelSteer& steering : car->m_steering_wheels)
		{
			if (steering.pwheel->bone_id == wheel_id)
			{
				steer_angle += steering.GetSteerAngle();
				break;
			}
		}
		++wheel_count;
	};

	sample_wheel(visual->first_wheel);
	sample_wheel(visual->second_wheel);
	if (wheel_count == 0)
		return;

	vertical_offset = vertical_offset / float(wheel_count) + visual->offset;
	steer_angle /= float(wheel_count);
	Fmatrix transform;
	transform.identity();
	if (visual->steer)
		transform.rotateY(steer_angle);
	if (visual->move)
	{
		const SWheel& wheel = car->m_wheels_map.find(visual->first_wheel)->second;
		transform.c.y = clampr(vertical_offset, -wheel.suspension_distance, wheel.suspension_distance);
	}
	bone->mTransform.mulB_43(transform);
}

void CCar::InitDcpSuspensionVisuals(CInifile* ini)
{
	if (!m_dcp_white_niva_profile)
		return;

	IKinematics* kinematics = PKinematics(Visual());
	for (const CInifile::Sect* section : ini->sections())
	{
		LPCSTR section_name = section->Name.c_str();
		if (!ini->line_exist(section_name, "suspension") || !ini->line_exist(section_name, "first_wheel"))
			continue;

		const u16 bone_id = kinematics->LL_BoneID(section_name);
		const u16 first_wheel = kinematics->LL_BoneID(ini->r_string(section_name, "first_wheel"));
		const u16 second_wheel = ini->line_exist(section_name, "second_wheel") ?
			kinematics->LL_BoneID(ini->r_string(section_name, "second_wheel")) : BI_NONE;
		if (bone_id == BI_NONE || first_wheel == BI_NONE || !m_wheels_map.contains(first_wheel))
			continue;

		SDcpSuspensionVisual* visual = new SDcpSuspensionVisual();
		visual->car = this;
		visual->bone_id = bone_id;
		visual->first_wheel = first_wheel;
		visual->second_wheel = m_wheels_map.contains(second_wheel) ? second_wheel : BI_NONE;
		visual->move = !!READ_IF_EXISTS(ini, r_bool, section_name, "move", false);
		visual->steer = !!READ_IF_EXISTS(ini, r_bool, section_name, "steer", false);
		visual->offset = READ_IF_EXISTS(ini, r_float, section_name, "suspension_offset", 0.f);
		m_dcp_suspension_visuals.push_back(visual);
		// COP_ENGINE_PATCH: COP-ENG-090 - this callback stores visual animation
		// data, not a CPhysicsElement. Physics hits must not cast it as one.
		kinematics->LL_GetBoneInstance(bone_id).set_callback(bctCustom, cb_DcpSuspension, visual);
	}
}

void CCar::ClearDcpSuspensionVisuals()
{
	for (SDcpSuspensionVisual*& visual : m_dcp_suspension_visuals)
		xr_delete(visual);
	m_dcp_suspension_visuals.clear();
}

// COP_ENGINE_PATCH: COP-ENG-200 - no new bones, Lua polling, thermal damage,
// electrical or pneumatic gameplay is introduced. Auxiliary needles are visual
// operating-state indicators; fuel, speed and RPM keep their existing paths.
void CCar::InitDcpGaugeVisuals(CInifile* ini)
{
	ClearDcpGaugeVisuals();
	if (!m_dcp_white_niva_profile)
		return;
	IKinematics* kinematics = PKinematics(Visual());
	m_dashboard_clock_game_base = Level().GetGameTime();
	m_dashboard_clock_real_base = Device.dwTimeGlobal;
	m_dashboard_real_clock = !!READ_IF_EXISTS(ini, r_bool, "clock_seconds", "clock_seconds_real", false);
	LPCSTR sections[] = { "pressure", "temp", "battery", "brakes", "clock_seconds", "clock_minutes", "clock_hours", "gear" };
	LPCSTR markers[] = { "pressure", "temp", "battery", "brakes_pressure", "clock_seconds", "clock_minutes", "clock_hours", "gear" };
	for (u8 kind = 0; kind < 8; ++kind)
	{
		LPCSTR section = sections[kind];
		if (!ini->line_exist(section, markers[kind]))
			continue;
		const u16 bone = kinematics->LL_BoneID(section);
		if (bone == BI_NONE)
			continue;
		SDcpGaugeVisual* gauge = new SDcpGaugeVisual{};
		gauge->car = this;
		gauge->bone_id = bone;
		gauge->kind = kind;
		gauge->last_frame = u32(-1);
		gauge->last_time = Device.dwTimeGlobal;
		string64 key;
		xr_sprintf(key, "%s_offset", section);
		gauge->offset = READ_IF_EXISTS(ini, r_float, section, key, 0.f);
		xr_sprintf(key, "%s_factor", section);
		gauge->factor = READ_IF_EXISTS(ini, r_float, section, key, 1.f);
		xr_sprintf(key, "%s_angle", section);
		gauge->angle = READ_IF_EXISTS(ini, r_float, section, key, 0.f);
		if (kind == 0)
		{
			gauge->angle = READ_IF_EXISTS(ini, r_float, section, "pressure_max_angle", 90.f);
			gauge->idle = READ_IF_EXISTS(ini, r_float, section, "pressure_idle_angle", 0.f);
		}
		if (kind == 3)
		{
			gauge->angle = READ_IF_EXISTS(ini, r_float, section, "brakes_pressure_max_angle", 160.f);
			gauge->factor = READ_IF_EXISTS(ini, r_float, section, "brakes_pressure_factor", .4f);
		}
		if (kind == 7)
		{
			gauge->manual = !!READ_IF_EXISTS(ini, r_bool, section, "gear_manual", false);
			gauge->gear_drive = READ_IF_EXISTS(ini, r_float, section, "gear_drive_angle_x", 0.f);
			gauge->gear_park = READ_IF_EXISTS(ini, r_float, section, "gear_park_angle_x", 0.f);
			gauge->gear_reverse = READ_IF_EXISTS(ini, r_float, section, "gear_reverse_angle_x", 0.f);
			gauge->gear_left = READ_IF_EXISTS(ini, r_float, section, "gear_left_angle", 0.f);
			gauge->gear_middle = READ_IF_EXISTS(ini, r_float, section, "gear_middle_angle", 0.f);
			gauge->gear_right = READ_IF_EXISTS(ini, r_float, section, "gear_right_angle", 0.f);
		}
		m_dcp_gauge_visuals.push_back(gauge);
		kinematics->LL_GetBoneInstance(bone).set_callback(bctPhysics, cb_DcpGauge, gauge);
	}
}

void CCar::ClearDcpGaugeVisuals()
{
	for (SDcpGaugeVisual* gauge : m_dcp_gauge_visuals)
		xr_delete(gauge);
	m_dcp_gauge_visuals.clear();
}

void CCar::cb_DcpGauge(CBoneInstance* bone)
{
	SDcpGaugeVisual* gauge = static_cast<SDcpGaugeVisual*>(bone->callback_param());
	CCar* car = gauge->car;
	if (gauge->last_frame != Device.dwFrame)
	{
		const float dt = clampr(float(Device.dwTimeGlobal - gauge->last_time) / 1000.f, 0.f, 60.f);
		gauge->last_time = Device.dwTimeGlobal;
		gauge->last_frame = Device.dwFrame;
		float target = gauge->offset, target_y = 0.f;
		const bool running = car->b_engine_on;
		switch (gauge->kind)
		{
		case 0:
			if (running)
			{
				const float magnitude = _min(_abs(gauge->angle), gauge->idle +
					_abs(gauge->factor) * _max(car->m_current_rpm, 0.f) / (2.f * PI));
				target += gauge->factor < 0.f ? -magnitude : magnitude;
			}
			break;
		case 1: target += running ? gauge->angle : 0.f; break;
		case 2: target += running ? gauge->angle * gauge->factor : 0.f; break;
		case 3:
			target += running ? gauge->angle * (car->b_breaks ? 1.f - clampr(gauge->factor, 0.f, 1.f) : 1.f) : 0.f;
			break;
		case 4:
		case 5:
		case 6:
		{
			const u64 time = car->m_dashboard_real_clock ? car->m_dashboard_clock_game_base +
				u32(Device.dwTimeGlobal - car->m_dashboard_clock_real_base) : Level().GetGameTime();
			const u64 period = gauge->kind == 4 ? 60000ull : gauge->kind == 5 ? 3600000ull : 43200000ull;
			target -= 360.f * float(time % period) / float(period);
			break;
		}
		case 7:
		{
			const size_t gear = car->CurrentTransmission();
			target = car->e_state_drive == neutral ? gauge->gear_park : gear == 0 ? gauge->gear_reverse : gauge->gear_drive;
			if (gauge->manual && car->e_state_drive != neutral && gear > 0)
			{
				target = gear % 2 ? gauge->gear_drive : gauge->gear_reverse;
				const size_t column = (gear - 1) / 2;
				target_y = column == 0 ? gauge->gear_left : column == 1 ? gauge->gear_middle : gauge->gear_right;
			}
			break;
		}
		}
		const float response = gauge->kind == 1 ? 1.f / 30.f : gauge->kind == 3 ? .3f : 8.f;
		const float blend = gauge->initialized ? 1.f - expf(-response * dt) : 1.f;
		gauge->current_x += (target - gauge->current_x) * blend;
		gauge->current_y += (target_y - gauge->current_y) * blend;
		if (gauge->kind >= 4 && gauge->kind <= 6)
			gauge->current_x = target; // clock wrap must never sweep backwards through 360 degrees.
		gauge->initialized = true;
	}
	Fmatrix rotation;
	if (gauge->kind == 7)
	{
		rotation.setXYZ(deg2rad(gauge->current_x), deg2rad(gauge->current_y), 0.f);
	}
	else
		rotation.rotateZ(deg2rad(gauge->current_x));
	bone->mTransform.mulB_43(rotation);
}

void CCar::CloseProfileTrunk()
{
	if (!m_dcp_white_niva_profile || !m_trunk_close_on_inventory_exit || m_bone_trunk == BI_NONE)
		return;

	auto trunk = m_doors.find(m_bone_trunk);
	if (trunk != m_doors.end())
		trunk->second.Close();

	for (u16 bone_id : m_trunk_aux_bones)
	{
		auto auxiliary = m_doors.find(bone_id);
		if (auxiliary != m_doors.end())
			auxiliary->second.Close();
	}
}

bool CCar::DoorUse(u16 id)
{
	xr_map<u16, CCarDoor>::iterator i;
	if (is_Door(id, i))
	{
		i->second.Use();
		return true;
	}

	return false;
}

// COP_ENGINE_PATCH: COP-ENG-197 - explicit data, no vehicle-name special cases.
void CCar::InitLinkedDoorGroups(CInifile* ini)
{
	m_linked_door_groups.clear();
	if (!m_dcp_white_niva_profile || !ini->line_exist("car_definition", "linked_door_groups"))
		return;
	IKinematics* kinematics = PKinematics(Visual());
	LPCSTR groups = ini->r_string("car_definition", "linked_door_groups");
	for (int index = 0; index < _GetItemCount(groups); ++index)
	{
		string128 section;
		_GetItem(groups, index, section);
		if (!ini->line_exist(section, "bones"))
			continue;
		LPCSTR definitions = ini->r_string(section, "bones");
		xr_vector<u16> members;
		bool valid = true;
		for (int item = 0; item < _GetItemCount(definitions); ++item)
		{
			string128 name;
			_GetItem(definitions, item, name);
			const u16 bone = kinematics->LL_BoneID(name);
			if (bone == BI_NONE || !m_doors.contains(bone) ||
				m_linked_door_groups.contains(bone) ||
				std::find(members.begin(), members.end(), bone) != members.end() ||
				kinematics->LL_GetData(bone).IK_data.type != jtJoint)
			{
				valid = false;
				break;
			}
			members.push_back(bone);
		}
		if (!valid || members.size() < 2)
		{
			Msg("! Invalid linked door group [%s] in vehicle [%s]", section, cNameSect().c_str());
			continue;
		}
		for (u16 bone : members)
			m_linked_door_groups.emplace(bone, members);
	}
}

bool CCar::UseLinkedDoorGroup(u16 selected)
{
	const auto group = m_linked_door_groups.find(selected);
	if (group == m_linked_door_groups.end())
		return false;
	const auto& members = group->second;
	const CCarDoor::eState state = m_doors.at(members.front()).state;
	const bool close = state == CCarDoor::opened || state == CCarDoor::opening;
	if (close)
	{
		for (auto item = members.rbegin(); item != members.rend(); ++item)
			m_doors.at(*item).Close();
	}
	else
	{
		for (u16 bone : members)
			m_doors.at(bone).Open();
	}
	return true;
}

bool CCar::DoorSwitch(u16 id)
{
	xr_map<u16, CCarDoor>::iterator i;
	if (is_Door(id, i))
	{
		i->second.Switch();
		return true;
	}

	return false;
}

bool CCar::DoorClose(u16 id)
{
	xr_map<u16, CCarDoor>::iterator i;
	if (is_Door(id, i))
	{
		i->second.Close();
		return true;
	}

	return false;
}

bool CCar::DoorOpen(u16 id)
{
	xr_map<u16, CCarDoor>::iterator i;
	if (is_Door(id, i))
	{
		i->second.Open();
		return true;
	}

	return false;
}

void CCar::InitParabola()
{
	m_a = expf((m_power_rpm - m_torque_rpm) / (2.f * m_power_rpm)) * m_max_power / m_power_rpm;
	m_b = m_torque_rpm;
	m_c = _sqrt(2.f * m_power_rpm * (m_power_rpm - m_torque_rpm));
}

float CCar::Parabola(float rpm)
{
	float ex = (rpm - m_b) / m_c;
	float value = m_a * expf(-ex * ex) * rpm;
	if (value < 0.f) return 0.f;
	if (e_state_drive == neutral) value *= m_power_neutral_factor;
	return value;
}

float CCar::EnginePower()
{
	float value; value = Parabola(m_current_rpm);
	if (b_starting)
	{
		if (m_current_rpm < m_min_rpm)
		{
			value = Parabola(m_min_rpm);
		}
		else if (Device.dwTimeGlobal - m_dwStartTime > 1000) b_starting = false;
	}

	if (value > m_current_engine_power)
		return value * m_power_increment_factor + m_current_engine_power * (1.f - m_power_increment_factor);
	else
		return value * m_power_decrement_factor + m_current_engine_power * (1.f - m_power_decrement_factor);
}

float CCar::DriveWheelsMeanAngleRate() const
{
	float drive_speed = 0.f;

	for (const SWheelDrive& Wheel : m_driving_wheels)
	{
		drive_speed += Wheel.ASpeed();
	}

	return drive_speed / m_driving_wheels.size();
}

float CCar::EngineDriveSpeed()
{
	if (m_dcp_white_niva_profile)
	{
		// COP_ENGINE_PATCH: COP-ENG-049 - exact veh_niva_u_02 donor formula:
		// abs(mean driven-wheel angular speed * selected ratio), idle floor and
		// normal asymmetric interpolation. The donor neither clamps this path to
		// max RPM nor substitutes brake-specific interpolation coefficients.
		const float calc_rpm = _max(m_min_rpm, EngineRpmFromWheels());
		if (calc_rpm > m_current_rpm)
			return (1.f - m_rpm_increment_factor) * m_current_rpm + m_rpm_increment_factor * calc_rpm;
		return (1.f - m_rpm_decrement_factor) * m_current_rpm + m_rpm_decrement_factor * calc_rpm;
	}

	float calc_rpm = 0.f;
	if (b_transmission_switching)
	{
		calc_rpm = m_max_rpm;
		if (m_current_rpm > m_power_rpm)
		{
			b_transmission_switching = false;
		}
	}
	else
	{
		calc_rpm = EngineRpmFromWheels();

		if (!b_clutch && calc_rpm < m_min_rpm)
		{
			calc_rpm = m_min_rpm;
		}
		limit_above(calc_rpm, m_max_rpm);
	}

	if (calc_rpm > m_current_rpm)
		return		(1.f - m_rpm_increment_factor) * m_current_rpm + m_rpm_increment_factor * calc_rpm;
	else
		return		(1.f - m_rpm_decrement_factor) * m_current_rpm + m_rpm_decrement_factor * calc_rpm;
}

void CCar::UpdateFuel(float time_delta)
{
	if (m_dcp_white_niva_profile && !m_fuel_system_enabled)
	{
		m_fuel = m_fuel_tank;
		return;
	}

	if (!b_engine_on)
		return;

	if (m_fuel_system_enabled)
		m_fuel -= time_delta * _max(m_current_rpm, m_min_rpm) * m_fuel_consumption;
	else if (m_current_rpm > m_min_rpm)
		m_fuel -= time_delta * (m_current_rpm - m_min_rpm) * m_fuel_consumption;
	else
		m_fuel -= time_delta * m_min_rpm * m_fuel_consumption;

	if (m_fuel < EPS)
	{
		m_fuel = 0.f;
		StopEngine();
	}
}

float CCar::AddFuel(float ammount)
{
	float free_space = m_fuel_tank - m_fuel;
	if (ammount < free_space)
	{
		m_fuel += ammount;
		return ammount;
	}
	else
	{
		m_fuel = m_fuel_tank;
		return free_space;
	}
}

void CCar::ResetKeys()
{
	bkp = false;
	fwp = false;
	lsp = false;
	rsp = false;
}

#undef   _USE_MATH_DEFINES

void CCar::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	CExplosive::OnEvent(P, type);

	//обработка сообщений, нужных для работы с багажником машины
	u16 id;
	switch (type)
	{
		case GE_TRADE_BUY:
		case GE_OWNERSHIP_TAKE:
		{
			P.r_u16(id);
			CObject* O = Level().Objects.net_Find(id);
			VERIFY(O);

			CGameObject* GO = smart_cast<CGameObject*>(O);
			CInventoryItem* pIItem = smart_cast<CInventoryItem*>(GO);
			// GE_OWNERSHIP_TAKE also restores items already parented to a car by a
			// save. Grandfather those contents even if a later config lowers the
			// capacity; interactive trunk transfers use GE_TRADE_BUY and are gated.
			if (!GO || !pIItem ||
				(type == GE_TRADE_BUY && !CanStoreInTrunk(pIItem->Weight())) ||
				!inventory().CanTakeItem(pIItem))
			{
				Msg("! CCar::OnEvent: rejected item [%u] because the vehicle trunk cannot accept it", id);
				break;
			}
			pIItem->m_ItemCurrPlace.type = eItemPlaceRuck;

			O->H_SetParent(this);
			inventory().Take(GO, true, true);
			break;
		}
		case GE_TRADE_SELL:
		case GE_OWNERSHIP_REJECT:
		{
			P.r_u16(id);
			CObject* O = Level().Objects.net_Find(id);
			VERIFY(O);

			bool just_before_destroy = !P.r_eof() && P.r_u8();
			bool dont_create_shell = (type == GE_TRADE_SELL) || just_before_destroy;

			O->SetTmpPreDestroy(just_before_destroy);
			inventory().DropItem(smart_cast<CGameObject*>(O), just_before_destroy, dont_create_shell);
			break;
		}
	}
}

void CCar::ResetScriptData(void* P)
{
	CScriptEntity::ResetScriptData(P);
}

bool CCar::CanRefuelCanister() const
{
	if (!m_fuel_system_enabled || !g_Alive() || m_fuel_tank <= EPS || m_fuel_canister_amount <= EPS)
		return false;

	return m_fuel_tank - clampr(m_fuel, 0.f, m_fuel_tank) + EPS >= m_fuel_refuel_min_free;
}

float CCar::TryRefuelCanister()
{
	if (!CanRefuelCanister())
		return 0.f;

	m_fuel = clampr(m_fuel, 0.f, m_fuel_tank);
	return AddFuel(m_fuel_canister_amount);
}

float CCar::DcpSoundEngineSpeed() const
{
	// COP_ENGINE_PATCH: COP-ENG-049 - ease audible RPM from the value captured
	// before a shift to the wheel-coupled RPM with DCP's half-sine curve. This
	// affects pitch only and never feeds back into torque or shift thresholds.
	if (!m_dcp_white_niva_profile || m_transmission_switch_time == 0 || m_transmission_switch_delay <= EPS)
		return m_current_rpm;

	const float elapsed = float(Device.dwTimeGlobal - m_transmission_switch_time) / 1000.f;
	if (elapsed >= m_transmission_switch_delay)
		return m_current_rpm;

	const float phase = clampr(elapsed / m_transmission_switch_delay, 0.f, 1.f);
	const float blend = 0.5f + 0.5f * sinf(phase * M_PI - M_PI * 0.5f);
	return m_transmission_previous_rpm + (m_current_rpm - m_transmission_previous_rpm) * blend;
}

void CCar::UpdateDcpWheelFriction(float step)
{
	// COP_ENGINE_PATCH: COP-ENG-039
	// COP_ENGINE_PATCH: COP-ENG-040 - blend the dynamic tyre model in only
	// after launch. The old zero-speed ratio immediately latched every driven
	// wheel into the low-grip spin state and could never recover.
	// COP_ENGINE_PATCH: COP-ENG-042 - longitudinal wheelspin must not by itself
	// remove lateral tyre grip. Treat the two ODE friction axes independently,
	// so normal acceleration through a bend does not turn into power oversteer.
	if (!m_dcp_white_niva_profile || !m_pPhysicsShell)
		return;

	Fvector velocity;
	m_pPhysicsShell->get_LinearVel(velocity);
	const float forward_speed = _abs(velocity.dotproduct(XFORM().k));
	const float lateral_speed = _abs(velocity.dotproduct(XFORM().i));
	float maximum_slip = 0.f;
	const float dynamic_grip_blend = clampr(forward_speed / m_vel_sliding_threshold, 0.f, 1.f);

	for (auto& wheel_item : m_wheels_map)
	{
		SWheel& wheel = wheel_item.second;
		if (!wheel.joint)
			continue;

		SWheel::SWheelCollisionParams& params = wheel.collision_params;
		wheel.joint->GetAxisDirDynamic(1, params.friction_direction);
		if (params.friction_direction.square_magnitude() > EPS_S)
			params.friction_direction.normalize();
		const float wheel_speed = _abs(wheel.joint->GetAxisAngleRate(1)) * wheel.radius;
		const float speed_base = _max(_max(wheel_speed, forward_speed), 0.25f);
		const float longitudinal_slip = _abs(wheel_speed - forward_speed) / speed_base;
		const float lateral_slip = lateral_speed / m_vel_sliding_threshold;
		const float measured_slip = _max(longitudinal_slip, lateral_slip);
		maximum_slip = _max(maximum_slip, measured_slip * dynamic_grip_blend);

		if (wheel_speed > 0.25f && wheel_speed > forward_speed)
		{
			const float spin_factor = forward_speed / wheel_speed;
			if (params.spinning)
				params.spinning = spin_factor < m_wheels_spin_factor_stop;
			else
				params.spinning = spin_factor < m_wheels_spin_factor_start;
		}
		else
		{
			params.spinning = false;
		}

		const bool longitudinal_sliding = longitudinal_slip >= m_slip_sliding_threshold;
		const bool lateral_sliding = lateral_slip >= m_slip_sliding_threshold;
		const float reduced_mu = params.spinning ? m_friction_spin_factor :
			(longitudinal_sliding ? m_friction_slip_factor : 1.f);
		const float reduced_mu2 = lateral_sliding ?
			(params.spinning ? m_friction2_spin_factor : m_friction2_slip_factor) : 1.f;
		const float target_mu = 1.f + (reduced_mu - 1.f) * dynamic_grip_blend;
		const float target_mu2 = 1.f + (reduced_mu2 - 1.f) * dynamic_grip_blend;
		const float response = clampr(step * m_drift_start_boost / m_drift_boost_time, 0.f, 1.f);
		params.dynamic_mu_factor += (target_mu - params.dynamic_mu_factor) * response;
		params.dynamic_mu2_factor += (target_mu2 - params.dynamic_mu2_factor) * response;
		params.friction2_factor = params.steering ? m_friction2_steering_wheels :
			m_friction2_nonsteering_wheels;
		if (brp && params.handbrake_wheel)
			params.friction2_factor *= m_friction2_brp_boost;
		params.slip_speed = m_friction_slip_speed * m_contact_slip_scale * dynamic_grip_blend;
	}

	const float slip_response = clampr(step * m_steer_turn_delta, 0.f, 1.f);
	m_dcp_current_slip += (maximum_slip - m_dcp_current_slip) * slip_response;
}

void CCar::UpdateDcpBodyRollDamping(float step)
{
	if (!m_dcp_body_roll_damping_enabled || m_dcp_body_roll_damping <= EPS_S || step <= 0.f ||
		!PPhysicsShell() || !PPhysicsShell()->isActive())
		return;

	u32 grounded_wheels = 0;
	for (const auto& wheel_item : m_wheels_map)
	{
		const u32 last_contact = wheel_item.second.collision_params.last_contact_time;
		if (last_contact != 0 && Device.dwTimeGlobal - last_contact <= m_dcp_body_roll_contact_grace_ms)
		{
			++grounded_wheels;
			if (grounded_wheels >= m_dcp_body_roll_min_grounded_wheels)
				break;
		}
	}

	if (grounded_wheels < m_dcp_body_roll_min_grounded_wheels)
		return;

	CPhysicsElement* root_element = PPhysicsShell()->get_ElementByStoreOrder(0);
	if (!root_element || !root_element->isActive() || !root_element->isEnabled())
		return;

	Fvector forward_axis = root_element->XFORM().k;
	if (forward_axis.square_magnitude() <= EPS_S)
		return;
	forward_axis.normalize();

	Fvector angular_velocity;
	root_element->get_AngularVel(angular_velocity);
	const float roll_velocity = angular_velocity.dotproduct(forward_axis);
	if (_abs(roll_velocity) <= EPS_S)
		return;

	// COP_ENGINE_PATCH: COP-ENG-072 - attenuate only the longitudinal roll
	// component. This is rate damping, not an upright spring: it does not pull a
	// parked car toward the world horizon and does not suppress airborne flips.
	const float retained_roll = expf(-m_dcp_body_roll_damping * step);
	angular_velocity.mad(forward_axis, roll_velocity * (retained_roll - 1.f));
	root_element->set_AngularVel(angular_velocity);
}

void CCar::PhDataUpdate(float step)
{
	UpdateDcpWheelFriction(step);
	UpdateDcpBodyRollDamping(step);

	// COP_ENGINE_PATCH: COP-ENG-044 - IX-Ray normally programs the steering
	// motor only when the input state changes. The donor DCP velocity curve is
	// time-dependent, so evaluating it only on the key press permanently leaves
	// the first direction at its five-percent starting response. Refresh the
	// motor every physics step while the profile input is held; SteerRight/Left
	// preserve m_steering_start_time when the direction has not changed.
	if (m_dcp_white_niva_profile && m_steer_nonlinear)
	{
		if (e_state_steer == right)
			SteerRight();
		else if (e_state_steer == left)
			SteerLeft();
	}

	LimitWheels();
	UpdateFuel(step);
	if (m_dcp_white_niva_profile && m_dcp_drive_pending)
		Drive();

	UpdatePower();
	if (b_engine_on && !b_starting && m_current_rpm < m_min_rpm)
		Stall();

	if (bkp)
	{
		UpdateBack();
	}

	if (brp)
		HandBreak();

	for (int k = 0; k < (int)m_doors_update.size(); ++k)
	{
		CCarDoor* D = m_doors_update[k];
		if (!D->update)
		{
			m_doors_update.erase(m_doors_update.begin() + k);
			--k;
		}
		else
		{
			D->Update();
		}
	}

	if (m_steering_wheels.empty())
	{
		m_steer_angle = 0;
		return;
	}

	m_steer_angle = m_steering_wheels.begin()->GetSteerAngle() * 0.1f + m_steer_angle * 0.9f;
	VERIFY(_valid(m_steer_angle));
}

BOOL CCar::UsedAI_Locations()
{
	return (FALSE);
}

u16 CCar::DriverAnimationType()
{
	return m_driver_anim_type;
}

void CCar::OnAfterExplosion()
{

}

void CCar::OnBeforeExplosion()
{
	setEnabled(FALSE);
}

void CCar::CarExplode()
{
	if (b_exploded) 
		return;

	CPHSkeleton::SetNotNeedSave();
	if (m_car_weapon)
		m_car_weapon->Action(CCarWeapon::eWpnActivate, 0);

	m_lights.TurnOffHeadLights();
	m_damage_particles.Stop1(this);
	m_damage_particles.Stop2(this);
	m_condition_damage_particles_active = false;
	b_exploded = true;

	CExplosive::GenExplodeEvent(Position(), Fvector().set(0.f, 1.f, 0.f));

	if (CActor* A = OwnerActor())
	{
		if (!m_doors.empty())
			m_doors.begin()->second.GetExitPosition(m_exit_position);
		else 
			m_exit_position.set(Position());

		A->detach_Vehicle();
		if (A->g_Alive() <= 0.f)A->character_physics_support()->movement()->DestroyCharacter();
	}

	if (CPHDestroyable::CanDestroy())
		CPHDestroyable::Destroy(ID(), "physic_destroyable_object");
}

template <class T> IC void CCar::fill_wheel_vector(LPCSTR S, xr_vector<T>& type_wheels)
{
	IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
	string64 S1;
	int count = _GetItemCount(S);
	for (int i = 0; i < count; ++i)
	{
		_GetItem(S, i, S1);

		u16 bone_id = pKinematics->LL_BoneID(S1);

		type_wheels.push_back(T());
		T& twheel = type_wheels.back();

		if (!bone_map.contains(bone_id))
		{
			bone_map.insert(std::make_pair(bone_id, physicsBone()));

			SWheel& wheel = (m_wheels_map.insert(std::make_pair(bone_id, SWheel(this)))).first->second;
			wheel.bone_id = bone_id;
			twheel.pwheel = &wheel;
			wheel.Load(S1);
			twheel.Load(S1);
		}
		else
		{
			twheel.pwheel = &(m_wheels_map.find(bone_id))->second;
			twheel.Load(S1);
		}
	}
}

IC void CCar::fill_exhaust_vector(LPCSTR S, xr_vector<SExhaust>& exhausts)
{
	IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
	string64					S1;
	int count = _GetItemCount(S);
	for (int i = 0; i < count; ++i)
	{
		_GetItem(S, i, S1);

		u16 bone_id = pKinematics->LL_BoneID(S1);
		exhausts.push_back(SExhaust(this));

		SExhaust& exhaust = exhausts.back();
		exhaust.bone_id = bone_id;

		if (!bone_map.contains(bone_id))
		{
			bone_map.insert(std::make_pair(bone_id, physicsBone()));
		}
	}
}

IC void CCar::fill_doors_map(LPCSTR S, xr_map<u16, CCarDoor>& doors)
{
	IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
	string64 S1;
	int count = _GetItemCount(S);

	for (int i = 0; i < count; ++i)
	{
		_GetItem(S, i, S1);

		u16 bone_id = pKinematics->LL_BoneID(S1);
		CCarDoor door(this);
		door.bone_id = bone_id;
		doors.insert(std::make_pair(bone_id, door));

		if (!bone_map.contains(bone_id))
		{
			bone_map.insert(std::make_pair(bone_id, physicsBone()));
		}
	}
}

IC void CCar::fill_bone_vector(LPCSTR S, xr_vector<u16>& bones)
{
	IKinematics* kinematics = smart_cast<IKinematics*>(Visual());
	string64 bone_name;
	const int count = _GetItemCount(S);

	for (int index = 0; index < count; ++index)
	{
		_GetItem(S, index, bone_name);
		const u16 bone_id = kinematics->LL_BoneID(bone_name);
		if (bone_id != BI_NONE && std::find(bones.begin(), bones.end(), bone_id) == bones.end())
			bones.push_back(bone_id);
	}
}

DLL_Pure* CCar::_construct()
{
	inherited::_construct();
	CScriptEntity::_construct();
	return						(this);
}

u16 CCar::Initiator()
{
	if (g_Alive() && Owner())
	{
		return Owner()->ID();
	}

	return ID();
}

float CCar::RefWheelMaxSpeed()
{
	return m_max_rpm / m_current_gear_ratio;
}

float CCar::EngineCurTorque()
{
	if (m_dcp_white_niva_profile)
		return m_current_engine_power / _max(m_current_rpm, EPS);
	return m_current_engine_power / m_current_rpm;
}
float CCar::RefWheelCurTorque()
{
	if (b_transmission_switching)
	{
		if (!m_dcp_white_niva_profile)
			return 0.f;
		return EngineCurTorque() * _abs(m_current_gear_ratio) * (1.f - m_transmission_clutch_factor);
	}

	return EngineCurTorque() * ((m_current_gear_ratio < 0.f) ? -m_current_gear_ratio : m_current_gear_ratio);
}

void CCar::GetRayExplosionSourcePos(Fvector& pos)
{
	random_point_in_object_box(pos, this);
}

void CCar::net_Relcase(CObject* O)
{
	CExplosive::net_Relcase(O);
	inherited::net_Relcase(O);

	if (m_memory)
		m_memory->remove_links(O);
}

void CCar::ASCUpdate()
{
	for (u16 i = 0; i < cAsCallsnum; ++i)
	{
		EAsyncCalls c = EAsyncCalls(1 << i);

		if (async_calls.test(u16(c)))
			ASCUpdate(c);
	}
}

void CCar::ASCUpdate(EAsyncCalls c)
{
	async_calls.set(u16(c), FALSE);
	switch (c) 
	{
	case ascSndTransmission:m_car_sound->TransmissionSwitch(); break;
	case ascSndStall:m_car_sound->Stop(); break;
	case ascExhoustStop:StopExhausts(); break;
	default: NODEFAULT;
	}
}

void CCar::AscCall(EAsyncCalls c)
{
	async_calls.set(u16(c), TRUE);
}

bool CCar::CanRemoveObject()
{
	return CExplosive::IsExploded() && !CExplosive::IsSoundPlaying();
}

void CCar::SetExplodeTime(u32 et)
{
	CDelayedActionFuse::Initialize(float(et) / 1000.f, CDamagableItem::DamageLevelToHealth(2));
}

u32 CCar::ExplodeTime()
{
	if (CDelayedActionFuse::isInitialized())
		return u32(CDelayedActionFuse::Time()) * 1000;
	else return 0;
}

void CCar::Die(CObject* who)
{
	inherited::Die(who);
	CarExplode();
}

Fvector CCar::ExitPosition()
{
	// COP_ENGINE_PATCH: COP-ENG-202 - forced/scripted exits need the current
	// chassis transform too; a point cached on entry can be metres behind it.
	if (UsesSafeVehicleExit())
	{
		auto door = m_doors.find(m_exit_bone);
		if (door != m_doors.end())
			door->second.GetExitPosition(m_exit_position);
		else
			m_exit_position.set(Position());
	}
	return m_exit_position;
}

Fvector	CCar::ExitVelocity()
{
	CPhysicsShell* P = PPhysicsShell();
	if (!P || !P->isActive())return Fvector().set(0, 0, 0);
	// COP_ENGINE_PATCH: COP-ENG-202 - inherit actual translation, not angular
	// rocking amplified at the exit point by settling doors/suspension.
	if (m_dcp_white_niva_profile)
	{
		Fvector velocity;
		P->get_LinearVel(velocity);
		if (velocity.square_magnitude() < .0225f) velocity.set(0.f, 0.f, 0.f);
		return velocity;
	}
	CPhysicsElement* E = P->get_ElementByStoreOrder(0);
	Fvector v = ExitPosition();

	E->GetPointVel(v, v);
	return v;
}

bool CCar::DoorHit(float P, s16 element, ALife::EHitType hit_type)
{
	if (hit_type == ALife::eHitTypeStrike && P > 20.f)
	{
		for (auto& [ID, DoorObj] : m_doors)
		{
			DoorObj.Open();
		}
	}

	auto i = m_doors.find(element);
	if (i != m_doors.end())
	{
		if (!m_part_damage_body_only)
			i->second.Hit(P);
		return true;
	}

	return false;
}
