#include "StdAfx.h"

#include "GraviZone.h"

#include "../xrPhysics/PhysicsShell.h"
#include "entity_alive.h"
#include "PHMovementControl.h"
#include "xrMessages.h"
#include "PhysicsShellHolder.h"
#include "Level.h"
#include "CharacterPhysicsSupport.h"
#include "Car.h"
CBaseGraviZone ::CBaseGraviZone (void)
{
	m_dwTeleTime = 0;
	m_fThrowInImpulseCar = 0.f;
	m_fThrowInHitKoefAlive = 1.f;
	m_fThrowInHitKoefCar = 1.f;
}

CBaseGraviZone ::~CBaseGraviZone (void)
{
}

void CBaseGraviZone ::Load(LPCSTR section)
{
	inherited::Load(section);
	
	m_fThrowInImpulse		= pSettings->r_float(section,	"throw_in_impulse");//800.f;
	m_fThrowInImpulseAlive	= pSettings->r_float(section,	"throw_in_impulse_alive");//800.f;
	// COP_ENGINE_PATCH: COP-ENG-043 - read the original DCP car anomaly
	// coefficients with the same fallbacks as the donor DLL.
	m_fThrowInImpulseCar = READ_IF_EXISTS(pSettings, r_float, section,
		"throw_in_impulse_car", m_fThrowInImpulseAlive);
	m_fThrowInHitKoefAlive = READ_IF_EXISTS(pSettings, r_float, section,
		"throw_in_hit_koef_alive", 1.f);
	m_fThrowInHitKoefCar = READ_IF_EXISTS(pSettings, r_float, section,
		"throw_in_hit_koef_car", m_fThrowInHitKoefAlive);
	m_fThrowInAtten			= pSettings->r_float(section,	"throw_in_atten");
	m_fBlowoutRadiusPercent = pSettings->r_float(section,	"blowout_radius_percent");//0.3f;

	m_fTeleHeight			= pSettings->r_float(section,	"tele_height");//1.5f;
    m_dwTimeToTele			= pSettings->r_u32(section,		"time_to_tele");//7000;
	m_dwTelePause			= pSettings->r_u32(section,		"tele_pause");//1000

	if(pSettings->line_exist(section,	"tele_particles_big"))
		m_sTeleParticlesBig = pSettings->r_string(section,	"tele_particles_big");
	else
		m_sTeleParticlesBig = nullptr;
	
	if(pSettings->line_exist(section, "tele_particles_small"))
		m_sTeleParticlesSmall = pSettings->r_string(section, "tele_particles_small");
	else
		m_sTeleParticlesSmall = nullptr;
}

BOOL CBaseGraviZone ::net_Spawn(CSE_Abstract* DC)
{
	return inherited::net_Spawn(DC);
}

void CBaseGraviZone ::net_Destroy()
{
	Telekinesis().deactivate();
	inherited::net_Destroy();
}

void CBaseGraviZone ::shedule_Update		(u32 dt)
{
	inherited::shedule_Update(dt);
	Telekinesis().schedule_update();
}


bool  CBaseGraviZone ::BlowoutState()
{
	bool result = inherited::BlowoutState();

	UpdateBlowout();
	AffectObjects();

	return result;
}


bool CBaseGraviZone ::IdleState()
{
	bool result = inherited::IdleState();

	m_dwTeleTime += Device.dwTimeDelta;

	if(!result)
	{
		if(m_dwTeleTime> m_dwTimeToTele)
		{
			for (SZoneObjectInfo& info : m_ObjectInfoMap)
			{
				if(info.object && !info.object->getDestroy())
				{
					CPhysicsShellHolder* GO = info.object->cast_physics_shell_holder();

					if (GO && GO->PPhysicsShell() && Telekinesis().is_active_object(GO))
					{
						Telekinesis().deactivate(GO);
						StopTeleParticles(GO);
					}
				}
			}
		}
		if(m_dwTeleTime> m_dwTimeToTele + m_dwTelePause)
		{
			m_dwTeleTime = 0;

			for (SZoneObjectInfo& info : m_ObjectInfoMap)
			{
				if (info.object && !info.object->getDestroy())
				{
					CPhysicsShellHolder* GO = info.object->cast_physics_shell_holder();

					if (GO && GO->PPhysicsShell() && !Telekinesis().is_active_object(GO))
					{
						const float response = VehicleAnomalyResponse(GO);
						if (IsProfiledVehicle(GO) && response <= EPS)
							continue;
						CTelekineticObject* teleObject = Telekinesis().activate(
							GO, 0.1f * response * VehicleAnomalyPullStrengthScale(GO),
							m_fTeleHeight, m_dwTimeToTele);
						if (teleObject)
						{
							teleObject->set_acceleration_limit(
								VehicleAnomalyPullAccelerationLimit(GO));
							teleObject->set_gravity_compensation(
								VehicleAnomalyTelekinesisGravityCompensation(GO));
							teleObject->set_hold_enabled(
								VehicleAnomalyTelekinesisHoldEnabled(GO));
						}
						PlayTeleParticles(GO);
					}
				}
			}
		}
	}
	else
		Telekinesis().deactivate();

	return result;
}

bool CBaseGraviZone::CheckAffectField(CPhysicsShellHolder* GO,float dist_to_radius)
{
	return dist_to_radius>BlowoutRadiusPercent(GO);
}

void CBaseGraviZone ::Affect(SZoneObjectInfo* O) 
{
	CPhysicsShellHolder* GO = smart_cast<CPhysicsShellHolder*>(O->object);
	if(!GO) return;


	//////////////////////////////////////////////////////////////////////////
	//	зат€гиваем объет по направлению к центру зоны

	Fvector					throw_in_dir;
	Fvector					zone_center;
	ThrowInCenter			(zone_center);
	Fvector					go_center;
	GO->Center				(go_center);
	throw_in_dir.sub		(zone_center, go_center);

	float dist				= throw_in_dir.magnitude();
	float dist_to_radius	= dist/Radius();

	if(!fis_zero(dist))
		throw_in_dir.mul(1.f/dist);
	else 
		throw_in_dir.set(0.f,1.f,0.f);

	bool CanApplyPhisImpulse = GO->Local() == TRUE;

	if( CheckAffectField(GO,dist_to_radius)&& CanApplyPhisImpulse)
	{
		AffectPull(GO,throw_in_dir,dist);
	}
	else
	{
		//////////////////////////////////////////////////////////////////////////
		// выброс аномалии
		
		//если врем€ выброса еще не пришло
		if(m_dwBlowoutExplosionTime<(u32)m_iPreviousStateTime ||
			m_dwBlowoutExplosionTime>=(u32)m_iStateTime)
		{

			AffectPull(GO,throw_in_dir,BlowoutRadiusPercent(GO)*Radius());	
			return;
		}
		AffectThrow(O,GO,throw_in_dir,dist);
			
	}
}

void CBaseGraviZone::ThrowInCenter(Fvector& C)
{
	Center(C);
}

void CBaseGraviZone::AffectPull(CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist)
{
	CEntityAlive* EA = smart_cast<CEntityAlive*>(GO);	
	if(EA && EA->g_Alive())
	{
		AffectPullAlife(EA,throw_in_dir,dist);
	}
	else if(GO && GO->PPhysicsShell())
	{
		AffectPullDead(GO,throw_in_dir,dist);
	}
}

void CBaseGraviZone::AffectPullAlife(CEntityAlive* EA,const Fvector& throw_in_dir,float dist)
{
	float rel_power = RelativePower(dist, Radius());
	float throw_power = m_fThrowInImpulseAlive*rel_power*rel_power*rel_power*rel_power*rel_power;

	Fvector vel;
	vel.set(throw_in_dir);
	vel.mul(throw_power);
	EA->character_physics_support()->movement()->AddControlVel(vel);
}

void CBaseGraviZone::AffectPullDead(CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist)
{
	GO->PPhysicsShell()->applyImpulse(throw_in_dir, dist * PullImpulse(GO) * GO->GetMass() / 100.f);
}

float CBaseGraviZone::PullImpulse(CPhysicsShellHolder* GO) const
{
	// COP_ENGINE_PATCH: COP-ENG-045 - retain the donor car-specific pull path.
	// COP_ENGINE_PATCH: COP-ENG-066 - make it reusable and mass-dependent.
	if (!IsProfiledVehicle(GO))
		return m_fThrowInImpulse;
	return m_fThrowInImpulseCar * VehicleAnomalyResponse(GO) *
		VehicleAnomalyPullStrengthScale(GO);
}

float CBaseGraviZone::TelekinesisImpulse(CPhysicsShellHolder* GO) const
{
	if (!IsProfiledVehicle(GO))
		return m_fThrowInImpulse;
	return m_fThrowInImpulseCar * VehicleAnomalyResponse(GO) *
		VehicleAnomalyPullStrengthScale(GO);
}

float CBaseGraviZone::PullHitKoef(CPhysicsShellHolder* GO) const
{
	if (!IsProfiledVehicle(GO))
		return m_fThrowInHitKoefAlive;
	return m_fThrowInHitKoefCar * VehicleAnomalyResponse(GO) * VehicleAnomalyDamageScale(GO);
}

float CBaseGraviZone::AffectTimeDeltaSeconds() const
{
	return float(m_iStateTime - m_iPreviousStateTime) / 1000.f;
}

void CBaseGraviZone::AffectThrow(SZoneObjectInfo* O, CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist)
{
	Fvector position_in_bone_space;

	float power = Power(dist, Radius()); //Power(GO->Position().distance_to(zone_center));
	float impulse = m_fHitImpulseScale*power*GO->GetMass();
	if (IsProfiledVehicle(GO))
	{
		const float response = VehicleAnomalyResponse(GO);
		// COP_ENGINE_PATCH: COP-ENG-068 - keep throw impulse donor-exact but
		// scale only the direct hull hit to IX-Ray's condition range.
		power *= response * VehicleAnomalyDamageScale(GO);
		impulse *= response;
	}

	if(power > 0.01f) 
	{
		position_in_bone_space.set(0.f,0.f,0.f);
		CreateHit(GO->ID(),ID(),throw_in_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);
		PlayHitParticles(GO);
	}
}


void CBaseGraviZone::PlayTeleParticles(CGameObject* pObject)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if(!PP) return;

	shared_str particle_str = nullptr;

	//разные партиклы дл€ объектов разного размера
	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sTeleParticlesSmall) return;
		particle_str = m_sTeleParticlesSmall;
	}
	else
	{
		if(!m_sTeleParticlesBig) return;
		particle_str = m_sTeleParticlesBig;
	}

	PP->StartParticles(particle_str, Fvector().set(0,1,0), ID());
}

void CBaseGraviZone::StopTeleParticles(CGameObject* pObject)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if(!PP) return;
	shared_str particle_str = nullptr;

	//разные партиклы дл€ объектов разного размера
	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sTeleParticlesSmall) return;
		particle_str = m_sTeleParticlesSmall;
	}
	else
	{
		if(!m_sTeleParticlesBig) return;
		particle_str = m_sTeleParticlesBig;
	}

	//остановить партиклы
	PP->StopParticles	(particle_str, BI_NONE, true);
}

void CBaseGraviZone::net_Relcase(CObject* O)
{
	inherited::net_Relcase(O);
	
	Telekinesis().remove_links(O);
}
