#pragma once
#include "PHSimpleCharacter.h"
class CPHAICharacter : 
	public CPHSimpleCharacter
{
	using inherited = CPHSimpleCharacter;

	bool	m_forced_physics_control;
	// COP_ENGINE_PATCH: COP-ENG-123 - retain real vehicle contact response between AI updates.
	u64 m_vehicle_contact_step = u64(-1);
public:
	CPHAICharacter();
	virtual CPHAICharacter* CastAICharacter() { return this; }
	virtual		void		ValidateWalkOn();
	virtual		bool		TryPosition(Fvector pos, bool exact_state);
	virtual		void		Jump(const Fvector& jump_velocity);
	virtual		void		SetMaximumVelocity(dReal vel) { m_max_velocity = vel; }
	virtual		void		InitContact(dContact* c, bool& do_collide, u16 material_idx_1, u16 material_idx_2);
	virtual		void		SetForcedPhysicsControl(bool v) { m_forced_physics_control = v; }
	virtual		bool		ForcedPhysicsControl() { return m_forced_physics_control || VehicleContactActive(); }
	virtual		void		Create(dVector3 sizes);
	void NotifyVehicleContact();
	bool VehicleContactActive() const;
private:
	virtual		void		UpdateStaticDamage(dContact* c, SGameMtl* tri_material, bool bo1) {}
#ifdef DEBUG_DRAW
	virtual		void		OnRender() override;
#endif
};
