#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterMoveToRestrictorAbstract CStateMonsterMoveToRestrictor<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToRestrictorAbstract::initialize()
{
	inherited::initialize();
	this->object->path().prepare_builder();

	Fvector position;
	u32		node = this->object->control().path_builder().restrictions().accessible_nearest(this->object->Position(), position);
	m_recovery_pending = !ai().level_graph().valid_vertex_id(node);
	m_recovery_retry_at = Device.dwTimeGlobal + 500;
	
	if (ai().level_graph().valid_vertex_id(node)) {
		this->object->path().set_target_point(ai().level_graph().vertex_position(node), node);
	}
}

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToRestrictorAbstract::execute()
{
	// COP_ENGINE_PATCH: COP-ENG-186 - a failed initial exit is retried, not abandoned forever.
	if (m_recovery_pending) {
		if (s32(Device.dwTimeGlobal - m_recovery_retry_at) >= 0) {
			m_recovery_retry_at = Device.dwTimeGlobal + 500;
			Fvector position;
			const u32 node = this->object->control().path_builder().restrictions().accessible_nearest(this->object->Position(), position);
			if (ai().level_graph().valid_vertex_id(node)) {
				this->object->path().set_target_point(position, node);
				m_recovery_pending = false;
			}
		}
		if (m_recovery_pending) {
			this->object->set_action(ACT_STAND_IDLE);
			return;
		}
	}
	this->object->set_action					(ACT_RUN);
	
	this->object->anim().accel_activate		(EAccelType(eAT_Aggressive));
	this->object->anim().accel_set_braking	(true);
	this->object->set_state_sound			(MonsterSound::eMonsterSoundIdle);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveToRestrictorAbstract::check_start_conditions()
{
	return (!this->object->control().path_builder().accessible(this->object->Position()));
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveToRestrictorAbstract::check_completion()
{
	return (this->object->control().path_builder().accessible(this->object->Position()));
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterMoveToRestrictorAbstract
