#pragma once
#include "../state.h"

template<typename _Object>
class CStateMonsterMoveToRestrictor : public CState<_Object> {
	typedef CState<_Object> inherited;
	// COP_ENGINE_PATCH: COP-ENG-186 - transient retry state, never serialized.
	bool m_recovery_pending = false;
	u32 m_recovery_retry_at = 0;

public:
						CStateMonsterMoveToRestrictor	(_Object *obj) : inherited(obj) {}
	virtual				~CStateMonsterMoveToRestrictor	() {}

	virtual void		initialize					();
	virtual	void		execute						();

	virtual bool		check_start_conditions		();
	virtual bool		check_completion			();
	virtual void		remove_links				(CObject* object_) { inherited::remove_links(object_);}
};

#include "state_move_to_restrictor_inline.h"
