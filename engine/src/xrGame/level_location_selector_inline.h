////////////////////////////////////////////////////////////////////////////
//	Module 		: level_location_selector_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 18.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Level location selector inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _VertexEvaluator,\
	typename _vertex_id_type\
>

#define CLevelLocationSelector CBaseLocationSelector<ILevelGraph,_VertexEvaluator,_vertex_id_type>

TEMPLATE_SPECIALIZATION
IC	CLevelLocationSelector::CBaseLocationSelector	(CRestrictedObject *object) :
	inherited(object, this->selector_manager)
{
}

TEMPLATE_SPECIALIZATION
IC	void CLevelLocationSelector::before_search	(_vertex_id_type &vertex_id)
{
	if (this->m_restricted_object) {
		if (!this->m_restricted_object->accessible(vertex_id)) {
			Fvector dest_pos;
			// COP_ENGINE_PATCH: COP-ENG-186 - the selector still needs a valid seed on failure.
			const auto recovered = this->m_restricted_object->accessible_nearest(this->m_graph->vertex_position(vertex_id),dest_pos);
			if (this->m_graph->valid_vertex_id(recovered)) vertex_id = recovered;
		}
		this->m_restricted_object->add_border(vertex_id, this->m_evaluator->m_fRadius);
	}
}

TEMPLATE_SPECIALIZATION
IC	void CLevelLocationSelector::after_search	()
{
	if (this->m_restricted_object)
		this->m_restricted_object->remove_border();
}

#undef TEMPLATE_SPECIALIZATION
#undef CLevelLocationSelector
