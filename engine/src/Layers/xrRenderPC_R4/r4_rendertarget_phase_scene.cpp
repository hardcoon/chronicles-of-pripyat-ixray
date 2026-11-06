// COP_ENGINE_PATCH: COP-ENG-138 - only puddle metadata; native world/HUD velocity stays untouched.
#include "stdafx.h"
#include "../xrRender/SSSSettings.h"

void CRenderTarget::phase_scene_forward() {
	u_setrt(rt_Generic_0, 0, 0, RDepth); // LDR RT
}

// startup
void	CRenderTarget::phase_scene_prepare	()
{
	GPU_EVENT(phase_scene_prepare);

	prepare_pf_sss_data(); // COP_ENGINE_PATCH: COP-ENG-128
	//	TODO: DX10: Check if we need to set RT here.
	u_setrt((u32)RCache.get_width(), (u32)RCache.get_height(), rt_Normal->pRT, NULL, NULL, RDepth);

	FLOAT ColorRGBA[4] = { 0.5f, 0.5f, 1.0f, 1.0f };

	RContext->ClearRenderTargetView(rt_Normal->pRT, ColorRGBA);
	RContext->ClearDepthStencilView(RDepth, D3D_CLEAR_DEPTH | D3D_CLEAR_STENCIL, 1.0f, 0);

	//	Igor: for volumetric lights
	m_bHasActiveVolumetric = false;
	//	Clear later if try to draw volumetric
}

// begin
void	CRenderTarget::phase_scene_begin() {
	// Enable ANISO
	SSManager.SetMaxAnisotropy(ps_r__tf_Anisotropic);

	u_setrt(rt_Color, rt_Normal, rt_Surface, rt_Velocity, RDepth);
	if (rt_pf_sss_data) { RCache.set_RT(rt_pf_sss_data->pRT, 4); } // COP_ENGINE_PATCH: COP-ENG-128

	// Stencil - write 0x1 at pixel pos
	RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);

	RCache.set_CullMode(CULL_CCW);
	RCache.set_ColorWriteEnable();
}

void CRenderTarget::phase_scene_begin_hud()
{
	// COP_ENGINE_PATCH: COP-ENG-011 - replace only the velocity MRT during the exact HUD draw.
	if (!(ssfx_active() ? ssfx_hybrid_aa() : ps_r2_aa_type == 4) || ps_r_scale_mode >= 2)
	{
		phase_scene_begin();
		return;
	}

	SSManager.SetMaxAnisotropy(ps_r__tf_Anisotropic);
	const FLOAT clearMask[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	RContext->ClearRenderTargetView(rt_HudAAMask->pRT, clearMask);
	u_setrt(rt_Color, rt_Normal, rt_Surface, rt_HudAAMask, RDepth);
	if (rt_pf_sss_data) { RCache.set_RT(rt_pf_sss_data->pRT, 4); } // COP_ENGINE_PATCH: COP-ENG-128

	RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
	RCache.set_CullMode(CULL_CCW);
	RCache.set_ColorWriteEnable();
}

void	CRenderTarget::disable_aniso		()
{
	// Disable ANISO
	SSManager.SetMaxAnisotropy(1);
}

// end
void	CRenderTarget::phase_scene_end		()
{
	disable_aniso();
}
