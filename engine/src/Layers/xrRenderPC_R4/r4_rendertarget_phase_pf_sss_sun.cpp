#include "stdafx.h"
#include "r4_sss_viewport.h"
#include "r4_rendertarget.h"
#include "../xrRender/SSSSettings.h"
// COP_ENGINE_PATCH: COP-ENG-128 - original SSS temporal sun mask, native R4 depth/projection.
class CBlender_pf_sss_sun : public IBlender {
public:
    CBlender_pf_sss_sun(){description.CLS=0;}
    LPCSTR getComment() override{return "Private SSS temporal sun contact shadows";}
    BOOL canBeDetailed() override{return FALSE;}BOOL canBeLMAPped() override{return FALSE;}
    void Compile(CBlender_Compile& C) override {
        IBlender::Compile(C);
        C.r_Pass("stub_fullscreen_triangle",C.iElement==0?"pf_sss_sun_temporal":"pf_sss_sun_filter",FALSE,FALSE,FALSE);
        C.r_dx10Texture("s_position",r2_RT_P);C.r_dx10Texture("s_normal",r2_RT_N);C.r_dx10Texture("s_surface",r2_RT_S);C.r_dx10Texture("s_diffuse",r2_RT_albedo);
        C.r_dx10Texture("s_velocity",r2_RT_velocity);C.r_dx10Texture("pf_sss_sun_previous","$user$pf_sss_sun_previous");C.r_dx10Texture("pf_sss_sun_current","$user$pf_sss_sun_current");
        C.r_dx10Sampler("smp_nofilter");C.r_dx10Sampler("smp_rtlinear");C.r_End();
    }
};
void CRenderTarget::reset_pf_sss_sun() {
    s_pf_sss_sun.destroy();rt_pf_sss_sun_current.destroy();rt_pf_sss_sun_previous.destroy();rt_pf_sss_sun_filter.destroy();
    pf_sss_sun_frame=ps_ssfx_sun_frame=ps_ssfx_local_frame=pf_sss_local_frame=u32(-1);
    for(u32 i=0;i<2;++i){s_pf_sss_local[i].destroy();rt_pf_sss_local[i].destroy();rt_pf_sss_local_previous[i].destroy();rt_pf_sss_local_filter[i].destroy();}
}
void CRenderTarget::phase_pf_sss_sun() {
    if(!ssfx_active() || (ps_ssfx_modules.x<.5f && ps_ssfx_modules.y<.5f) || !rt_pf_sss_data){reset_pf_sss_sun();return;}
    string_path path;if(!FS.exist(path,"$game_shaders$","d3d11\\pf_sss_sun_temporal.ps.hlsl")){reset_pf_sss_sun();return;}
    if(!s_pf_sss_sun) {
        const u32 w=rt_Position->dwWidth,h=rt_Position->dwHeight;
        rt_pf_sss_sun_current.create("$user$pf_sss_sun_current",w,h,DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT);
        rt_pf_sss_sun_previous.create("$user$pf_sss_sun_previous",w,h,DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT);
        rt_pf_sss_sun_filter.create("$user$pf_sss_sun_filtered",w,h,DxgiFormat::DXGI_FORMAT_R16_FLOAT);
        CBlender_pf_sss_sun blender;s_pf_sss_sun.create(&blender);
    }
    const light* sun=(light*)RImplementation.Lights.sun_adapted._get();if(!sun){reset_pf_sss_sun();return;}
    Fvector direction;Device.mView.transform_dir(direction,sun->direction);direction.normalize_safe();
    const bool history=pf_sss_sun_frame+1==Device.dwFrame && pf_sss_sun_revision==ps_ssfx_revision && pf_sss_sun_camera.distance_to_sqr(Device.vCameraPosition)<4.f
        && pf_sss_sun_direction.dotproduct(Device.vCameraDirection)>.8f && _abs(pf_sss_sun_fov-Device.fFOV)<.01f
        && pf_sss_sun_svp==Device.hudViewportData.isRenderActive && pf_sss_sun_light.dotproduct(sun->direction)>.999f;
    pf_sss_sun_history=history;
    if(!history){const FLOAT clear[4]={0,0,0,1};RContext->ClearRenderTargetView(rt_pf_sss_sun_previous->pRT,clear);}
    auto draw=[&](const ref_rt& target,u32 element) {
        SSSViewportScope viewport(dwWidth, dwHeight);
        u_setrt(target,nullptr,nullptr,nullptr);RImplementation.rmNormal();RCache.set_Scissor(nullptr);RCache.set_CullMode(CULL_NONE);RCache.set_Stencil(FALSE);RCache.set_ColorWriteEnable();
        u32 offset=0;auto* v=(FVF::TL*)RCache.Vertex.Lock(3,g_combine->vb_stride,offset);
        v[0].set(-1,1,1,1,0,0,0);v[1].set(3,1,1,1,0,2,0);v[2].set(-1,-3,1,1,0,0,2);RCache.Vertex.Unlock(3,g_combine->vb_stride);
        RCache.set_Element(s_pf_sss_sun->E[element]);RCache.set_c("pf_sss_history",history?1.f:0.f,0,0,0);RCache.set_c("Ldynamic_dir",direction.x,direction.y,direction.z,0);
        RCache.set_Geometry(g_combine);RCache.Render(D3DPT_TRIANGLELIST,offset,0,3,0,1);
    };
    draw(rt_pf_sss_sun_current,0);draw(rt_pf_sss_sun_filter,1);
    u_setrt(rt_Position->dwWidth,rt_Position->dwHeight,nullptr,nullptr,nullptr,nullptr);
    // Depth history is committed after local-light masks have consumed the previous frame.
    pf_sss_sun_frame=ps_ssfx_sun_frame=Device.dwFrame;pf_sss_sun_revision=ps_ssfx_revision;pf_sss_sun_camera=Device.vCameraPosition;
    pf_sss_sun_direction=Device.vCameraDirection;pf_sss_sun_fov=Device.fFOV;pf_sss_sun_svp=Device.hudViewportData.isRenderActive;pf_sss_sun_light=sun->direction;
}

void CRenderTarget::finish_pf_sss_sun() {
    if(rt_pf_sss_sun_current && rt_pf_sss_sun_previous && pf_sss_sun_frame==Device.dwFrame)
        RContext->CopyResource(rt_pf_sss_sun_previous->pSurface,rt_pf_sss_sun_current->pSurface);
}
class CBlender_pf_sss_local : public IBlender {
    u32 group;
public:
    CBlender_pf_sss_local(u32 index):group(index){description.CLS=0;}
    LPCSTR getComment() override{return "Private SSS temporal local contact shadows";}
    BOOL canBeDetailed() override{return FALSE;}BOOL canBeLMAPped() override{return FALSE;}
    void Compile(CBlender_Compile& C) override {
        IBlender::Compile(C);if(C.iElement>1)return;
        C.r_Pass("stub_fullscreen_triangle",C.iElement==0?"pf_sss_local_temporal":"pf_sss_local_filter",FALSE,FALSE,FALSE);
        C.r_dx10Texture("s_position",r2_RT_P);C.r_dx10Texture("s_normal",r2_RT_N);C.r_dx10Texture("s_surface",r2_RT_S);C.r_dx10Texture("s_diffuse",r2_RT_albedo);
        C.r_dx10Texture("s_velocity",r2_RT_velocity);C.r_dx10Texture("pf_sss_data","$user$pf_sss_data");
        C.r_dx10Texture("pf_sss_sun_previous","$user$pf_sss_sun_previous");C.r_dx10Texture("pf_sss_sun_current","$user$pf_sss_sun_current");
        string64 name;xr_sprintf(name,"$user$pf_sss_local_previous%u",group);C.r_dx10Texture("pf_sss_local_previous",name);
        xr_sprintf(name,"$user$pf_sss_local_current%u",group);C.r_dx10Texture("pf_sss_local_current",name);
        C.r_dx10Sampler("smp_nofilter");C.r_dx10Sampler("smp_rtlinear");C.r_End();
    }
};
void CRenderTarget::phase_pf_sss_local(light_Package& normal,light_Package& pending) {
    if(!ssfx_active() || ps_ssfx_modules.y<.5f || !rt_pf_sss_sun_current){ps_ssfx_local_frame=u32(-1);return;}
    light* current[8]={};u32 count=0;
    for(auto* package:{&normal,&pending})for(auto* light:package->v_shadowed) {
        if(!light || count==8)break;
        bool duplicate=false;for(u32 n=0;n<count;++n)duplicate|=current[n]->position.similar(light->position,.001f);
        if(!duplicate)current[count++]=light;
    }
    bool history=pf_sss_sun_history && pf_sss_local_frame+1==Device.dwFrame;
    for(u32 n=0;n<8;++n) {
        history &= current[n]==pf_sss_local_ids[n];pf_sss_local_ids[n]=current[n];
        Fvector position={};if(current[n])Device.mView.transform_tiny(position,current[n]->position);
        ps_ssfx_local_lights[n].set(position.x,position.y,position.z,current[n]?1.f:0.f);
    }
    for(u32 group=0;group<2;++group) {
        if(!s_pf_sss_local[group]) {
            const auto format=DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT;string64 name;
            xr_sprintf(name,"$user$pf_sss_local_current%u",group);rt_pf_sss_local[group].create(name,rt_Position->dwWidth,rt_Position->dwHeight,format);
            xr_sprintf(name,"$user$pf_sss_local_previous%u",group);rt_pf_sss_local_previous[group].create(name,rt_Position->dwWidth,rt_Position->dwHeight,format);
            xr_sprintf(name,"$user$pf_sss_local_filtered%u",group);rt_pf_sss_local_filter[group].create(name,rt_Position->dwWidth,rt_Position->dwHeight,format);
            CBlender_pf_sss_local blender(group);s_pf_sss_local[group].create(&blender);
        }
        if(!history){const FLOAT clear[4]={1,1,1,1};RContext->ClearRenderTargetView(rt_pf_sss_local_previous[group]->pRT,clear);}
        for(u32 pass=0;pass<2;++pass) {
            const auto& target=pass?rt_pf_sss_local_filter[group]:rt_pf_sss_local[group];
            SSSViewportScope viewport(dwWidth, dwHeight);
            u_setrt(target,nullptr,nullptr,nullptr);RImplementation.rmNormal();RCache.set_Scissor(nullptr);RCache.set_CullMode(CULL_NONE);RCache.set_Stencil(FALSE);RCache.set_ColorWriteEnable();
            u32 offset=0;auto* v=(FVF::TL*)RCache.Vertex.Lock(3,g_combine->vb_stride,offset);
            v[0].set(-1,1,1,1,0,0,0);v[1].set(3,1,1,1,0,2,0);v[2].set(-1,-3,1,1,0,0,2);RCache.Vertex.Unlock(3,g_combine->vb_stride);
            RCache.set_Element(s_pf_sss_local[group]->E[pass]);RCache.set_c("pf_sss_history",history?1.f:0.f,0,0,0);RCache.set_c("pf_sss_local_group",float(group),0,0,0);
            RCache.set_Geometry(g_combine);RCache.Render(D3DPT_TRIANGLELIST,offset,0,3,0,1);
        }
        u_setrt(rt_Position->dwWidth,rt_Position->dwHeight,nullptr,nullptr,nullptr,nullptr);
        RContext->CopyResource(rt_pf_sss_local_previous[group]->pSurface,rt_pf_sss_local[group]->pSurface);
    }
    pf_sss_local_frame=ps_ssfx_local_frame=Device.dwFrame;
}
