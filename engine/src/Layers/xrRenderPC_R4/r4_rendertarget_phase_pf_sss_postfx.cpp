#include "stdafx.h"
#include "r4_sss_viewport.h"
#include "r4_rendertarget.h"
#include "blender_pf_sss_postfx.h"
#include "../xrRender/SSSSettings.h"
// COP_ENGINE_PATCH: COP-ENG-128 - private postprocessing targets, no mutation of native preferences.
void CRenderTarget::reset_pf_sss_postfx() {
    reset_pf_sss_volume(); // lifetime follows the own rendering targets
    rt_pf_sss_emissive.destroy();pf_sss_emissive_frame=ps_ssfx_emissive_frame=u32(-1);
    s_pf_sss_postfx.destroy();s_pf_sss_bloom_finish.destroy();
    rt_pf_sss_post_resolve.destroy();rt_pf_sss_post_previous.destroy();rt_pf_sss_post_composite.destroy();rt_pf_sss_post_blur.destroy();
    for(u32 i=0;i<5;++i){s_pf_sss_bloom_down[i].destroy();rt_pf_sss_bloom_down[i].destroy();}
    for(u32 i=0;i<4;++i){s_pf_sss_bloom_up[i].destroy();rt_pf_sss_bloom_up[i].destroy();}
    rt_pf_sss_bloom_lens.destroy();rt_pf_sss_bloom_final.destroy();
    pf_sss_post_history=false;pf_sss_dof_far=0;
}
void CRenderTarget::prepare_pf_sss_postfx() {
    if(!ssfx_active()){reset_pf_sss_postfx();return;}
    if(s_pf_sss_postfx) return;
    const auto format=DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT;
    const u32 w=rt_Generic_0->dwWidth,h=rt_Generic_0->dwHeight;
    rt_pf_sss_emissive.create("$user$pf_sss_emissive",rt_Accumulator->dwWidth,rt_Accumulator->dwHeight,format);
    rt_pf_sss_post_resolve.create("$user$pf_sss_post_resolve",w,h,format);
    rt_pf_sss_post_previous.create("$user$pf_sss_post_previous",w,h,format);
    rt_pf_sss_post_composite.create("$user$pf_sss_post_composite",w,h,format);
    rt_pf_sss_post_blur.create("$user$pf_sss_post_blur",_max(1u,w/2),_max(1u,h/2),format);
    CBlender_pf_sss_postfx post; s_pf_sss_postfx.create(&post);
    for(u32 i=0;i<5;++i){
        string64 name;xr_sprintf(name,"$user$pf_sss_bloom_down%u",i);
        rt_pf_sss_bloom_down[i].create(name,_max(1u,rt_Generic->dwWidth>>(i+1)),_max(1u,rt_Generic->dwHeight>>(i+1)),format);
        string64 input;xr_sprintf(input,"$user$pf_sss_bloom_down%u",i?i-1:0);
        CBlender_pf_sss_bloom bloom(input,name);s_pf_sss_bloom_down[i].create(&bloom);
    }
    for(u32 i=0;i<4;++i){
        string64 name,input,base;xr_sprintf(name,"$user$pf_sss_bloom_up%u",i);
        xr_sprintf(input,i==3?"$user$pf_sss_bloom_down%u":"$user$pf_sss_bloom_up%u",i+1);
        xr_sprintf(base,"$user$pf_sss_bloom_down%u",i);
        rt_pf_sss_bloom_up[i].create(name,rt_pf_sss_bloom_down[i]->dwWidth,rt_pf_sss_bloom_down[i]->dwHeight,format);
        CBlender_pf_sss_bloom bloom(input,base);s_pf_sss_bloom_up[i].create(&bloom);
    }
    rt_pf_sss_bloom_lens.create("$user$pf_sss_bloom_lens",_max(1u,rt_Generic->dwWidth/4),_max(1u,rt_Generic->dwHeight/4),format);
    rt_pf_sss_bloom_final.create("$user$pf_sss_bloom_final",_max(1u,rt_Generic->dwWidth/2),_max(1u,rt_Generic->dwHeight/2),format);
    CBlender_pf_sss_bloom finish("$user$pf_sss_bloom_up0","$user$pf_sss_bloom_down0");s_pf_sss_bloom_finish.create(&finish);
    Msg("[SSS] MB/TAA and five-level bloom pipeline created");
}
void CRenderTarget::draw_pf_sss_postfx(const ref_rt& output,const ref_shader& shader,u32 element,const Fvector4& setup) {
    SSSViewportScope viewport(dwWidth, dwHeight);
    u_setrt(output,nullptr,nullptr,nullptr);RImplementation.rmNormal();RCache.set_Scissor(nullptr);RCache.set_CullMode(CULL_NONE);RCache.set_Stencil(FALSE);RCache.set_ColorWriteEnable();
    u32 offset=0;auto* v=(FVF::TL*)RCache.Vertex.Lock(3,g_combine->vb_stride,offset);
    v[0].set(-1,1,1,1,0,0,0);v[1].set(3,1,1,1,0,2,0);v[2].set(-1,-3,1,1,0,0,2);
    RCache.Vertex.Unlock(3,g_combine->vb_stride);RCache.set_Element(shader->E[element]);
    RCache.set_c("pf_sss_history",pf_sss_post_history?1.f:0.f,0.f,0.f,0.f);
    RCache.set_c("pf_sss_post_context",pf_sss_post_context);RCache.set_c("pf_sss_bloom_setup",setup);
    RCache.set_Geometry(g_combine);RCache.Render(D3DPT_TRIANGLELIST,offset,0,3,0,1);
    // Ensure CopyResource and subsequent reads never alias the output RTV.
    u_setrt(output->dwWidth,output->dwHeight,nullptr,nullptr,nullptr,nullptr);RImplementation.rmNormal();
}
void CRenderTarget::phase_pf_sss_postfx(bool temporal) {
    prepare_pf_sss_postfx();if(!s_pf_sss_postfx) return;
    const Fvector4 unused={};
    if(temporal) {
        const bool valid=pf_sss_post_history && pf_sss_post_frame+1==Device.dwFrame && pf_sss_post_revision==ps_ssfx_revision
            && pf_sss_post_camera.distance_to_sqr(Device.vCameraPosition)<4.f && pf_sss_post_direction.dotproduct(Device.vCameraDirection)>.8f
            && _abs(pf_sss_post_fov-Device.fFOV)<.01f && pf_sss_post_svp==Device.hudViewportData.isRenderActive;
        pf_sss_post_history=valid;
        if(ps_r_scale_mode<2 && ssfx_option(SSS_taa_enabled)>.5f) {
            draw_pf_sss_postfx(rt_pf_sss_post_resolve,s_pf_sss_postfx,0,unused);
            RContext->CopyResource(rt_pf_sss_post_previous->pSurface,rt_pf_sss_post_resolve->pSurface);
            draw_pf_sss_postfx(rt_pf_sss_post_composite,s_pf_sss_postfx,1,unused);
            RContext->CopyResource(rt_Generic_0->pSurface,rt_pf_sss_post_composite->pSurface);
            pf_sss_post_history=true;
        } else pf_sss_post_history=false;
        pf_sss_post_frame=Device.dwFrame;pf_sss_post_revision=ps_ssfx_revision;pf_sss_post_camera=Device.vCameraPosition;
        pf_sss_post_direction=Device.vCameraDirection;pf_sss_post_fov=Device.fFOV;pf_sss_post_svp=Device.hudViewportData.isRenderActive;
        return;
    }
    // COP_ENGINE_PATCH: COP-ENG-128 - DOF excluded by the user.
    pf_sss_post_context.set(0,0,0,Device.fTimeDelta);
    if(ssfx_option(SSS_fog_enabled)>.5f && ssfx_option(SSS_fog_scattering)>0) {
        const Fvector4 fogSetup={0,0,6,0};
        draw_pf_sss_postfx(rt_pf_sss_post_blur,s_pf_sss_postfx,3,fogSetup);
        draw_pf_sss_postfx(rt_pf_sss_post_composite,s_pf_sss_postfx,5,unused);
        RContext->CopyResource(rt_Generic_0->pSurface,rt_pf_sss_post_composite->pSurface);
    }
    if(ssfx_option(SSS_mb_enabled)>.5f) {
        draw_pf_sss_postfx(rt_pf_sss_post_composite,s_pf_sss_postfx,2,unused);
        RContext->CopyResource(rt_Generic_0->pSurface,rt_pf_sss_post_composite->pSurface);
    }

}
void CRenderTarget::phase_pf_sss_bloom() {
    prepare_pf_sss_postfx();if(!s_pf_sss_postfx) return;
    if(ssfx_option(SSS_bloom_enabled)<.5f){const FLOAT clear[4]={};RContext->ClearRenderTargetView(rt_pf_sss_bloom_final->pRT,clear);return;}
    Fvector4 setup={float(rt_Generic->dwWidth),float(rt_Generic->dwHeight),ssfx_option(SSS_bloom_blur),0};
    draw_pf_sss_postfx(rt_pf_sss_bloom_down[0],s_pf_sss_bloom_down[0],0,setup);
    for(u32 i=1;i<5;++i){setup.x=float(rt_pf_sss_bloom_down[i-1]->dwWidth);setup.y=float(rt_pf_sss_bloom_down[i-1]->dwHeight);
        draw_pf_sss_postfx(rt_pf_sss_bloom_down[i],s_pf_sss_bloom_down[i],1,setup);}
    for(int i=3;i>=0;--i){const auto& input=i==3?rt_pf_sss_bloom_down[4]:rt_pf_sss_bloom_up[i+1];setup.x=float(input->dwWidth);setup.y=float(input->dwHeight);
        draw_pf_sss_postfx(rt_pf_sss_bloom_up[i],s_pf_sss_bloom_up[i],2,setup);}
    draw_pf_sss_postfx(rt_pf_sss_bloom_lens,s_pf_sss_bloom_finish,3,setup);
    draw_pf_sss_postfx(rt_pf_sss_bloom_final,s_pf_sss_bloom_finish,4,setup);
}

// COP_ENGINE_PATCH: COP-ENG-128 - separate low-resolution point-light volume, no native preference writes.
void CRenderTarget::reset_pf_sss_volume() {
    rt_pf_sss_volume_point.destroy();rt_pf_sss_volume_tmp.destroy();
    s_pf_sss_volume_point.destroy();s_pf_sss_volume_tmp.destroy();pf_sss_volume_frame=u32(-1);
}
void CRenderTarget::prepare_pf_sss_volume() {
    const float scale=ssfx_option(SSS_volume_scale);
    const u32 w=_max(1u,u32(rt_Generic_0->dwWidth*scale)),h=_max(1u,u32(rt_Generic_0->dwHeight*scale));
    if(rt_pf_sss_volume_point && (rt_pf_sss_volume_point->dwWidth!=w || rt_pf_sss_volume_point->dwHeight!=h))reset_pf_sss_volume();
    if(rt_pf_sss_volume_point)return;
    rt_pf_sss_volume_point.create("$user$pf_sss_volume_point",w,h,DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT);
    rt_pf_sss_volume_tmp.create("$user$pf_sss_volume_tmp",w,h,DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT);
    CBlender_pf_sss_volume point("$user$pf_sss_volume_point"),tmp("$user$pf_sss_volume_tmp");
    s_pf_sss_volume_point.create(&point);s_pf_sss_volume_tmp.create(&tmp);
}
void CRenderTarget::accum_pf_sss_volume() {
    prepare_pf_sss_volume();u_setrt(rt_pf_sss_volume_point,nullptr,nullptr,nullptr);
    RImplementation.rmNormal(); // COP_ENGINE_PATCH: COP-ENG-136 - volume geometry uses its own scaled target.
    if(pf_sss_volume_frame!=Device.dwFrame) {
        const FLOAT clear[4]={};RContext->ClearRenderTargetView(rt_pf_sss_volume_point->pRT,clear);pf_sss_volume_frame=Device.dwFrame;
    }
    RCache.set_Stencil(FALSE);RCache.set_CullMode(CULL_NONE);RCache.set_ColorWriteEnable();RCache.set_Scissor(nullptr);
}
void CRenderTarget::combine_pf_sss_volume() {
    if(ssfx_option(SSS_volume_enabled)<.5f)return;
    prepare_pf_sss_postfx();prepare_pf_sss_volume();
    const bool point=pf_sss_volume_frame==Device.dwFrame;
    if(point)for(u32 i=0;i<4;++i) {
        Fvector4 setup;setup.set(0,0,(i+1)*ssfx_option(SSS_volume_blur),0);
        draw_pf_sss_postfx(i%2?rt_pf_sss_volume_point:rt_pf_sss_volume_tmp,i%2?s_pf_sss_volume_tmp:s_pf_sss_volume_point,0,setup);
    }
    Fvector4 setup;setup.set(m_bHasActiveVolumetric?1.f:0.f,point?1.f:0.f,0,0);
    draw_pf_sss_postfx(rt_pf_sss_post_composite,s_pf_sss_volume_point,1,setup);
    u_setrt(rt_Generic_0->dwWidth,rt_Generic_0->dwHeight,nullptr,nullptr,nullptr,nullptr);
    RContext->CopyResource(rt_Generic_0->pSurface,rt_pf_sss_post_composite->pSurface);
}

// Keep emissive geometry in a separate native-HDR target, then add it once to native lighting.
void CRenderTarget::begin_pf_sss_emissive() {
    if(!ssfx_active())return;
    prepare_pf_sss_postfx();if(!rt_pf_sss_emissive)return;
    const FLOAT clear[4]={};RContext->ClearRenderTargetView(rt_pf_sss_emissive->pRT,clear);
    u_setrt(rt_pf_sss_emissive,nullptr,nullptr,RDepth);
    pf_sss_emissive_frame=ps_ssfx_emissive_frame=Device.dwFrame;
}
void CRenderTarget::end_pf_sss_emissive() {
    if(!ssfx_active() || pf_sss_emissive_frame!=Device.dwFrame)return;
    // COP_ENGINE_PATCH: COP-ENG-133 - emissive uses the valid native slot 4, never E[6].
    draw_pf_sss_postfx(rt_Accumulator,s_pf_sss_postfx,4,Fvector4().set(0,0,0,0));
    phase_accumulator();
}
