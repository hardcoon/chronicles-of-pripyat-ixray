#include "stdafx.h"

#include "FHierrarhyVisual.h"
#include "SkeletonCustom.h"
#include "../../xrEngine/Fmesh.h"
#include "../../xrEngine/IRenderable.h"
#include "../../xrEngine/IGame_Persistent.h"
#include "../../xrEngine/Environment.h"

#include "FLOD.h"
#include "InteriorWeatherRange.h"
#include "ParticleGroup.h"
#include "FTreeVisual.h"

using	namespace R_dsgraph;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene graph actual insertion and sorting ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
float		r_ssaDISCARD;
float		r_ssaDONTSORT;
float		r_ssaLOD_A,			r_ssaLOD_B;
float		r_ssaGLOD_start,	r_ssaGLOD_end;
float		r_ssaHZBvsTEX;

ICF	float	CalcSSA				(float& distSQ, Fvector& C, dxRender_Visual* V)
{
	float R	= V->vis.sphere.R + 0;
	distSQ	= Device.vCameraPosition.distance_to_sqr(C)+EPS;
	return	R/distSQ;
}
ICF	float	CalcSSA				(float& distSQ, Fvector& C, float R)
{
	distSQ	= Device.vCameraPosition.distance_to_sqr(C)+EPS;
	return	R/distSQ;
}

// COP_ENGINE_PATCH: COP-ENG-008
// Beyond the user's normal visibility budget, keep large silhouettes and
// authored FLOD impostors but reject progressively smaller static visuals.
// Dynamic renderables remain on the regular budget because they are the most
// CPU-heavy part of a distant scene (bones, animation and per-object submits).
ICF bool ExperimentalFarViewRejectDynamic(float distSQ, float radius)
{
	// COP_ENGINE_PATCH: COP-ENG-012
	// Profile zero must bypass every custom reject regardless of legacy alias state.
	if ((psFarViewProfile == 0 && ps_r4_full_detail_distance_scale >= 1.f) || !psDeviceFlags.test(rsR4) || !g_pGamePersistent ||
		!g_pGamePersistent->Environment().CurrentEnv)
		return false;

	// The toggle is a one-click recommended profile: never raise a lower user
	// budget, but cap higher saved values at the requested 0.5 equivalent.
	const float detail_budget_factor = (psFarViewProfile == 0) ? ps_r4_full_detail_distance_scale : _min(psVisDistance, 0.5f);
	// COP_ENGINE_PATCH: COP-ENG-010
	// COP_ENGINE_PATCH: COP-ENG-013
	// Camera extensions must not expand the expensive full-detail radius.
	const float optimized_far_plane = _max(1.f,
		g_pGamePersistent->Environment().CurrentEnv->far_plane - PFFarViewCameraExtension(psFarViewProfile));
	const float full_detail_distance =
		optimized_far_plane * detail_budget_factor;
	const float nearest_distance = _sqrt(distSQ) - radius;
	return nearest_distance > _max(100.f, full_detail_distance);
}

ICF bool ExperimentalFarViewRejectStatic(float distSQ, float radius, float ssa, u32 visual_type)
{
	// COP_ENGINE_PATCH: COP-ENG-012
	if ((psFarViewProfile == 0 && ps_r4_full_detail_distance_scale >= 1.f) || !psDeviceFlags.test(rsR4) ||
		!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv)
		return false;

	const float far_plane = g_pGamePersistent->Environment().CurrentEnv->far_plane;
	// COP_ENGINE_PATCH: COP-ENG-010
	// COP_ENGINE_PATCH: COP-ENG-013
	const float optimized_far_plane = _max(1.f,
		far_plane - PFFarViewCameraExtension(psFarViewProfile));
	const float detail_budget_factor = (psFarViewProfile == 0) ? ps_r4_full_detail_distance_scale : _min(psVisDistance, 0.5f);
	const float full_detail_distance = optimized_far_plane * detail_budget_factor;
	const float nearest_distance = _sqrt(distSQ) - radius;

	// Fogged optimized profiles do not need any geometry behind fully opaque
	// visual weather fog, including authored LOD impostors. Bounding-sphere near
	// edges remain visible until the whole visual is hidden by fog.
	// COP_ENGINE_PATCH: COP-ENG-014
	// Match the early static cutoff to the shader-visible 0.65 fog range.
	float visual_fog_near = 0.f;
	float visual_fog_far = 0.f;
	// COP_ENGINE_PATCH: COP-ENG-017
	// COP_ENGINE_PATCH: COP-ENG-018
	if (!PFFarViewAbsoluteVisualFog(psFarViewProfile, visual_fog_near, visual_fog_far))
	{
		visual_fog_far = g_pGamePersistent->Environment().CurrentEnv->fog_far *
			PFFarViewDistanceScale(psFarViewProfile) +
			PFFarViewFogExtension(psFarViewProfile);
	}
	if (PFFarViewHasVisualFog(psFarViewProfile) && nearest_distance >=
		_min(far_plane, visual_fog_far))
		return true;

	if (visual_type == MT_LOD || nearest_distance <= _max(100.f, full_detail_distance))
		return false;

	const float transition_range = _max(1.f, far_plane - full_detail_distance);
	const float transition = clampr((nearest_distance - full_detail_distance) / transition_range, 0.f, 1.f);
	const float discard_multiplier = 1.f + transition * 11.f;
	return ssa <= r_ssaDISCARD * discard_multiplier;
}

void R_dsgraph_structure::r_dsgraph_insert_dynamic	(dxRender_Visual *pVisual, Fvector& Center)
{
	CRender&	RI			=	RImplementation;

	if (pVisual->vis.marker	==	RI.marker)	return	;
	pVisual->vis.marker		=	RI.marker			;

#if RENDER==R_R1
	if (RI.o.vis_intersect &&	(pVisual->vis.accept_frame!=Device.dwFrame))	return;
	pVisual->vis.accept_frame	=	Device.dwFrame	;
#endif

	float distSQ			;
	float SSA				=	CalcSSA		(distSQ,Center,pVisual);
	// COP_ENGINE_PATCH: COP-ENG-156 - furniture children keep their tagged owner in every pass.
	const bool furniture_range = !RI.val_bHUD && PFFurnitureWeatherRange(RI.val_pObject);
	if (!furniture_range && !RI.val_bHUD && ExperimentalFarViewRejectDynamic(distSQ, pVisual->vis.sphere.R)) return;
	if (!furniture_range && SSA<=r_ssaDISCARD)		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY						(pVisual->shader._get());
	ShaderElement*		sh_d	= &*pVisual->shader->E[4];
	if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority/2]) {
		mapSorted_T& test = RI.val_bHUD ? mapHUDDistort : mapDistort;

		mapSorted_Node* N		= test.insertInAnyWay(distSQ);

		N->val.ssa				= SSA;
		N->val.pObject			= RI.val_pObject;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= *RI.val_pTransform;
		N->val.se				= sh_d;		// 4=L_special
	}

	if (sh_d && sh_d->flags.bScopeMask && pmask[0]) {
		mapHUDScopeMask.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh_d });
	}

	// Select shader
	ShaderElement*	sh		=	RImplementation.rimp_select_sh_dynamic	(pVisual,distSQ);
	if (0==sh)								return;
	if (!pmask[sh->flags.iPriority/2])		return;

	// Create common node
	// NOTE: Invisible elements exist only in R1

	// HUD rendering
	if (RI.val_bHUD)			
	{
		if (sh->flags.bStrictB2F)	
		{
#if RENDER==R_R4
			// COP_ENGINE_PATCH: COP-ENG-216 - defer the screen, keeping hands/body in native AA.
			const char* pixelShader = !sh->passes.empty() && sh->passes[0]->ps ? sh->passes[0]->ps->cName.c_str() : nullptr;
			// Compiled shader resources may append an underscore and an options CRC.
			if (pixelShader && strncmp(pixelShader, "pf_pda_screen", 13) == 0 &&
				(pixelShader[13] == '\0' || pixelShader[13] == '_'))
			{
				mapHUDPda.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh });
				return;
			}
#endif
			mapHUDSorted.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh });
			return;
		} 
		else 
		{
			mapHUD.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh });
#if RENDER!=R_R1
			if (sh->flags.bEmissive) 
				mapHUDEmissive.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh_d });
#endif	//	RENDER!=R_R1
			return;
		}
	}

	// Shadows registering
#if RENDER==R_R1
	_MatrixItem		item = { SSA,RI.val_pObject,pVisual,*RI.val_pTransform };
	RI.L_Shadows->add_element	(item);
#endif
	if (RI.val_bInvisible)		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F && !pVisual->dcast_ParticleCustom())
	{
		mapSorted.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh });
		return;
	}

#if RENDER!=R_R1
	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
		mapEmissive.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh_d });

	if (sh->flags.bWmark && pmask[2])
	{
		mapWmark.insertInAnyWay(distSQ, { SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh });						
		return;
	}
#endif

	for ( u32 iPass = 0; iPass<sh->passes.size(); ++iPass)
	{
		// the most common node
		if (sh->passes[iPass] == nullptr)
		{
			continue;
		}

		//SPass&						pass	= *sh->passes.front	();
		//mapMatrix_T&				map		= mapMatrix			[sh->flags.iPriority/2];
		SPass&						pass	= *sh->passes[iPass];
		mapMatrix_T&				map		= mapMatrixPasses	[sh->flags.iPriority/2][iPass];
		

#ifdef USE_RESOURCE_DEBUGGER
	#ifdef USE_DX11
		mapMatrixVS::TNode*			Nvs		= map.insert		(pass.vs);
		mapMatrixGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs);
		mapMatrixPS::TNode*			Nps		= Ngs->val.insert	(pass.ps);
	#else //USE_DX11
		mapMatrixVS::TNode*			Nvs		= map.insert		(pass.vs);
		mapMatrixPS::TNode*			Nps		= Nvs->val.insert	(pass.ps);
	#endif
#else
	#ifdef USE_DX11
		mapMatrixVS::TNode*			Nvs		= map.insert		(&*pass.vs);
		mapMatrixGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs->gs);
		mapMatrixPS::TNode*			Nps		= Ngs->val.insert	(pass.ps->ps);
	#else //USE_DX11
		mapMatrixVS::TNode*			Nvs		= map.insert		(pass.vs->vs);
		mapMatrixPS::TNode*			Nps		= Nvs->val.insert	(pass.ps->ps);
	#endif
#endif

#ifdef USE_DX11
#	ifdef USE_RESOURCE_DEBUGGER
		Nps->val.hs = pass.hs;
		Nps->val.ds = pass.ds;
		mapMatrixCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#	else
		Nps->val.hs = pass.hs->sh;
		Nps->val.ds = pass.ds->sh;
		mapMatrixCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#	endif
#else
		mapMatrixCS::TNode*			Ncs		= Nps->val.insert	(pass.constants._get());
#endif
		mapMatrixStates::TNode*		Nstate	= Ncs->val.insert	(pass.state->state);
		mapMatrixTextures::TNode*	Ntex	= Nstate->val.insert(pass.T._get());
#if RENDER==R_R1
		Ntex->val.push_back(item);
#else
		Ntex->val.push_back({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform });
#endif

	}
}

void R_dsgraph_structure::r_dsgraph_insert_static	(dxRender_Visual *pVisual)
{
	CRender&	RI				=	RImplementation;

	if (pVisual->vis.marker		==	RI.marker)	return	;
	pVisual->vis.marker			=	RI.marker			;

#if RENDER==R_R1
	if (RI.o.vis_intersect &&	(pVisual->vis.accept_frame!=Device.dwFrame))	return;
	pVisual->vis.accept_frame	=	Device.dwFrame		;
#endif

	float distSQ;
	float SSA					=	CalcSSA		(distSQ,pVisual->vis.sphere.P,pVisual);
	if (ExperimentalFarViewRejectStatic(distSQ, pVisual->vis.sphere.R, SSA, pVisual->Type)) return;
	if (SSA<=r_ssaDISCARD)		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY						(pVisual->shader._get());
	ShaderElement*		sh_d	= &*pVisual->shader->E[4];
	if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority/2])
		mapDistort.insertInAnyWay(distSQ, { SSA, nullptr, pVisual, Fidentity, sh_d });

	// Select shader
	ShaderElement*		sh		= RImplementation.rimp_select_sh_static(pVisual,distSQ);
	if (0==sh)								return;
	if (!pmask[sh->flags.iPriority/2])		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted.insertInAnyWay(distSQ, { SSA, nullptr, pVisual, Fidentity, sh });
		return;
	}

#if RENDER!=R_R1
	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
		mapEmissive.insertInAnyWay(distSQ, { SSA, nullptr, pVisual, Fidentity, sh_d });

	if (sh->flags.bWmark && pmask[2])
	{
		mapWmark.insertInAnyWay(distSQ, {SSA, nullptr, pVisual, Fidentity, sh});						
		return;
	}
#endif

	if	(val_feedback && counter_S==val_feedback_breakp)	val_feedback->rfeedback_static(pVisual);
	counter_S					++;

	for ( u32 iPass = 0; iPass<sh->passes.size(); ++iPass)
	{
		//SPass&						pass	= *sh->passes.front	();
		//mapNormal_T&				map		= mapNormal			[sh->flags.iPriority/2];
		SPass&						pass	= *sh->passes[iPass];
		mapNormal_T&				map		= mapNormalPasses[sh->flags.iPriority/2][iPass];

#ifdef USE_RESOURCE_DEBUGGER
#ifdef USE_DX11
		mapNormalVS::TNode*			Nvs		= map.insert		(pass.vs);
		mapNormalGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs);
		mapNormalPS::TNode*			Nps		= Ngs->val.insert	(pass.ps);
#	else //USE_DX11
		mapNormalVS::TNode*			Nvs		= map.insert		(pass.vs);
		mapNormalPS::TNode*			Nps		= Nvs->val.insert	(pass.ps);
#	endif
#else // USE_RESOURCE_DEBUGGER
#ifdef USE_DX11
		mapNormalVS::TNode*			Nvs		= map.insert		(&*pass.vs);
		mapNormalGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs->gs);
		mapNormalPS::TNode*			Nps		= Ngs->val.insert	(pass.ps->ps);
#	else //USE_DX11
		mapNormalVS::TNode*			Nvs		= map.insert		(pass.vs->vs);
		mapNormalPS::TNode*			Nps		= Nvs->val.insert	(pass.ps->ps);
#	endif
#endif // USE_RESOURCE_DEBUGGER

#ifdef USE_DX11
#	ifdef USE_RESOURCE_DEBUGGER
		Nps->val.hs = pass.hs;
		Nps->val.ds = pass.ds;
		mapNormalCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#	else
		Nps->val.hs = pass.hs->sh;
		Nps->val.ds = pass.ds->sh;
		mapNormalCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#	endif
#else
		mapNormalCS::TNode*			Ncs		= Nps->val.insert	(pass.constants._get());
#endif
		mapNormalStates::TNode*		Nstate	= Ncs->val.insert	(pass.state->state);
		mapNormalTextures::TNode*	Ntex	= Nstate->val.insert(pass.T._get());

		Ntex->val.push_back({SSA,pVisual});
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRender::add_leafs_Dynamic	(dxRender_Visual *pVisual)
{
	//PROF_EVENT("add_leafs_Dynamic")
	if (0==pVisual)				return;

	// Visual is 100% visible - simply add it
	xr_vector<dxRender_Visual*>::iterator I,E;	// it may be useful for 'hierrarhy' visual

	switch (pVisual->Type) {
	case MT_PARTICLE_GROUP:
		{
			// Add all children, doesn't perform any tests
			PS::CParticleGroup* pG	= (PS::CParticleGroup*)pVisual;
			xrCriticalSectionGuard guard(&pG->onframe_lock);
			for (PS::CParticleGroup::SItemVecIt i_it=pG->items.begin(); i_it!=pG->items.end(); i_it++)	{
				PS::CParticleGroup::SItem&			I_		= *i_it;
				if (I_._effect)		add_leafs_Dynamic		(I_._effect);
				for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_related.begin();	pit!=I_._children_related.end(); pit++)	add_leafs_Dynamic(*pit);
				for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_free.begin();		pit!=I_._children_free.end();	pit++)	add_leafs_Dynamic(*pit);
			}
		}
		return;
	case MT_HIERRARHY:
		{
			// Add all children, doesn't perform any tests
			FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
			I = pV->children.begin	();
			E = pV->children.end	();
			for (; I!=E; I++)	add_leafs_Dynamic	(*I);
		}
		return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
		{
			// Add all children, doesn't perform any tests
			CKinematics * pV			= (CKinematics*)pVisual;
			BOOL	_use_lod			= FALSE	;
			// COP_ENGINE_PATCH: COP-ENG-156 - preserve the full furniture model.
			if (pV->m_lod && !PFFurnitureWeatherRange(val_pObject))
			{
				Fvector							Tpos;	float		D;
				val_pTransform->transform_tiny	(Tpos, pV->vis.sphere.P);
				float		ssa		=	CalcSSA	(D,Tpos,pV->vis.sphere.R/2.f);	// assume dynamics never consume full sphere
				if (ssa<r_ssaLOD_A)	_use_lod	= TRUE;
			}
			if (_use_lod)				
			{
				add_leafs_Dynamic			(pV->m_lod)		;
			} else {
#if RENDER==R_R1
				pV->CalculateBones			(TRUE);
				pV->CalculateWallmarks		();		//. bug?
#endif
				I = pV->children.begin		();
				E = pV->children.end		();
				for (; I!=E; I++)	add_leafs_Dynamic	(*I);
			}
		}
		return;
	default:
		{
			// General type of visual
			// Calculate distance to it's center
			Fvector							Tpos;
			val_pTransform->transform_tiny	(Tpos, pVisual->vis.sphere.P);
			r_dsgraph_insert_dynamic		(pVisual,Tpos);
		}
		return;
	}
}

void CRender::add_leafs_Static(dxRender_Visual *pVisual)
{
	//PROF_EVENT("add_leafs_Static")
	// COP_ENGINE_PATCH: COP-ENG-008
	// Prune small distant hierarchy branches before HOM and child traversal;
	// the leaf insertion check remains as a conservative final guard.
	float far_view_distSQ;
	const float far_view_ssa = CalcSSA(far_view_distSQ, pVisual->vis.sphere.P, pVisual);
	if (ExperimentalFarViewRejectStatic(
		far_view_distSQ, pVisual->vis.sphere.R, far_view_ssa, pVisual->Type))
		return;
#if RENDER!=R_R1
	if(RImplementation.phase==CRender::PHASE_NORMAL)
#endif
	if (!HOM.visible(pVisual->vis))		return;

	// Visual is 100% visible - simply add it
	xr_vector<dxRender_Visual*>::iterator I,E;	// it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type) {
	case MT_PARTICLE_GROUP:
		{
			// Add all children, doesn't perform any tests
			PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
			for (PS::CParticleGroup::SItemVecIt i_it=pG->items.begin(); i_it!=pG->items.end(); i_it++){
				PS::CParticleGroup::SItem&			I_		= *i_it;
				if (I_._effect)		add_leafs_Dynamic		(I_._effect);
				for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_related.begin();	pit!=I_._children_related.end(); pit++)	add_leafs_Dynamic(*pit);
				for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_free.begin();		pit!=I_._children_free.end();	pit++)	add_leafs_Dynamic(*pit);
			}
		}
		return;
	case MT_HIERRARHY:
		{
			// Add all children, doesn't perform any tests
			FHierrarhyVisual* pV	= (FHierrarhyVisual*)pVisual;
			I = pV->children.begin	();
			E = pV->children.end	();
			for (; I!=E; I++)		add_leafs_Static (*I);
		}
		return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
		{
			// Add all children, doesn't perform any tests
			CKinematics * pV		= (CKinematics*)pVisual;
#if RENDER==R_R1
			pV->CalculateBones		(TRUE);
#endif
			I = pV->children.begin	();
			E = pV->children.end	();
			for (; I!=E; I++)		add_leafs_Static	(*I);
		}
		return;
	case MT_LOD:
		{
			FLOD		* pV	=		(FLOD*) pVisual;
			float		D;
			float		ssa		=		CalcSSA(D,pV->vis.sphere.P,pV);
			ssa					*=		pV->lod_factor;
			if (ssa<r_ssaLOD_A)
			{
				if (ssa<r_ssaDISCARD)	return;
				mapLOD_Node*	N	=	mapLOD.insertInAnyWay(D);
				N->val.ssa			=	ssa;
				N->val.pVisual		=	pVisual;
			}
#if RENDER!=R_R1
			if (ssa>r_ssaLOD_B || phase==PHASE_SMAP)
#else
			if (ssa>r_ssaLOD_B)
#endif
			{
				// Add all children, doesn't perform any tests
				I = pV->children.begin	();
				E = pV->children.end	();
				for (; I!=E; I++)	add_leafs_Static (*I);
			}
		}
		return;
	case MT_TREE_PM:
	case MT_TREE_ST:
		{
			// General type of visual
			r_dsgraph_insert_static		(pVisual);
		}
		return;
	default:
		{
			// General type of visual
			r_dsgraph_insert_static		(pVisual);
		}
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRender::add_Dynamic(dxRender_Visual *pVisual, u32 planes)
{
	//PROF_EVENT("add_Dynamic")

	// Check frustum visibility and calculate distance to visual's center
	Fvector		Tpos;	// transformed position
	EFC_Visible	VIS;

	val_pTransform->transform_tiny	(Tpos, pVisual->vis.sphere.P);
	VIS = View->testSphere			(Tpos, pVisual->vis.sphere.R,planes);
	if (fcvNone==VIS) return FALSE	;

	// If we get here visual is visible or partially visible
	xr_vector<dxRender_Visual*>::iterator I,E;	// it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type) {
	case MT_PARTICLE_GROUP:
		{
			// Add all children, doesn't perform any tests
			PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
			for (PS::CParticleGroup::SItemVecIt i_it=pG->items.begin(); i_it!=pG->items.end(); i_it++)
			{
				PS::CParticleGroup::SItem&			I_		= *i_it;
				if (fcvPartial==VIS) 
				{
					if (I_._effect)		add_Dynamic				(I_._effect,planes);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_related.begin();	pit!=I_._children_related.end(); pit++)	add_Dynamic(*pit,planes);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_free.begin();		pit!=I_._children_free.end();	pit++)	add_Dynamic(*pit,planes);
				} else 
				{
					if (I_._effect)		add_leafs_Dynamic		(I_._effect);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_related.begin();	pit!=I_._children_related.end(); pit++)	add_leafs_Dynamic(*pit);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_free.begin();		pit!=I_._children_free.end();	pit++)	add_leafs_Dynamic(*pit);
				}
			}
		}
		break;
	case MT_HIERRARHY:
		{
			// Add all children
			FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
			I = pV->children.begin	();
			E = pV->children.end	();
			if (fcvPartial==VIS) 
			{
				for (; I!=E; I++)	add_Dynamic			(*I,planes);
			} else {
				for (; I!=E; I++)	add_leafs_Dynamic	(*I);
			}
		}
		break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
		{
			// Add all children, doesn't perform any tests
			CKinematics * pV			= (CKinematics*)pVisual;
			BOOL	_use_lod			= FALSE	;
			// COP_ENGINE_PATCH: COP-ENG-156
			if (pV->m_lod && !PFFurnitureWeatherRange(val_pObject))
			{
				Fvector							Tpos_;	float		D;
				val_pTransform->transform_tiny	(Tpos_, pV->vis.sphere.P);
				float		ssa		=	CalcSSA	(D,Tpos_,pV->vis.sphere.R/2.f);	// assume dynamics never consume full sphere
				if (ssa<r_ssaLOD_A)	_use_lod	= TRUE		;
			}
			if (_use_lod)
			{
				add_leafs_Dynamic			(pV->m_lod)		;
			} else 
			{
#if RENDER==R_R1
				pV->CalculateBones			(TRUE);
				pV->CalculateWallmarks		();		//. bug?
#endif
				I = pV->children.begin		();
				E = pV->children.end		();
				for (; I!=E; I++)	add_leafs_Dynamic	(*I);
			}
			/*
			I = pV->children.begin		();
			E = pV->children.end		();
			if (fcvPartial==VIS) {
				for (; I!=E; I++)	add_Dynamic			(*I,planes);
			} else {
				for (; I!=E; I++)	add_leafs_Dynamic	(*I);
			}
			*/
		}
		break;
	default:
		{
			// General type of visual
			r_dsgraph_insert_dynamic(pVisual,Tpos);
		}
		break;
	}
	return TRUE;
}

void CRender::add_Static(dxRender_Visual *pVisual, u32 planes)
{
	//PROF_EVENT("add_Static")

	// Check frustum visibility and calculate distance to visual's center
	EFC_Visible	VIS;
	vis_data&	vis			= pVisual->vis;
	VIS = View->testSAABB	(vis.sphere.P,vis.sphere.R,vis.box.data(),planes);
	if (fcvNone==VIS)		
		return;
	// COP_ENGINE_PATCH: COP-ENG-008
	// Avoid HOM queries and recursive child traversal for hierarchy branches
	// that the experimental far-view budget will discard in any case.
	float far_view_distSQ;
	const float far_view_ssa = CalcSSA(far_view_distSQ, vis.sphere.P, pVisual);
	if (ExperimentalFarViewRejectStatic(
		far_view_distSQ, vis.sphere.R, far_view_ssa, pVisual->Type))
		return;
#if RENDER!=R_R1
	if(RImplementation.phase==CRender::PHASE_NORMAL)
#endif
	if (!HOM.visible(vis))	
		return;

	// If we get here visual is visible or partially visible
	xr_vector<dxRender_Visual*>::iterator I,E;	// it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type) {
	case MT_PARTICLE_GROUP:
		{
			// Add all children, doesn't perform any tests
			PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
			for (PS::CParticleGroup::SItemVecIt i_it=pG->items.begin(); i_it!=pG->items.end(); i_it++){
				PS::CParticleGroup::SItem&			I_		= *i_it;
				if (fcvPartial==VIS) {
					if (I_._effect)		add_Dynamic				(I_._effect,planes);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_related.begin();	pit!=I_._children_related.end(); pit++)	add_Dynamic(*pit,planes);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_free.begin();		pit!=I_._children_free.end();	pit++)	add_Dynamic(*pit,planes);
				} else {
					if (I_._effect)		add_leafs_Dynamic		(I_._effect);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_related.begin();	pit!=I_._children_related.end(); pit++)	add_leafs_Dynamic(*pit);
					for (xr_vector<dxRender_Visual*>::iterator pit = I_._children_free.begin();		pit!=I_._children_free.end();	pit++)	add_leafs_Dynamic(*pit);
				}
			}
		}
		break;
	case MT_HIERRARHY:
		{
			// Add all children
			FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
			I = pV->children.begin	();
			E = pV->children.end		();
			if (fcvPartial==VIS) {
				for (; I!=E; I++)	add_Static			(*I,planes);
			} else {
				for (; I!=E; I++)	add_leafs_Static	(*I);
			}
		}
		break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
		{
			// Add all children, doesn't perform any tests
			CKinematics * pV		= (CKinematics*)pVisual;
#if RENDER==R_R1
			pV->CalculateBones		(TRUE);
#endif
			I = pV->children.begin	();
			E = pV->children.end	();
			if (fcvPartial==VIS) {
				for (; I!=E; I++)	add_Static			(*I,planes);
			} else {
				for (; I!=E; I++)	add_leafs_Static	(*I);
			}
		}
		break;
	case MT_LOD:
		{
			FLOD		* pV	= (FLOD*) pVisual;
			float		D;
			float		ssa		= CalcSSA	(D,pV->vis.sphere.P,pV);
			ssa					*= pV->lod_factor;
			if (ssa<r_ssaLOD_A)	
			{
				if (ssa<r_ssaDISCARD)	return;
				mapLOD_Node*	N		= mapLOD.insertInAnyWay(D);
				N->val.ssa				= ssa;
				N->val.pVisual			= pVisual;
			}
#if RENDER!=R_R1
			if (ssa>r_ssaLOD_B || phase==PHASE_SMAP)
#else
			if (ssa>r_ssaLOD_B)
#endif
			{
				// Add all children, perform tests
				I = pV->children.begin	();
				E = pV->children.end	();
				for (; I!=E; I++)	add_leafs_Static	(*I);
			}
		}
		break;
	case MT_TREE_ST:
	case MT_TREE_PM:
		{
			// General type of visual
			r_dsgraph_insert_static		(pVisual);
		}
		return;
	default:
		{
			// General type of visual
			r_dsgraph_insert_static		(pVisual);
		}
		break;
	}
}
