#include "StdAfx.h"
#include "SSSGameEffects.h"
#include "Actor.h"
#include "Level.h"
#include "GamePersistent.h"
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/Rain.h"
// COP_ENGINE_PATCH: COP-ENG-128 - SSS 23.5 rain footsteps by ascii1457, native callback adapter.
namespace {
ref_sound sssRainSounds[14];bool sssRainSoundsReady=false;
u32 sssLastStep=u32(-1),sssJumpIndex=0,sssLandIndex=0;
float sssValue(LPCSTR name) {float low=0,high=0;return Console->GetFloat(name,low,high);}
bool prepareSounds() {
    if(sssRainSoundsReady)return true;
    for(u32 i=0;i<14;++i) {
        string128 name;string_path path;
        if(i<8)xr_sprintf(name,"pf_sss\\rain_steps\\rain_%02u",i+1);
        else xr_sprintf(name,"pf_sss\\rain_steps\\rain_%s_%02u",i<11?"jump":"land",i<11?i-7:i-10);
        xr_strconcat(path,name,".ogg");
        if(!FS.exist("$game_sounds$",path)){ResetSSSRainSounds();return false;}
        sssRainSounds[i].create(name,st_Effect,sg_SourceType);
    }
    sssRainSoundsReady=true;return true;
}
// COP_ENGINE_PATCH: COP-ENG-192 - a static ground contact must have actual puddle coverage.
// The optional C export leaves native renderer/game virtual interfaces unchanged.
float contactCoverage(CActor* actor) {
    if(actor->Holder() || !g_pGameLevel)return 0.f;
    Fvector start=actor->Position();start.y+=.2f;
    Fvector down;down.set(0.f,-1.f,0.f);
    collide::rq_result hit;
    if(!Level().ObjectSpace.RayPick(start,down,.6f,collide::rqtBoth,hit,actor) || hit.O)return 0.f;
    using Probe=float(__cdecl*)(float,float,float);
    HMODULE renderer=GetModuleHandleA("xrRender_R4.dll");
    Probe probe=renderer?reinterpret_cast<Probe>(GetProcAddress(renderer,"xrSSSPuddleContact")):nullptr;
    if(!probe)return 0.f;
    const Fvector point=Fvector().mad(start,down,hit.range);
    return probe(point.x,point.y,point.z);
}
}
bool SSSRainStepOwns() {return Console && Console->GetBool("ssfx_enabled");}
void ResetSSSRainSounds() {
    for(auto& sound:sssRainSounds)sound.destroy();
    sssRainSoundsReady=false;sssLastStep=u32(-1);sssJumpIndex=sssLandIndex=0;
}
void SSSRainStep(CActor* actor,u32 kind,bool hudView) {
    if(!actor || actor!=Actor() || !actor->g_Alive() || !SSSRainStepOwns() || sssValue("ssfx_footsteps_enabled")<.5f || !g_pGamePersistent)return;
    // COP_ENGINE_PATCH: COP-ENG-193 - the puddle mask gates playback, not the legacy volume.
    // COP_ENGINE_PATCH: COP-ENG-192 - dry and non-terrain ground remain silent.
    if(sssValue("ssfx_puddles_enabled")<.5f || sssValue("ssfx_footsteps_volume")<=0.f || contactCoverage(actor)<.05f)return;
    const auto& environment=g_pGamePersistent->Environment();
    if(!environment.CurrentEnv || !environment.eff_Rain || !environment.eff_Rain->Rain_ROS)return;
    // COP_ENGINE_PATCH: COP-ENG-139 - wet steps follow the same buildup/drying clock as visible puddles.
    const float puddleWetness=sssValue("pf_sss_puddle_wetness");
    float surfaceWetness=Console->GetCommand("pf_sss_puddle_wetness") && puddleWetness>=0.f?puddleWetness:environment.wetness_factor;
    surfaceWetness=sssValue("ssfx_puddles_always")>.5f?1.f:_max(surfaceWetness,sssValue("ssfx_puddles_min_wetness"));
    const float wetness=clampr(surfaceWetness*2.f,0.f,1.f);
    if(wetness<=0)return;
    const float* cube=environment.eff_Rain->Rain_ROS->get_luminocity_hemi_cube();
    const float hemi=clampr(_max(_max(_max(cube[0],cube[1]),_max(cube[2],cube[3])),cube[5]),0.f,1.f);
    const float rain=clampr(environment.CurrentEnv->rain_density,sssValue("ssfx_footsteps_without_rain"),1.f);
    const float base=wetness*hemi*rain;
    float volume=0;u32 index=0;
    if(kind==0) {
        if(base<.05f)return;
        const u32 movement=actor->mstate_real;
        float modifier=1.f-sssValue("ssfx_footsteps_walk")*((movement&mcCrouch?1.f:0.f)+(movement&mcAccel?0.f:1.f)+(actor->IsZoomAimingMode()?1.f:0.f));
        if(movement&mcSprint)modifier=sssValue("ssfx_footsteps_run");
        volume=base*(sssValue("ssfx_footsteps_volume")+Random.randF(0.f,sssValue("ssfx_footsteps_variation")))*modifier;
        index=Random.randI(8);if(index==sssLastStep)index=(index+1)%8;sssLastStep=index;
    } else {
        if(hemi<=.1f)return;
        if(kind==1){index=8+(sssJumpIndex++%3);volume=sssValue("ssfx_footsteps_jump")*rain;}
        else {index=11+(sssLandIndex++%3);volume=sssValue("ssfx_footsteps_land")*rain;}
    }
    volume=clampr(volume,0.f,1.f);
    if(volume<=0 || !prepareSounds())return;
    Fvector position=hudView?Fvector().set(0,0,0):actor->Position();
    sssRainSounds[index].play_no_feedback(actor,hudView?sm_2D:0,0,&position,&volume);
}
