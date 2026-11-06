#pragma once
#include "Entity.h"
#include "../xrPhysics/PhysicsShell.h"
#include "../xrPhysics/PHUpdateObject.h"
#include "script_entity.h"
#include "CarLights.h"
#include "InventoryOwner.h"
#include "holder_custom.h"
#include "PHSkeleton.h"
#include "DamagableItem.h"
#include "PHCollisionDamageReceiver.h"
#include "CarDamageParticles.h"
#include "xrServer_Objects_ALife.h"
#include "CarDamageParticles.h"
#include "hit_immunity.h"
#include "Explosive.h"
#include "PHDestroyable.h"
#include "DelayedActionFuse.h"
#include "CarDoors.h"
#include "../xrEngine/VehicleGrass.h"

// refs
class ENGINE_API			CBoneInstance;
class						CActor;
class						CInventory;
class						CSE_PHSkeleton;
class						CCarWeapon;
struct						dxGeomUserData;
struct						dSurfaceParameters;
// defs

#ifdef DEBUG
	#include "../xrEngine/StatGraph.h"
	#include "PHDebug.h"
#endif

class CScriptEntityAction;
class car_memory;

class CCar : 
	public CEntity, 
	public CScriptEntity,
	public CPHUpdateObject,
	public CHolderCustom,
	public CPHSkeleton,
	public CDamagableItem,
	public CPHDestroyable,
	public CPHCollisionDamageReceiver,
	public CHitImmunity,
	public CExplosive,
	public CDelayedActionFuse,
	public CInventoryOwner
{
	friend struct SWheel;
	friend class CCarDoor;
	friend class CCarLights;
private:
    // COP_ENGINE_PATCH: COP-ENG-170 - donor-local spheres, never ODE colliders.
    xr_vector<VehicleGrass::Sphere> m_grass_spheres;
    void LoadGrassSpheres(CInifile* ini);
    void UpdateGrassSpheres();
    void ClearGrassSpheres();
	void RegisterProfileCollisionSoundOwner();
	void UnregisterProfileCollisionSoundOwner();

	collide::rq_results		RQR;
	u16						m_bone_trunk;
	xr_vector<u16> m_trunk_aux_bones;
	xr_vector<u16> m_recursive_door_bones;
	xr_map<u16, u16> m_door_use_aliases;
	bool m_dcp_white_niva_profile;
	bool m_trunk_open_inventory;
	bool m_trunk_close_on_inventory_exit;
	float m_trunk_max_weight;
	float m_trunk_use_distance;
	Fvector m_trunk_interaction_padding;
	Fvector m_trunk_interaction_offset;
	bool m_fuel_system_enabled;
	bool m_part_damage_body_only;
	float m_fuel_consumption_scale;
	float m_fuel_canister_amount;
	float m_fuel_refuel_min_free;
	float m_fuel_refuel_distance;
	float m_durability_factor;
	float m_condition_warning_health;
	float m_condition_critical_health;
	float m_condition_smoke_health;
	bool m_condition_damage_particles_active;
	bool m_dcp_body_roll_damping_enabled;
	float m_dcp_body_roll_damping;
	u32 m_dcp_body_roll_min_grounded_wheels;
	u32 m_dcp_body_roll_contact_grace_ms;
	float m_dcp_starter_drive_delay;
	u32 m_dcp_engine_start_time;
	bool m_dcp_drive_pending;
	float m_interior_volume;
	float m_collision_hit_threshold;
	float m_hit_stall_threshold;
	float m_transmission_switch_delay;
	float m_transmission_settle_delay;
	float m_transmission_clutch_factor;
	float m_transmission_previous_rpm;
	u32 m_transmission_switch_time;
	xr_vector<float> m_dcp_gear_min_speed;
	float m_rpm_brp_increment_factor;
	float m_rpm_brp_decrement_factor;
	bool m_steer_nonlinear;
	bool m_steer_invert;
	float m_steering_limit_factor;
	float m_steering_start_time;
	float m_dcp_current_slip;
	float m_slip_sliding_threshold;
	float m_vel_sliding_threshold;
	float m_wheels_spin_factor_start;
	float m_wheels_spin_factor_stop;
	float m_friction_slip_factor;
	float m_friction2_slip_factor;
	float m_friction_spin_factor;
	float m_friction2_spin_factor;
	float m_friction_slip_speed;
	float m_friction2_steering_wheels;
	float m_friction2_nonsteering_wheels;
	float m_friction2_brp_boost;
	float m_contact_slip_scale;
	float m_handbrake_rpm_threshold;
	float m_handbrake_rpm_decrease;
	float m_drift_start_boost;
	float m_drift_boost_time;
	float m_steer_spin_boost;
	float m_steer_speed_boost;
	float m_steer_turn_factor;
	float m_steer_counter_turn_factor;
	float m_steer_turn_compensation;
	float m_steer_turn_compensation_max;
	float m_steer_turn_delta;
	float m_brake_boost;
	bool m_dcp_dashboard;
	// COP_ENGINE_PATCH: COP-ENG-151 - opt-in body-speed calibration for donor dashboard units.
	bool m_dcp_speedometer_body_profile;
	float m_dcp_speedometer_min;
	float m_dcp_speedometer_max;
	float m_dcp_speedometer_factor;
	float m_dcp_speedometer_offset;
	float m_dcp_speedometer_direction;
	float m_dcp_tachometer_factor;
	float m_dcp_tachometer_min_rpm;
	float m_dcp_tachometer_max_rpm;
	float m_dcp_tachometer_offset;
	float m_dcp_tachometer_direction;
	float m_dcp_fuel_angle;
	float m_dcp_fuel_offset;
	u16 m_dcp_fan_bone;
	float m_dcp_fan_speed;
	float m_dcp_fan_current_speed;
	float m_dcp_fan_angle;
	bool m_dcp_fan_engine;
	u32 m_dcp_fan_update_frame;
	float m_camera_inertion_n;
	float m_camera_inertion_k;
	float m_camera_angle_factor;
	bool m_vehicle_camera_dynamics;
	float m_camera_motion_strength;
	float m_camera_horizon_limit;
	float m_camera_motion_response;
	float m_camera_motion_return;
	float m_camera_acceleration_response;
	float m_camera_max_lateral_acceleration;
	float m_camera_max_longitudinal_acceleration;
	float m_camera_lateral_offset_per_accel;
	float m_camera_longitudinal_offset_per_accel;
	float m_camera_max_lateral_offset;
	float m_camera_max_longitudinal_offset;
	float m_camera_pitch_per_accel;
	float m_camera_max_pitch;
	Fvector m_camera_previous_local_velocity;
	Fvector m_camera_filtered_local_acceleration;
	Fvector m_camera_local_offset;
	Fvector m_camera_local_angles;
	bool m_camera_dynamics_valid;
	float m_door_air_resistance_scale;
	void ResetVehicleCameraDynamics();
	void UpdateVehicleCameraDynamics(float dt);
	void ApplyVehicleCameraHorizon();
	void ApplyVehicleCameraMotion();
	struct SDcpSuspensionVisual
	{
		CCar* car;
		u16 bone_id;
		u16 first_wheel;
		u16 second_wheel;
		bool move;
		bool steer;
		float offset;
	};
	xr_vector<SDcpSuspensionVisual*> m_dcp_suspension_visuals;
	static void _BCL cb_DcpSuspension(CBoneInstance* bone);
	static void _BCL cb_DcpFan(CBoneInstance* bone);
	void InitDcpSuspensionVisuals(CInifile* ini);
	void ClearDcpSuspensionVisuals();
	// COP_ENGINE_PATCH: COP-ENG-200 - animate only existing donor dashboard meshes.
	struct SDcpGaugeVisual
	{
		CCar* car;
		u16 bone_id;
		u8 kind;
		float angle, offset, factor, idle;
		float current_x, current_y;
		float gear_drive, gear_park, gear_reverse, gear_left, gear_middle, gear_right;
		bool manual, initialized;
		u32 last_frame, last_time;
	};
	xr_vector<SDcpGaugeVisual*> m_dcp_gauge_visuals;
	u64 m_dashboard_clock_game_base;
	u32 m_dashboard_clock_real_base;
	bool m_dashboard_real_clock;
	static void _BCL cb_DcpGauge(CBoneInstance* bone);
	void InitDcpGaugeVisuals(CInifile* ini);
	void ClearDcpGaugeVisuals();
	bool BuildProfileTrunkInteractionObb(u16 bone_id, Fobb& interaction_obb);

#ifdef DEBUG
	CFunctionGraph 			m_dbg_power_rpm			;
	CFunctionGraph 			m_dbg_torque_rpm		;
	CStatGraph	   			*m_dbg_dynamic_plot		;
	bool					b_plots					;
	float					TorqueRpmFun		(float rpm)		{return Parabola(rpm)/rpm;}
	void 					InitDebug			()				;
	void 					DbgSheduleUpdate	()				;
	void 					DbgUbdateCl			()				;
	void 					DbgCreatePlots		()				;
	void 					DBgClearPlots		()				;
#endif
////////////////////////////////////////////////////////////////////	
	Flags16						async_calls						;
static	const u16				cAsCallsnum						=3;
	enum EAsyncCalls
	{
		ascSndTransmission			=			1<<0,
		ascSndStall					=			1<<1,
		ascExhoustStop				=			1<<2,
		ascLast 					=			1<<cAsCallsnum
	};
	void					ASCUpdate			()				;
	void					ASCUpdate			(EAsyncCalls c)	;
	void					AscCall				(EAsyncCalls c)	;
////////////////////////////////////////////////////////////////////////////////////////
	virtual bool						CanRemoveObject				();
////////////////////////////////////////////////////////////////////////
			BONE_P_MAP					bone_map;					//interface for PhysicsShell
	static	void 						ActorObstacleCallback		(bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2);
	// COP_ENGINE_PATCH: COP-ENG-123 - all spawned cars notify AI of real physical contact.
	static void CharacterContactCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl*, SGameMtl*);
	virtual void						PhDataUpdate				(float step)			;
	virtual void						PhTune						(float step)			;
/////////////////////////////////////////////////////////////////////////
	virtual void						ApplyDamage					(u16 level)				;
	virtual	float						Health						()						{return GetfHealth();}
	virtual void						ChangeCondition				(float fDeltaCondition)	;
	virtual void						StartTimerEffects			()						{};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual CPhysicsShellHolder*		PPhysicsShellHolder			()						{return static_cast<CPhysicsShellHolder*>(this);}
	virtual ICollisionDamageReceiver	*PHCollisionDamageReceiver	()						{ return this; }

////////////////////////////////////////////////////////////////////////
	CCarDamageParticles					m_damage_particles;
///////////////////////////////////////////////////////////////////////
protected:
	enum ECarCamType{
		ectFirst	= 0,
		ectChase,
		ectFree
	};
public:

	bool TryTrunk();
	bool HasTrunkWeightLimit() const { return m_trunk_max_weight > EPS; }
	float TrunkMaxWeight() const { return m_trunk_max_weight; }
	bool CanStoreInTrunk(float additional_weight) const;
	bool TryProfileTrunk(const Fvector& ray_start, const Fvector& ray_direction, float* hit_distance = nullptr);
	bool IsProfileTrunkInRange(const Fvector& world_position);
	bool TryUsableBones();

	bool rsp,lsp,fwp,bkp,brp;
	Fmatrix m_root_transform;
	Fvector m_exit_position;
	u16 m_exit_bone = BI_NONE;

	enum eStateDrive
	{
		drive,
		neutral
	};

	eStateDrive e_state_drive;

	enum eStateSteer
	{
		right,
		idle,
		left
	};

	// Приборная панель
	Fvector4 m_speed_offsets;
	Fvector4 m_fuel_offsets;
	Fvector4 m_rpm_offsets;
	u16 m_speed_b;
	u16 m_fuel_b;
	u16 m_rpm_b;

	static void cb_Rpm(CBoneInstance* B);
	static void cb_Fuel(CBoneInstance* B);
	static void cb_Speed(CBoneInstance* B);

	eStateSteer e_state_steer;

	bool b_wheels_limited;
	bool b_engine_on;
	bool b_clutch;
	bool b_starting;
	bool b_breaks;
	bool b_transmission_switching;

	u32	 m_dwStartTime;
	float m_fuel;
	float m_fuel_tank;
	float m_fuel_consumption;

	u16	  m_driver_anim_type;

	float   m_break_start;
	float	m_break_time;
	float	m_breaks_to_back_rate;
	float	m_power_neutral_factor;//multiplier for power when accelerator is not pressed (0-1,0.25)
	bool	b_exploded;

	enum EDcpWheelSurface
	{
		dcpSurfaceRoad,
		dcpSurfaceDirt,
		dcpSurfaceGrass,
		dcpSurfaceWater
	};

	struct SWheel: 
	public CDamagableHealthItem
	{
		typedef		CDamagableHealthItem inherited;
		u16									bone_id				;
		bool								inited				;
		float								radius				;
		CPhysicsJoint						*joint				;
		CCar								*car				;
		float suspension_distance;
		float suspension_force_inc;
		float suspension_spring_factor;
		float suspension_damping_factor;
		float suspension_inertia;
		struct			SWheelCollisionParams
		{
			float							spring_factor		;
			float							damping_factor		;
			float							mu_factor			;
			float							dynamic_mu_factor	;
			float							dynamic_mu2_factor	;
			float							friction2_factor	;
			float							slip_speed			;
			Fvector						friction_direction	;
			bool							dcp_profile			;
			bool							steering			;
			bool							handbrake_wheel		;
			bool							spinning			;
			EDcpWheelSurface				surface				;
			u32							last_contact_time	;
			SWheelCollisionParams			();
		}									collision_params	;

	IC	static	void			applywheelCollisionParams				(const dxGeomUserData *ud,bool& do_colide,dContact& c,SGameMtl* material_1,SGameMtl* material_2);
		static	void			WheellCollisionCallback					(bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2)							;
				
				void 			Init									();//asumptions: bone_map is 1. ini parsed 2. filled in 3. bone_id is set 
				void			Load									(LPCSTR section);
				void 			RestoreNetState							(const CSE_ALifeCar::SWheelState& a_state)														;
				void 			SaveNetState							(NET_Packet& P)																					;
				void 			ApplyDriveAxisVel						(float vel)																						;
				void 			ApplyDriveAxisTorque					(float torque)																					;
				void 			ApplyDriveAxisVelTorque					(float vel,float torque)																		;
				void 			ApplySteerAxisVel						(float vel)																						;
				void 			ApplySteerAxisTorque					(float torque)																					;
				void 			ApplySteerAxisVelTorque					(float vel,float torque)																		;
				void 			SetSteerLoLimit							(float lo)																						;
				void			SetSteerHiLimit							(float hi)																						;
				void			SetSteerLimits							(float hi,float lo)																				;

virtual void ApplyDamage			(u16 level);
		SWheel(CCar* acar)
		{
			bone_id=BI_NONE;
			car=acar;
			joint=NULL;
			inited=false;
			suspension_distance = 0.f;
			suspension_force_inc = 0.f;
			suspension_spring_factor = 1.f;
			suspension_damping_factor = 1.f;
			suspension_inertia = 0.f;
		}
	};
	struct SWheelDrive  
	{
		SWheel* pwheel;
		float	pos_fvd;
		float	gear_factor;
		float	torque_factor;
		void	Init		()						;
		void	Drive		()						;
		void	Neutral		()						;
		void	UpdatePower	()						;
		float	ASpeed		() const;
		void	Load		(LPCSTR section)		;
	};
	struct SWheelSteer 
	{
		SWheel* pwheel;
		float pos_right;
		float lo_limit;
		float hi_limit;
		float steering_velocity;
		float steering_torque;
		bool  limited;			//zero limited for idle steering drive
		float GetSteerAngle();
	
		void	 Init		()						;
		void	 SteerRight	()						;
		void	 SteerLeft	()						;
		void	 SteerIdle	()						;
		void	 Limit		()						;
		void	 Load		(LPCSTR /*section*/){}	;
	};
	struct SWheelBreak 
	{
		SWheel			*pwheel							;
		float			break_torque					;
		float			hand_break_torque				;
		void		 Init			()					;
		void		 Break			(float k)			;
		void		 HandBreak		()					;
		void		 Neutral		()					;
		void		 Load			(LPCSTR section)	;
	};

	struct SExhaust
	{
		u16					bone_id;
		Fmatrix				transform;
		//Fvector				velocity;
		xr_shared_ptr<CParticlesObject>	p_pgobject;
		CPhysicsElement*	pelement;
		CCar*				pcar;
		void Init();
		void Play();
		void Stop();
		void Update();
		void Clear ();
		SExhaust(CCar* acar)
		{
			bone_id=BI_NONE;
			pcar=acar;
			p_pgobject=NULL;
			pelement=NULL;
		}
		~SExhaust();
	};

	struct SCarSound
	{
		ref_sound					snd_engine							;
		ref_sound					snd_engine_start					;
		ref_sound					snd_engine_stop						;
		ref_sound					snd_transmission					;
		ref_sound					snd_idle;
		ref_sound					snd_decel;
		ref_sound					snd_whine;
		ref_sound					snd_wheel_road;
		ref_sound					snd_wheel_dirt;
		ref_sound					snd_wheel_grass;
		ref_sound					snd_wheel_water;
		ref_sound					snd_wheel_skid;
		xr_vector<ref_sound>		collision_sounds;

		struct SDoorSound
		{
			u16 bone_id;
			ref_sound open;
			ref_sound close;
		};
		xr_vector<SDoorSound*> door_sounds;
		xr_vector<std::pair<u16, bool>> pending_door_sounds;

		enum ESoundState
		{
			sndOff,
			sndStalling,
			sndStoping,
			sndStarting,
			sndDrive
		} eCarSound;
		void	Update				()							;
		void	UpdateStarting		()							;
		void	UpdateStoping		()							;
		void	UpdateStalling		()							;
		void	UpdateDrive			()							;
		void	UpdateDcpProfile	()							;
		void	StartDcpLoops		()							;
		void	StopDcpLoops			()							;
		void	QueueDoorSound		(u16 bone_id, bool opening)		;
		void	SetDoorSoundPosition	(ref_sound& snd, u16 bone_id)	;
		float	InteriorVolumeFactor	() const				;
		void	SetInteriorOneShotVolume(ref_sound& snd)			;
		bool	PlayCollisionSound	(const Fvector& position, float impact_volume, u16 material);
		void	SwitchState			(ESoundState new_state)		;
		void	SetSoundPosition	(ref_sound	&snd)			;
		void	SwitchOff			()							;
		void	SwitchOn			()							;
		void	Init				()							;
		void	Destroy				()							;
		void	Start				()							;
		void	Stop				()							;
		void	Stall				()							;
		void	Drive				()							;
		void	TransmissionSwitch	()							;

				SCarSound			(CCar* car)					;
				~SCarSound			()							;
		Fvector	relative_pos									;
		// COP_ENGINE_PATCH: COP-ENG-198 - donor engine location is distinct from gearbox/wheels.
		Fvector relative_engine_pos;
		float	volume											;
		float	sound_rpm;
		float	engine_sound_scale_min;
		float	engine_sound_scale_max;
		float	engine_sound_scale_factor;
		float	whine_speed;
		bool	whine_reverse;
		float	previous_rpm;
		float	idle_volume;
		float	run_volume;
		float	decel_volume;
		float	whine_volume;
		float	wheel_volume;
		float	skid_volume;
		float	collision_volume_scale;
		u16		collision_material;
		u32		engine_start_delay								;//snd_engine starts after engine_start_delay ms by snd_engine_start
		u32		time_state_start								;
		CCar*	pcar											;
	} *m_car_sound;

private:
	typedef CEntity			inherited;
private:
	CCarWeapon*				m_car_weapon;
	float					m_steer_angle;
	u16						m_bone_steer;
	CCameraBase*			camera[3];
	CCameraBase*			active_camera;

	Fvector3				m_camera_position;
	Fvector3				*m_camera_current_position;
	Fvector3				m_camera_position_2;
	Fvector3				m_camera_position_3;

	xr_map   <u16,SWheel>	m_wheels_map;
	xr_vector <SWheelDrive> m_driving_wheels;
	xr_vector <SWheelSteer> m_steering_wheels;
	xr_vector <SWheelBreak> m_breaking_wheels;
	xr_vector <SExhaust>	m_exhausts;
	shared_str				m_exhaust_particles;
	xr_map <u16, CCarDoor>	m_doors;
	xr_vector <CCarDoor*>	m_doors_update;
	// COP_ENGINE_PATCH: COP-ENG-197 - model-declared linked physical door leaves.
	xr_map<u16, xr_vector<u16>> m_linked_door_groups;
	void InitLinkedDoorGroups(CInifile* ini);
	bool UseLinkedDoorGroup(u16 selected);
	xr_vector <Fvector>		m_gear_ratious;
	xr_vector <Fmatrix>		m_sits_transforms;// m_sits_transforms[0] - driver_place
	float					m_current_gear_ratio;

	/////////////////////////////////////////////////////////////
	bool					b_auto_switch_transmission;

	/////////////////////////////////////////////////////////////
	float					m_doors_torque_factor;
	/////////////////////////////////////////////////////////////

	float					m_max_power					;//best rpm
	float					m_power_increment_factor	;
	float					m_power_decrement_factor	;
	float					m_rpm_increment_factor		;
	float					m_rpm_decrement_factor		;
	/////////////////////porabola
	float m_a,m_b,m_c;

	float					m_current_engine_power;
	float					m_current_rpm;

	float					m_axle_friction;

	float					m_max_rpm;
	float					m_min_rpm;
	float					m_power_rpm;//max power
	float					m_torque_rpm;//max torque


	float					m_steering_speed;
	float					m_ref_radius;
	size_t					m_current_transmission_num;
	///////////////////////////////////////////////////
	CCarLights				m_lights;
	////////////////////////////////////////////////////
	/////////////////////////////////////////////////
	void				InitParabola();
	float				Parabola(float rpm);
	//float GetSteerAngle();
	void				 LimitWheels						()	;
	void				 Drive								()	;
	void				 Starter							()	;
	void				 StartEngine						()	;
	void				 StopEngine							()	;
	void				 Stall								()	;
	void				 Clutch								()	;
	void				 Unclutch							()	;
	void				 NeutralDrive						()	;
	void				 UpdatePower						()	;
	void				 UpdateDcpWheelFriction			(float step);
	void				 UpdateDcpBodyRollDamping		(float step);
	float				 DcpSteeringVelocity				(float current_angle, float target_direction) const;
	void				 ReleasePedals						()	;
	void				 ResetKeys							()	;
	u16					 GetTrunkBone						() const { return m_bone_trunk; };

	////////////////////////////////////////////////////////////////////////////
	float				RefWheelMaxSpeed					()	;
	float				EngineCurTorque						()	;
	float				RefWheelCurTorque					()	;
	float	 			EnginePower							()	;
	float	 			EngineDriveSpeed					()	;
	float	 			DriveWheelsMeanAngleRate			() const;
	float	 			DcpSoundEngineSpeed				() const;
IC	float	 			EngineRpmFromWheels					() const {return _abs(DriveWheelsMeanAngleRate()*m_current_gear_ratio);}
	/////////////////////////////////////////////////////////////////////////	
	void				SteerRight							();
	void				SteerLeft							();
	void				SteerIdle							();
	void				Transmission						(size_t num);
	void				TransmissionUp						();
	void				TransmissionDown					();
IC	size_t				CurrentTransmission					() const {return m_current_transmission_num;}

	float				EffectiveGravity					();
	float				AntiGravityAccel					();
	float				GravityFactorImpulse				();
	void StartBreaking									();
	void StopBreaking									();
	void UpdateBack										();

	void HandBreak										();
	void ReleaseHandBreak								();
	void DriveForward									();
	void DriveBack										();
	void ParseDefinitions								();
	void CreateSkeleton									(CSE_Abstract	*po);//creates m_pPhysicsShell
	void Init											();

	void PlayExhausts									();
	void StopExhausts									();
	void UpdateExhausts									();
	void ClearExhausts									();
	void UpdateConditionDamageParticles					();
	void UpdateFuel										(float time_delta);
	float AddFuel										(float ammount); //ammount - fuel to load, ret - fuel loaded
	bool FuelSystemEnabled								() const { return m_fuel_system_enabled; }
	float FuelRefuelDistance								() const { return m_fuel_refuel_distance; }
	bool CanRefuelCanister								() const;
	float TryRefuelCanister								();
	void CarExplode										();
	////////////////////////////////////////////		////////

	void					OnCameraChange				(int type);





	bool					HUDview						( ) { return IsFocused(); }

	static void	_BCL		cb_Steer					(CBoneInstance* B);
	virtual	void			Hit							(SHit* pHDS);
	virtual void			Die							(CObject* who);
	virtual void PHHit									(SHit &H);
			bool WheelHit								(float P,s16 element,ALife::EHitType hit_type);
			bool DoorHit								(float P,s16 element,ALife::EHitType hit_type);
public:
	void DoExit();
	bool IsDcpWhiteNivaProfile() const { return m_dcp_white_niva_profile; }
	bool PlayProfileCollisionSound(const Fvector& position, float impact_volume, u16 material);
	static bool PlayProfileCollisionSoundForHolder(
		IPhysicsShellHolder* holder, const Fvector& position, float impact_volume, u16 material);
	bool UsesProfileTrunkInteraction() const
	{
		return m_dcp_white_niva_profile && m_trunk_open_inventory && m_bone_trunk != BI_NONE;
	}
	void CloseProfileTrunk();

	void SwitchEngine();
	void PressRight();
	void PressLeft();
	void PressForward();
	void PressBack();
	void PressBreaks();

	void ReleaseRight();
	void ReleaseLeft();
	void ReleaseForward();
	void ReleaseBack();
	void ReleaseBreaks();

	virtual bool			allowWeapon					() const		{return true;};
	virtual bool			HUDView						() const;
	virtual Fvector			ExitPosition				();
	bool UsesSafeVehicleExit() const { return m_dcp_white_niva_profile; }
	// COP_ENGINE_PATCH: COP-ENG-203 - local driver/gunner context, no save migration.
	bool IsWeaponMode() const;
	// COP_ENGINE_PATCH: COP-ENG-206 - ammunition belongs to the current gunner.
	CActor* WeaponOwner() { return IsWeaponMode() ? OwnerActor() : nullptr; }
	u32 WeaponInventoryAmmo() const;

	bool IsWeaponExternalView() const;
	bool SwitchVehicleRole(bool gunner);
	void SwitchWeaponView(bool external);
	virtual Fvector			ExitVelocity				();
	void					GetVelocity					(Fvector& vel)	{m_pPhysicsShell->get_LinearVel(vel);}
	void					cam_Update					(float dt, float fov);
	void					detach_Actor				();
	bool					attach_Actor				(CGameObject* actor);
	bool					is_Door						(u16 id,xr_map<u16, CCarDoor>::iterator& i);
	bool					is_Door						(u16 id);
	bool					DoorOpen					(u16 id);
	bool					DoorClose					(u16 id);
	bool					DoorUse						(u16 id);
	bool					DoorSwitch					(u16 id);
	bool					Enter						(const Fvector& pos,const Fvector& dir,const Fvector& foot_pos);
	bool					Exit						(const Fvector& pos,const Fvector& dir);
	bool					Use							(const Fvector& pos,const Fvector& dir,const Fvector& foot_pos);
	u16						DriverAnimationType 		();
	// Core events
	virtual DLL_Pure		*_construct					();
	virtual void			Load						( LPCSTR section );
	virtual BOOL			net_Spawn					( CSE_Abstract* DC );
	virtual void			net_Destroy					();
	virtual void			UpdateCL					( ); 
	virtual	void			UpdateEx					(float fov); //called by owner

	virtual void			shedule_Update				(u32 dt);
	virtual void			renderable_Render			( ); 
	virtual	bool			bfAssignMovement			(CScriptEntityAction *tpEntityAction);
	virtual	bool			bfAssignObject				(CScriptEntityAction *tpEntityAction);
    virtual bool			is_ai_obstacle() const		{return true;};
	// Network
	virtual void			net_Export					(NET_Packet& P);				// export to server
	virtual void			net_Import					(NET_Packet& P);				// import from server
	virtual BOOL			net_Relevant				()	{ return IsMyCar() || OnServer(); };		// relevant for export to server
	virtual BOOL			UsedAI_Locations			();
	virtual	void			net_Relcase					(CObject* O );
	// Input
	virtual void			OnMouseMove					(int x, int y);
	virtual void			OnKeyboardPress				(int dik);
	virtual void			OnKeyboardRelease			(int dik);
	virtual void			OnKeyboardHold				(int dik);
	virtual void			vfProcessInputKey			(int iCommand, bool bPressed);
	virtual void			OnEvent						( NET_Packet& P, u16 type);
	virtual void			OnAfterExplosion			();
	virtual void			OnBeforeExplosion			();
	virtual void			GetRayExplosionSourcePos	(Fvector &pos);
	virtual void			ActivateExplosionBox		(const Fvector &size,Fvector &in_out_pos){};
	virtual void			ResetScriptData				(void *P=0);

	virtual void			Action						(u16 id, u32 flags);
	virtual void			SetParam					(int id, Fvector2 val);
	virtual void			SetParam					(int id, Fvector val);
			bool			HasWeapon					();
			bool			WpnCanHit					();
			float			FireDirDiff					();
			bool			isObjectVisible				(CScriptGameObject* O);
			Fvector			CurrentVel					();
	virtual float			GetfHealth					() const		{return CEntity::GetfHealth();};
	virtual float			SetfHealth					(float value)	{return CEntity::SetfHealth(value);};

	// Hits
	virtual void			HitSignal					(float /**HitAmount/**/,	Fvector& /**local_dir/**/, CObject* /**who/**/, s16 /**element/**/)	{};
	virtual void			HitImpulse					(float /**amount/**/,		Fvector& /**vWorldDir/**/, Fvector& /**vLocalDir/**/)			{};
	virtual void			g_fireParams				(const CHudItem* /**pHudItem/**/, Fvector& /**P/**/, Fvector& /**D/**/)											{};
	virtual u16				Initiator					();
	// HUD
	virtual void			OnHUDDraw					(CCustomHUD* hud);
	
	CCameraBase*			Camera						()			{return active_camera;}
	void					SetExplodeTime				(u32 et)	;
			u32				ExplodeTime					()			;

		  void						VisualUpdate						(float fov=90.0f);
protected:
	virtual void					SpawnInitPhysics					(CSE_Abstract	*D)																;
	virtual void					net_Save							(NET_Packet& P)																	;
	virtual	BOOL					net_SaveRelevant					()																				;
			void					SaveNetState						(NET_Packet& P)																	;
	virtual	void					RestoreNetState						(CSE_PHSkeleton* po)															;
			void					SetDefaultNetState					(CSE_PHSkeleton* po)															;
	virtual void					save(NET_Packet& output_packet) override;
	virtual void					load(IReader& input_packet) override;
	virtual bool					IsHudModeNow		(){return false;};
	
public:
	CCar(void);
	virtual ~CCar(void);
	virtual BOOL					AlwaysTheCrow						();

public:
	virtual CEntity*					cast_entity				()						{return this;}
private:
	CCameraBase* m_weapon_camera = nullptr;
	CCameraBase* m_weapon_external_camera = nullptr;
	int m_driver_camera_type = ectFirst;
	template <class T> IC void fill_wheel_vector(LPCSTR S,xr_vector<T>& type_wheels);
	IC void fill_exhaust_vector(LPCSTR S,xr_vector<SExhaust>& exhausts);
	IC void fill_doors_map(LPCSTR S,xr_map<u16, CCarDoor>& doors);
	IC void fill_bone_vector(LPCSTR S, xr_vector<u16>& bones);
	u16 resolve_door_bone(u16 id) const;

	virtual	void reinit			();
	virtual	void reload			(LPCSTR section);
	virtual CGameObject			*cast_game_object			()	{return this;}
	virtual CExplosive			*cast_explosive				()	{return this;}
	virtual CPhysicsShellHolder	*cast_physics_shell_holder	()	{return this;}
	virtual CParticlesPlayer	*cast_particles_player		()	{return this;}
	virtual CScriptEntity		*cast_script_entity			()	{return this;}
	virtual IDamageSource		*cast_IDamageSource			()	{return this;}
	virtual CHolderCustom		*cast_holder_custom			()	{return this;}
	virtual CCar*				cast_car					()	{return this;}
	virtual CInventoryOwner*	cast_inventory_owner		()	{return this;}

private:
	car_memory	*m_memory;
	struct SCarNetUpdate
	{
		u32	TimeStamp;
		xr_vector<SPHNetState> StateVec;
	};
	xr_deque<SCarNetUpdate> m_CarNetUpdates;

	void Interpolate();
	float InterpolateStates(u32 element, SCarNetUpdate const& first, SCarNetUpdate const& last, SPHNetState& current);

public:
	virtual bool unlimited_ammo() { return false; };
	virtual bool infinite_fire() { return false; }
	// Inventory for the car	
	CInventory* GetInventory() { return &CInventoryOwner::inventory(); }

	bool IsMyCar() const;
	virtual void SyncRead(NET_Packet& Packet);
	virtual void SyncWrite(NET_Packet& Packet);
private:
	xr_hash_map<u16, shared_str> UsableBones;
	u16 UsableBonesActive = BI_NONE;

public:
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
