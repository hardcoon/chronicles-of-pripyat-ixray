#pragma once

#include "../xrRender/ColorMapManager.h"
#include "../xrRender/ScreenPostProcessTypes.h"
#include "r4_dlss_history.h"
#include "r4_dlss_activation.h"

class light;
class light_Package;

//#define DU_SPHERE_NUMVERTEX 92
//#define DU_SPHERE_NUMFACES	180
//#define DU_CONE_NUMVERTEX	18
//#define DU_CONE_NUMFACES	32
//	no less than 2
#define	VOLUMETRIC_SLICES	100

class CRenderTarget		: public IRender_Target
{
private:
	u32							dwWidth;
	u32							dwHeight;
	u32							dwAccumulatorClearMark;
public:
	enum	eStencilOptimizeMode
	{
		SO_Light = 0,	//	Default
		SO_Combine,		//	Default
	};

	u32							dwLightMarkerID;
	// 
	IBlender*					b_occq;
	IBlender*					b_accum_mask;
	IBlender*					b_accum_direct;
	IBlender*					b_accum_point;
	IBlender*					b_accum_spot;
	IBlender*					b_accum_reflected;
	IBlender*					b_bloom;
	IBlender*					b_luminance;
	IBlender*					b_combine;
	IBlender*					b_ssao;

	IBlender*					b_fxaa;
	IBlender*					b_spp;
	IBlender*					b_smaa;
	IBlender*					b_scale;
	IBlender*					b_cas;
	IBlender*					b_gtao;
    // COP_ENGINE_PATCH: COP-ENG-126 - addon resources are allocated lazily and released when disabled.
    // COP_ENGINE_PATCH: COP-ENG-128 - accurate material classification and HDR reflection passes.
    ref_rt rt_pf_sss_data;
    ref_rt rt_pf_sss_scene, rt_pf_sss_velocity;
    ref_rt rt_pf_sss_water_color,rt_pf_sss_water_filter,rt_pf_sss_water_tmp;
    ref_shader s_pf_sss_water_down,s_pf_sss_water_filter,s_pf_sss_water_tmp;
    ref_rt rt_pf_sss_volume_point,rt_pf_sss_volume_tmp;
    ref_shader s_pf_sss_volume_point,s_pf_sss_volume_tmp;
    u32 pf_sss_volume_frame=u32(-1);
    void reset_pf_sss_volume();
    void prepare_pf_sss_volume();
    void accum_pf_sss_volume();
    void combine_pf_sss_volume();
    ref_shader s_pf_sss_postfx, s_pf_sss_bloom_down[5], s_pf_sss_bloom_up[4], s_pf_sss_bloom_finish;
    ref_rt rt_pf_sss_emissive;
    u32 pf_sss_emissive_frame=u32(-1);
    void begin_pf_sss_emissive();void end_pf_sss_emissive();
    ref_rt rt_pf_sss_post_resolve, rt_pf_sss_post_previous, rt_pf_sss_post_composite, rt_pf_sss_post_blur;
    ref_rt rt_pf_sss_bloom_down[5], rt_pf_sss_bloom_up[4], rt_pf_sss_bloom_lens, rt_pf_sss_bloom_final;
    bool pf_sss_post_history = false;
    u32 pf_sss_post_frame = 0, pf_sss_post_revision = 0;
    Fvector pf_sss_post_camera = {}, pf_sss_post_direction = {};
    float pf_sss_post_fov = 0.f, pf_sss_dof_far = 0.f;
    bool pf_sss_post_svp = false;
    Fvector4 pf_sss_post_context = {};
    ref_shader s_pf_sss_reflections;
    ref_rt rt_pf_sss_ssr, rt_pf_sss_ssr_previous, rt_pf_sss_ssr_filter, rt_pf_sss_composite;
    ref_rt rt_pf_sss_il, rt_pf_sss_il_previous, rt_pf_sss_il_filter;
    bool pf_sss_reflection_history = false;
    u32 pf_sss_reflection_frame = 0, pf_sss_reflection_revision = 0;
    Fvector pf_sss_reflection_camera = {},pf_sss_reflection_direction={};
    float pf_sss_reflection_fov=0;bool pf_sss_reflection_svp=false;
    ref_shader s_pf_sss_sun;
    ref_shader s_pf_sss_local[2];ref_rt rt_pf_sss_local[2],rt_pf_sss_local_previous[2],rt_pf_sss_local_filter[2];
    light* pf_sss_local_ids[8]={};u32 pf_sss_local_frame=u32(-1);bool pf_sss_sun_history=false;
    void phase_pf_sss_local(light_Package& normal,light_Package& pending);
    void finish_pf_sss_sun();
    ref_rt rt_pf_sss_sun_current,rt_pf_sss_sun_previous,rt_pf_sss_sun_filter;
    u32 pf_sss_sun_frame=u32(-1),pf_sss_sun_revision=0;
    Fvector pf_sss_sun_camera={},pf_sss_sun_direction={},pf_sss_sun_light={};
    float pf_sss_sun_fov=0;bool pf_sss_sun_svp=false;
    void phase_pf_sss_sun();void reset_pf_sss_sun();
    ref_shader s_pf_sss;
    ref_rt rt_pf_sss_current, rt_pf_sss_previous;
    bool pf_sss_history_valid = false;
    u32 pf_sss_last_frame = 0;
    Fvector pf_sss_last_camera = {},pf_sss_last_direction={};
    float pf_sss_last_fov=0;bool pf_sss_last_svp=false;
    Fvector4 pf_sss_last_ao = {};
    Fvector4 pf_sss_last_ao2 = {};
    u32 pf_sss_last_revision = 0;
    // COP_ENGINE_PATCH: COP-ENG-212 - live PDA interface.
    ref_rt rt_pf_pda;
	IBlender*					b_taa;
	IBlender*					b_gamma;

#ifdef DEBUG
	struct		dbg_line_t		{
		Fvector	P0,P1;
		u32		color;
	};
	xr_vector<std::pair<Fsphere,Fcolor> >		dbg_spheres;
	xr_vector<dbg_line_t>										dbg_lines;
	xr_vector<Fplane>												dbg_planes;
#endif

	// MRT-path
	ref_rt						rt_Generic;
	ref_rt						rt_Back_Buffer;
	ref_rt						rt_Back_Buffer_AA;
	ref_rt						rt_Position;		// 64bit,	fat	(x,y,z,?)				(eye-space)
	ref_rt						rt_Normal;			// 64bit,	fat	(x,y,z,hemi)			(eye-space)
	ref_rt						rt_NormalTemp;		// 64bit,	fat	(x,y,z,hemi)			(eye-space)
	ref_rt						rt_Color;			// 64/32bit,fat	(r,g,b,specular-gloss)	(or decompressed MET-8-8-8-8)
	ref_rt						rt_Surface;
	ref_rt						rt_SurfaceTemp;
	ref_rt						rt_Velocity;

	ref_rtc						rt_Reflection;
	ref_rtc						rt_Reflection_temp;
	ref_rt						rt_Depth;

	// 
	ref_rt						rt_Accumulator;		// 64bit		(r,g,b,specular)
	ref_rt						rt_Generic_0;		// 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	ref_rt						rt_Generic_1;		// 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	//	Igor: for volumetric lights
	ref_rt						rt_Generic_2;		// 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	ref_rt						rt_BackbufferLUT;		// 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	ref_rt						rt_Bloom_1;			// 32bit, dim/4	(r,g,b,?)
	ref_rt						rt_Bloom_2;			// 32bit, dim/4	(r,g,b,?)
	ref_rt						rt_LUM_64;			// 64bit, 64x64,	log-average in all components
	ref_rt						rt_LUM_8;			// 64bit, 8x8,		log-average in all components

	ref_rt						rt_LUM_pool[2]	;	// 1xfp32,1x1,		exp-result -> scaler
	ref_texture				t_LUM_src		;	// source
	ref_texture				t_LUM_dest		;	// destination & usage for current frame

	// env
	ref_texture				t_envmap_0		;	// env-0
	ref_texture				t_envmap_1		;	// env-1

	//GTAO
	ref_rt						rt_gtao_0;

	//TAA
	ref_rt					rt_Generic_0_prev; //previous frame for taa
	ref_rt					rt_HudAAMask; // exact HUD coverage; COP_ENGINE_PATCH: COP-ENG-011

	// smap
	ref_rt						rt_smap_surf;	// 32bit,		color
	ref_rt						rt_smap_depth;	// 24(32) bit,	depth 
//	TODO: DX10: CHeck if we need old-style SMAP
//	IDirect3DSurface9*			rt_smap_ZB;		//

	//	Igor: for async screenshots
	ID3DTexture2D*			t_ss_async;				//32bit		(r,g,b,a) is situated in the system memory

	// Textures
	ID3DTexture3D*			t_material_surf;
	ref_texture					t_material;

	ID3DTexture2D*			t_noise_surf	[TEX_jitter_count];
	ref_texture					t_noise				[TEX_jitter_count];
private:
	ref_shader					s_spp;
	ref_shader					s_fxaa;
	ref_shader					s_smaa;
	ref_shader					s_scale;
	ref_shader					s_cas;
	ref_shader					s_gtao;
	ref_shader					s_puddles;
	ref_shader					s_taa;

	// For gamma correction in windowed mode
	ref_shader					s_gamma;

	// OCCq
	ref_shader					s_occq;

	// SMAA
	ref_rt						rt_smaa_edgetex;
	ref_rt						rt_smaa_blendtex;
	bool						m_hud_taa_history_valid = false;
	u32						m_hud_taa_history_frame = 0;

	ref_rt						rt_sslr;
	ref_rt						rt_sslr_temp;
	ref_rt						rt_sslr_old;
	ref_rt						rt_sslr_data;

	// SSAO
	ref_rt						rt_ssao_temp;
	ref_rt						rt_half_depth;
	ref_shader					s_ssao;
	ref_shader					s_hdao_cs;

	// Accum
	ref_shader					s_accum_mask	;
	ref_shader					s_accum_direct	;
	ref_shader					s_accum_direct_volumetric;
	ref_shader					s_accum_point	;
	ref_shader					s_accum_spot	;
	ref_shader					s_accum_reflected;
	ref_shader					s_accum_volume;

	//	DX10 Rain
	ref_shader					s_rain;
	ref_geom						g_accum_point	;
	ref_geom						g_accum_spot	;
	ref_geom						g_accum_omnipart;
	ref_geom						g_accum_volumetric;

	ID3DVertexBuffer*		g_accum_point_vb;
	ID3DIndexBuffer*		g_accum_point_ib;

	ID3DVertexBuffer*		g_accum_omnip_vb;
	ID3DIndexBuffer*		g_accum_omnip_ib;

	ID3DVertexBuffer*		g_accum_spot_vb	;
	ID3DIndexBuffer*		g_accum_spot_ib	;

	ID3DVertexBuffer*		g_accum_volumetric_vb;
	ID3DIndexBuffer*		g_accum_volumetric_ib;

	ID3DBlendState*			g_debug_blend_state = nullptr;

	// Bloom
	ref_geom					g_bloom_build;
	ref_geom					g_bloom_filter;
	ref_shader				s_bloom_dbg_1;
	ref_shader				s_bloom_dbg_2;
	ref_shader				s_bloom;
	float							f_bloom_factor;

	// Luminance
	ref_shader			s_luminance;
	float						f_luminance_adapt;

	// Combine
	ref_geom					g_combine;
	ref_geom					g_combine_VP;		// xy=p,zw=tc
	ref_geom					g_combine_2UV;
	ref_geom					g_combine_cuboid;
	ref_geom					g_aa_blur;
	ref_geom					g_aa_AA;
	ref_shader				s_combine_dbg_0;
	ref_shader				s_combine_dbg_1;
	ref_shader				s_combine_dbg_Accumulator;
	ref_shader				s_combine;
	ref_shader				s_combine_volumetric;
public:
	ref_shader				s_postprocess;
	ref_geom					g_postprocess;
	ref_shader				s_menu;
	ref_geom					g_menu;
private:
	float						im_noise_time;
	u32							im_noise_shift_w;
	u32							im_noise_shift_h;
	
	float						param_blur;
	float						param_gray;
	float						param_duality_h;
	float						param_duality_v;
	float						param_noise;
	float						param_noise_scale;
	float						param_noise_fps;
	u32							param_color_base;
	u32							param_color_gray;
	Fvector						param_color_add;

	//	Color mapping
	float						param_color_map_influence;
	float						param_color_map_interpolate;
	ColorMapManager				color_map_manager;

	//	Igor: used for volumetric lights
	bool						m_bHasActiveVolumetric;
public:
								CRenderTarget			();
								~CRenderTarget			();
	void						accum_point_geom_create	();
	void						accum_point_geom_destroy();
	void						accum_omnip_geom_create	();
	void						accum_omnip_geom_destroy();
	void						accum_spot_geom_create	();
	void						accum_spot_geom_destroy	();
	//	Igor: used for volumetric lights
	void						accum_volumetric_geom_create();
	void						accum_volumetric_geom_destroy();

	void						u_stencil_optimize		(eStencilOptimizeMode eSOM = SO_Light);
	void						u_compute_texgen_screen	(Fmatrix&	dest);
	void						u_compute_texgen_jitter	(Fmatrix&	dest);
	void						u_setrt					(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4, ID3DDepthStencilView* zb);
	void						u_setrt					(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, ID3DDepthStencilView* zb);
	void						u_setrt					(const ref_rt& _1, const ref_rt& _2, ID3DDepthStencilView* zb);
	void						u_setrt					(u32 W, u32 H, ID3DRenderTargetView* _1, ID3DRenderTargetView* _2, ID3DRenderTargetView* _3, ID3DDepthStencilView* zb);
	void						u_calc_tc_noise			(Fvector2& p0, Fvector2& p1);
	void						u_calc_tc_duality_ss	(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);
	BOOL						u_need_PP				();
	bool						u_need_CM				();
	BOOL						u_DBT_enable			(float zMin, float zMax);
	void						u_DBT_disable			();

	void						init_fsr				();
	bool						phase_fsr				();
	void						init_xess();
	bool						phase_xess();
    // COP_ENGINE_PATCH: COP-ENG-163 - reset explicitly at level and SDK boundaries.
    DLSSHistoryState dlss_history;
    dlss_activation::SceneGate dlss_scene_gate; // COP_ENGINE_PATCH: COP-ENG-177
    void reset_dlss_history() { dlss_history.reset(); }
	void init_dlss(bool createNow = false); // COP_ENGINE_PATCH: COP-ENG-177
	bool						phase_dlss();
	void						phase_scale(u32 filter = u32(-1));

	void						phase_fxaa				();
	void						phase_smaa				();
	void						phase_taa				();

	void						phase_cas				();
	void						phase_gtao				();
    bool phase_pf_sss_ao();
    void reset_pf_sss();
    void prepare_pf_sss_data();
    void phase_pf_sss_reflections();
    void reset_pf_sss_postfx();
    void prepare_pf_sss_postfx();
    void phase_pf_sss_postfx(bool temporal);
    void phase_pf_sss_bloom();
    void draw_pf_sss_postfx(const ref_rt& output, const ref_shader& shader, u32 element, const Fvector4& setup);
    void snapshot_pf_sss_scene();
    void reset_pf_sss_water();
    void prepare_pf_sss_water();
    void reset_pf_sss_reflections();
	void						phase_sslr				();

	void						phase_puddles			();

	void						RenderEffect			(ScreenPostProcessType postProcessType, bool = true);
	void						PhaseAberration			();
	void						PhaseVignette			();
	void						PhaseSaturation			();
	void						PhaseWinter				();

	void						phase_scene_forward		();
	void						phase_scene_prepare		();

	void						phase_scene_begin		();
	void						phase_scene_begin_hud	(); // COP_ENGINE_PATCH: COP-ENG-011
	void						phase_scene_end			();
	void						phase_occq				();
	void						phase_ssao				();
	void						phase_downsamp			();
	void						phase_wallmarks			();
	void						phase_smap_direct		(light* L,	u32 sub_phase);
	void						phase_smap_direct_tsh	(light* L,	u32 sub_phase);
	void						phase_smap_spot_clear	();
	void						phase_smap_spot			(light* L);
	void						phase_smap_spot_tsh		(light* L);
	void						phase_accumulator		();
	void						phase_vol_accumulator	();
	void						shadow_direct			(light* L, u32 dls_phase);

	void						phase_rain				();
	void						draw_rain				(light &RainSetup);

	bool						need_to_render_sunshafts();

	BOOL						enable_scissor			(light* L);		// true if intersects near plane
	void						enable_dbt_bounds		(light* L);

	void						disable_aniso			();

	void						draw_volume				(light* L);
	void						accum_direct_cascade	(u32	sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias );
	void						accum_direct_volumetric	(u32	sub_phase, const u32 Offset, const u32 i_offset, const Fmatrix &mShadow);
	void						accum_point				(light* L);
	void						accum_spot				(light* L);
	void						accum_reflected			(light* L);
	//	Igor: for volumetric lights
	void						accum_volumetric		(light* L);
	void						phase_bloom				();
	void						phase_luminance			();
	void						phase_combine			();
	void						phase_combine_volumetric();
	void						phase_pp				();

	virtual void				set_blur				(float	f)		{ param_blur=f;						}
	virtual void				set_gray				(float	f)		{ param_gray=f;						}
	virtual void				set_duality_h			(float	f)		{ param_duality_h=_abs(f);			}
	virtual void				set_duality_v			(float	f)		{ param_duality_v=_abs(f);			}
	virtual void				set_noise				(float	f)		{ param_noise=f;					}
	virtual void				set_noise_scale			(float	f)		{ param_noise_scale=f;				}
	virtual void				set_noise_fps			(float	f)		{ param_noise_fps=_abs(f)+EPS_S;	}
	virtual void				set_color_base			(u32	f)		{ param_color_base=f;				}
	virtual void				set_color_gray			(u32	f)		{ param_color_gray=f;				}
	virtual void				set_color_add			(const Fvector	&f)		{ param_color_add=f;		}

	virtual u32					get_width				();
	virtual u32					get_height				();
	virtual u32					get_target_width		();
	virtual u32					get_target_height		();
	virtual u32					get_core_width			();
	virtual u32					get_core_height			();

	virtual void				set_cm_imfluence	(float	f)		{ param_color_map_influence = f;							}
	virtual void				set_cm_interpolate	(float	f)		{ param_color_map_interpolate = f;							}
	virtual void				set_cm_textures		(const shared_str &tex0, const shared_str &tex1) {color_map_manager.SetTextures(tex0, tex1);}

	//	Need to reset stencil only when marker overflows.
	//	Don't clear when render for the first time
	void						reset_light_marker( bool bResetStencil = false);
	void						increment_light_marker();

	void						DoAsyncScreenshot		();
	void						PhaseGammaApply			();

#ifdef DEBUG
	IC void						dbg_addline				(Fvector& P0, Fvector& P1, u32 c)					{
		dbg_lines.push_back		(dbg_line_t());
		dbg_lines.back().P0		= P0;
		dbg_lines.back().P1		= P1;
		dbg_lines.back().color	= c;
	}
	IC void						dbg_addplane			(Fplane& P0,  u32 c)								{
		dbg_planes.push_back(P0);
	}
#else
	IC void						dbg_addline				(Fvector& P0, Fvector& P1, u32 c)					{}
	IC void						dbg_addplane			(Fplane& P0,  u32 c)								{}
#endif
};
