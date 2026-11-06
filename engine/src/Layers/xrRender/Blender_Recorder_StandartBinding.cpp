#include "stdafx.h"


#include "ResourceManager.h"
#include "blenders/Blender_Recorder.h"
#include "blenders/Blender.h"

#include "../../xrEngine/IGame_Persistent.h"
#include "../../xrEngine/Environment.h"

#include "dxRenderDeviceRender.h"
#include "../../xrEngine/IGame_Level.h"
#include "../../xrEngine/date_time.h"
#include "xrRender_console.h"
#include "SSSSettings.h"
#include "../../xrEngine/Rain.h"

// COP_ENGINE_PATCH: COP-ENG-172 - watch telemetry is snapshotted once per render frame.
namespace
{
using WatchSnapshot172 = BOOL (__cdecl*)(unsigned,float*,unsigned);
const float* WatchValues172()
{
    static float values[16]={0,0,0,0, 1,1,0,1, 1,1,1,1, .1f,.9f,.25f,0};
    static u32 frame=u32(-1);
    static WatchSnapshot172 snapshot=nullptr;
    if (frame!=Device.dwFrame)
    {
        frame=Device.dwFrame;
        if (!snapshot)
        {
            HMODULE game=GetModuleHandleA("xrGame.dll");
            if (game) snapshot=reinterpret_cast<WatchSnapshot172>(GetProcAddress(game,"xrWatchShaderSnapshot172"));
        }
        if (snapshot) snapshot(172,values,16);
    }
    return values;
}
template<unsigned Offset> class WatchConstant172 : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        const float* v=WatchValues172()+Offset;
        RCache.set_c(C,v[0],v[1],v[2],v[3]);
    }
};
WatchConstant172<0> watchTime172;
WatchConstant172<4> watchActor172;
WatchConstant172<8> watchCondition172;
WatchConstant172<12> watchColor172;
}

#ifdef USE_DX11
// COP_ENGINE_PATCH: COP-ENG-126 - master gate is evaluated at rendering time.
class cl_ssfx_control : public R_constant_setup {
    void setup(R_constant* C) override {
        RCache.set_c(C, float(ps_ssfx_enabled), ps_ssfx_modules.x, ps_ssfx_modules.y, ps_ssfx_modules.z);
    }
};
static cl_ssfx_control binder_ssfx_control;
// COP_ENGINE_PATCH: COP-ENG-128 - original per-map offsets plus independent manual adjustments.
static class cl_pf_sss_terrain_map : public R_constant_setup {
    void setup(R_constant* C) override {
        static const struct Profile {LPCSTR map;Fvector4 offsets;} profiles[]={
{"jupiter",{-0.05f,-0.05f,0.05f,-0.15f}},
{"k00_marsh",{-0.13f,-0.13f,-0.13f,-0.13f}},
{"k01_darkscape",{-0.13f,-0.05f,-0.13f,0.0f}},
{"k02_trucks_cemetery",{-0.07f,-0.05f,-0.13f,0.0f}},
{"l01_escape",{-0.1f,-0.05f,-0.1f,0.0f}},
{"l02_garbage",{-0.13f,-0.05f,-0.13f,-0.1f}},
{"l03_agroprom",{-0.07f,-0.05f,-0.13f,-0.1f}},
{"l04_darkvalley",{-0.07f,-0.05f,-0.07f,-0.1f}},
{"l05_bar",{-0.07f,-0.05f,-0.13f,0.0f}},
{"l06_rostok",{-0.07f,-0.05f,-0.07f,0.0f}},
{"l07_military",{-0.07f,-0.05f,0.0f,-0.05f}},
{"l08_yantar",{-0.13f,-0.05f,-0.13f,0.0f}},
{"l09_deadcity",{-0.13f,-0.05f,-0.13f,0.0f}},
{"l10_limansk",{-0.13f,-0.05f,0.0f,0.0f}},
{"l10_radar",{-0.07f,-0.05f,-0.07f,0.0f}},
{"l10_red_forest",{-0.13f,-0.05f,-0.07f,-0.1f}},
{"l11_hospital",{-0.13f,-0.13f,-0.13f,-0.13f}},
{"l11_pripyat",{-0.1f,-0.05f,0.0f,-0.05f}},
{"l12_stancia",{-0.1f,-0.05f,0.0f,-0.1f}},
{"l12_stancia_2",{-0.1f,-0.05f,0.0f,0.0f}},
{"l13_generators",{-0.07f,-0.07f,-0.13f,-0.05f}},
{"pripyat",{-0.13f,-0.05f,-0.05f,-0.1f}},
{"PripyatFull",{-0.13f,-0.05f,-0.05f,-0.1f}},
{"pripyatfull",{-0.13f,-0.05f,-0.05f,-0.1f}},
{"zaton",{-0.07f,-0.05f,-0.13f,-0.1f}},
{"y04_pole",{-0.07f,-0.05f,-0.07f,-0.07f}},
        };
        Fvector4 value={};
        if(g_pGameLevel && ssfx_option(SSS_terrain_auto_offsets)>.5f) {
            const shared_str map=g_pGameLevel->name();
            for(const auto& profile:profiles)if(map==profile.map){value=profile.offsets;break;}
        }
        RCache.set_c(C,value);
    }
} binder_sss_terrain_map;
static class cl_pf_sss_lut_transition : public R_constant_setup {
    void setup(R_constant* C) override {
        static u32 frame=u32(-1);static shared_str map;static float from=0,to=0,blend=0,intensity=0;
        static bool initialized=false;
        const shared_str currentMap=g_pGameLevel?g_pGameLevel->name():shared_str();
        if(initialized && map!=currentMap)ps_ssfx_lut_override.set(0,0,0,0);
        float target=ssfx_option(SSS_lut_table),targetIntensity=ssfx_option(SSS_lut_intensity);
        string_path profilePath;FS.update_path(profilePath,"$game_config$","pf_sss_lut.ltx");
        static CInifile profiles(profilePath,TRUE,TRUE,FALSE);
        if(currentMap.c_str()) {
            if(profiles.line_exist(currentMap.c_str(),"group"))target=clampr(profiles.r_float(currentMap.c_str(),"group"),0.f,3.f);
            if(profiles.line_exist(currentMap.c_str(),"intensity"))targetIntensity=clampr(profiles.r_float(currentMap.c_str(),"intensity"),0.f,1.f);
        }
        if(ps_ssfx_lut_override.x>.5f){target=ps_ssfx_lut_override.z;targetIntensity=ps_ssfx_lut_override.y;}
        if(!initialized || map!=currentMap || !ssfx_active() || ps_ssfx_lut_override.w>.5f) {
            from=to=target;blend=0;intensity=targetIntensity;initialized=ssfx_active();map=currentMap;
            ps_ssfx_lut_override.w=0;
        }
        if(frame!=Device.dwFrame) {
            frame=Device.dwFrame;
            if(to!=target && blend==0)to=target;
            const float step=_min(.005f*Device.fTimeDelta*50.f,.19f);
            if(from!=to) {blend=_min(1.f,blend+step);if(blend>=.995f){from=to;blend=0;}}
            intensity+=clampr(targetIntensity-intensity,-step,step);
        }
        RCache.set_c(C,intensity,from,to,blend);
    }
} binder_sss_lut_transition;

static class cl_pf_sss_bloom_weather : public R_constant_setup {
    void setup(R_constant* C) override {
        Fvector4 value={ssfx_option(SSS_bloom_threshold),ssfx_option(SSS_bloom_exposure),ssfx_option(SSS_bloom_sky),ps_ssfx_emissive_frame==Device.dwFrame?1.f:0.f};
        if(ssfx_option(SSS_bloom_weather)>.5f && g_pGamePersistent && g_pGamePersistent->Environment().CurrentWeatherName.c_str() && g_pGamePersistent->Environment().CurrentEnv) {
            auto& env=g_pGamePersistent->Environment();
            float values[2][3];
            for(u32 n=0;n<2;++n) {
                const auto* descriptor=env.Current[n];
                string_path profilePath;FS.update_path(profilePath,"$game_config$","pf_sss_weather.ltx");
                static CInifile ini(profilePath,TRUE,TRUE,FALSE);
                string256 section;xr_sprintf(section,"%s.%s",env.CurrentWeatherName.c_str(),descriptor?descriptor->m_identifier.c_str():"");
                if(!ini.section_exist(section))xr_strcpy(section,env.CurrentWeatherName.c_str());
                LPCSTR keys[]={"ssfx_bloom_threshold","ssfx_bloom_exposure","ssfx_bloom_sky_intensity"};
                for(u32 k=0;k<3;++k)values[n][k]=ini.line_exist(section,keys[k])?ini.r_float(section,keys[k]):(&value.x)[k];
            }
            for(u32 k=0;k<3;++k)(&value.x)[k]=values[0][k]*(1-env.CurrentEnv->weight)+values[1][k]*env.CurrentEnv->weight;
        }
        value.x=clampr(value.x,1.f,10.f);value.y=clampr(value.y,1.f,100.f);value.z=clampr(value.z,0.f,10.f);
        RCache.set_c(C,value);
    }
} binder_sss_bloom_weather;

static class cl_pf_sss_local_ready : public R_constant_setup {
    void setup(R_constant* C) override {RCache.set_c(C,ps_ssfx_local_frame==Device.dwFrame?1.f:0.f,0,0,0);}
} binder_sss_local_ready;
static class cl_pf_sss_local_lights : public R_constant_setup {
    void setup(R_constant* C) override {for(u32 i=0;i<8;++i){const auto& p=ps_ssfx_local_lights[i];RCache.set_ca(C,i,p.x,p.y,p.z,p.w);}}
} binder_sss_local_lights;
static class cl_pf_sss_sun_ready : public R_constant_setup {
    void setup(R_constant* C) override {RCache.set_c(C,ps_ssfx_sun_frame==Device.dwFrame?1.f:0.f,0.f,0.f,0.f);}
} binder_sss_sun_ready;
// COP_ENGINE_PATCH: COP-ENG-128 - weather-driven water/vegetation animation.
// COP_ENGINE_PATCH: COP-ENG-128 - private wet accumulation, never change Environment wetness/preferences.
// COP_ENGINE_PATCH: COP-ENG-139 - local linkage contract with xrRender_console.cpp; no exported ABI changes.
extern float ps_pf_sss_puddle_wetness;
static class cl_pf_sss_weather_rain : public R_constant_setup {
    float wetness=0;u32 frame=u32(-1);bool active=false;const void* level=nullptr;
public:
    // COP_ENGINE_PATCH: COP-ENG-140 - continuous wetness, even when no terrain was drawn last frame.
    float update() {
        const auto* environment=g_pGamePersistent?&g_pGamePersistent->Environment():nullptr;
        const auto* env=environment?environment->CurrentEnv:nullptr;
        const float rain=env?env->rain_density:0.f;
        const bool enabled=ssfx_puddles_active() && env && g_pGameLevel;
        if(!enabled){active=false;level=nullptr;wetness=environment?environment->wetness_factor:0.f;}
        else if(frame!=Device.dwFrame){
            if(!active || level!=g_pGameLevel || ps_pf_sss_puddle_wetness<0.f)wetness=environment->wetness_factor;
            else wetness+=Device.fTimeDelta*(rain>0?rain*.024f*ssfx_option(SSS_wet_buildup):-.012f*ssfx_option(SSS_wet_drying));
            clamp(wetness,0.f,1.f);active=true;level=g_pGameLevel;
        }
        frame=Device.dwFrame;ps_pf_sss_puddle_wetness=enabled?wetness:-1.f;
        return wetness;
    }
    void setup(R_constant* C) override {
        const auto* environment=g_pGamePersistent?&g_pGamePersistent->Environment():nullptr;
        const auto* env=environment?environment->CurrentEnv:nullptr;
        RCache.set_c(C,env?env->rain_density:0.f,update(),0,0);
    }
} binder_pf_sss_weather_rain;
float ssfx_update_puddle_wetness() {return binder_pf_sss_weather_rain.update();}
static Fvector2 sssWeatherWind() {
    const auto* environment=g_pGamePersistent?&g_pGamePersistent->Environment():nullptr;
    const auto* env=environment?environment->CurrentEnv:nullptr;
    Fvector2 result;result.set(env?env->wind_direction:0.f,env?env->wind_velocity:0.f);
    if(!environment || !environment->bWFX || !ssfx_active() || ssfx_option(SSS_wind_vanilla_emission)<.5f)return result;
    static CInifile* profiles=nullptr;static bool checked=false;
    if(!checked){checked=true;string_path path;if(FS.exist(path,"$game_config$","pf_sss_wind.ltx"))profiles=new CInifile(path,TRUE,TRUE,FALSE);}
    if(!profiles || !environment->CurrentWeatherName.c_str())return result;
    Fvector2 values[2]={result,result};
    for(u32 i=0;i<2;++i){const auto* descriptor=environment->Current[i];if(!descriptor)continue;
        string128 section;xr_sprintf(section,"%s.%s",environment->CurrentWeatherName.c_str(),descriptor->m_identifier.c_str());
        if(profiles->section_exist(section)){
            values[i].x=deg2rad(profiles->r_float(section,"wind_direction"));
            values[i].y=profiles->r_float(section,"wind_velocity");
        }
    }
    result.x=values[0].x+(values[1].x-values[0].x)*env->weight;
    result.y=values[0].y+(values[1].y-values[0].y)*env->weight;return result;
}
static class cl_ssfx_wind : public R_constant_setup {
    void setup(R_constant* C) override {
        const auto* env = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv : nullptr;
        const auto wind=sssWeatherWind();
        RCache.set_c(C, wind.x, wind.y,
            g_pGamePersistent ? g_pGamePersistent->Environment().wind_strength_factor : 0.f, 0.f);
    }
} binder_ssfx_wind;
// COP_ENGINE_PATCH: COP-ENG-128 - integrate weather flow once per frame; preserve previous wind and entity motion.
static Fvector4 pfSSSWindAnimation={}, pfSSSWindPrevious={};
static Fvector4 pfSSSBenders[48]={}, pfSSSBendersPrevious[48]={};
static u32 pfSSSAnimationFrame=u32(-1);
static float pfSSSBenderIDs[24]={};
static void updateSSSAnimation() {
    if(pfSSSAnimationFrame==Device.dwFrame)return;
    const bool contiguous=pfSSSAnimationFrame+1==Device.dwFrame;
    pfSSSAnimationFrame=Device.dwFrame;pfSSSWindPrevious=pfSSSWindAnimation;
    const auto* env=g_pGamePersistent?g_pGamePersistent->Environment().CurrentEnv:nullptr;
    if(env) {
        const auto wind=sssWeatherWind();
        const float speed=_max(_max(wind.y,ssfx_option(SSS_wind_min)*1000.f),200.f)*.001f;
        const float direction=-wind.x+PI_DIV_2;
        pfSSSWindAnimation.x+=speed*_cos(direction)*Device.fTimeDelta;
        pfSSSWindAnimation.y+=speed*_sin(direction)*Device.fTimeDelta;
        pfSSSWindAnimation.z+=clampr(speed*1.33f,0.f,1.f)*Device.fTimeDelta;
        pfSSSWindAnimation.w+=Device.fTimeDelta;
    }
    for(u32 i=0;i<24;++i) {
        pfSSSBendersPrevious[i]=pfSSSBenders[i];pfSSSBendersPrevious[i+24]=pfSSSBenders[i+24];
        const auto& point=ps_ssfx_bender_positions[i];const auto& meta=ps_ssfx_bender_metadata[i];
        // COP_ENGINE_PATCH: COP-ENG-187 - publish only opted-in grass benders; keep full SSS pipeline OFF.
        bool active=ssfx_effects_active() && ssfx_option(SSS_grass_enabled)>.5f && meta.x>0;
        active=active && (i!=0 || ssfx_option(SSS_grass_player)>.5f);
        if(i<16 && meta.y>1.5f && ssfx_option(SSS_grass_mutants)<.5f)active=false;
        pfSSSBenders[i].set(point.x,point.y,point.z,active?meta.x:0.f);
        if(i<16)pfSSSBenders[i+24].set(0,-99,0,1);
        else {const auto& direction=ps_ssfx_bender_directions[i-16];pfSSSBenders[i+24].set(direction.x,direction.y,direction.z,meta.y);}
        if(!contiguous || meta.z!=pfSSSBenderIDs[i]) {pfSSSBendersPrevious[i]=pfSSSBenders[i];pfSSSBendersPrevious[i+24]=pfSSSBenders[i+24];}
        pfSSSBenderIDs[i]=meta.z;
    }
    if(!contiguous)pfSSSWindPrevious=pfSSSWindAnimation;
}
template<bool previous> class cl_pf_sss_wind_animation : public R_constant_setup {
    void setup(R_constant* C) override {updateSSSAnimation();RCache.set_c(C,previous?pfSSSWindPrevious:pfSSSWindAnimation);}
};
template<bool previous> class cl_pf_sss_benders : public R_constant_setup {
    void setup(R_constant* C) override {updateSSSAnimation();const auto* values=previous?pfSSSBendersPrevious:pfSSSBenders;
        for(u32 i=0;i<48;++i)RCache.set_ca(C,i,values[i].x,values[i].y,values[i].z,values[i].w);}
};
static cl_pf_sss_wind_animation<false> binder_sss_wind_animation;
static cl_pf_sss_wind_animation<true> binder_sss_wind_previous;
static cl_pf_sss_benders<false> binder_sss_benders;
static cl_pf_sss_benders<true> binder_sss_benders_previous;
// COP_ENGINE_PATCH: COP-ENG-128 - original droplet accumulation uses rain exposure and elapsed time.
static class cl_pf_sss_hud_rain : public R_constant_setup {
    float animation=0,wetness=0;u32 frame=u32(-1);
    void setup(R_constant* C) override {
        if(frame!=Device.dwFrame) {
            const bool contiguous=frame+1==Device.dwFrame;frame=Device.dwFrame;
            if(!contiguous){animation=0;wetness=0;}
            const auto* env=g_pGamePersistent?g_pGamePersistent->Environment().CurrentEnv:nullptr;
            const auto* rain=g_pGamePersistent?g_pGamePersistent->Environment().eff_Rain:nullptr;
            float exposed=0;
            if(rain && rain->Rain_ROS){const auto* cube=rain->Rain_ROS->get_luminocity_hemi_cube();
                exposed=_max(_max(cube[0],cube[1]),_max(_max(cube[2],cube[3]),cube[5]));}
            const float density=env?env->rain_density:0.f;
            const float dt=_min(Device.fTimeDelta,.1f);
            if(!ssfx_active() || ssfx_option(SSS_hud_rain_enabled)<.5f || !env)wetness=0;
            else if(density>0 && exposed>.15f) {
                animation+=ssfx_option(SSS_hud_rain_speed)*.02f*dt*1000.f/((1.5f-density)*10.f);
                wetness+=ssfx_option(SSS_hud_rain_buildup)*.009f*dt*10.f;
            }else wetness-=ssfx_option(SSS_hud_rain_drying)*.001f*dt*10.f;
            clamp(wetness,0.f,1.f);if(animation>99000)animation=0;
        }
        RCache.set_c(C,animation,wetness,Device.hudViewportData.isRenderProcess?1.f:0.f,0.f);
    }
} binder_sss_hud_rain;
#define SSFX_BINDER(name, value) \
class cl_##name : public R_constant_setup { void setup(R_constant* C) override { RCache.set_c(C, value); } }; \
static cl_##name binder_##name;
SSFX_BINDER(ssfx_shadow, ps_ssfx_shadow)
SSFX_BINDER(ssfx_quality, ps_ssfx_quality)
SSFX_BINDER(ssfx_ao, ps_ssfx_ao)
SSFX_BINDER(ssfx_ao2, ps_ssfx_ao2)
SSFX_BINDER(ssfx_options0, ps_ssfx_options[0])
SSFX_BINDER(ssfx_options1, ps_ssfx_options[1])
SSFX_BINDER(ssfx_options2, ps_ssfx_options[2])
SSFX_BINDER(ssfx_options3, ps_ssfx_options[3])
SSFX_BINDER(ssfx_options4, ps_ssfx_options[4])
SSFX_BINDER(ssfx_options5, ps_ssfx_options[5])
SSFX_BINDER(ssfx_options6, ps_ssfx_options[6])
SSFX_BINDER(ssfx_options7, ps_ssfx_options[7])
SSFX_BINDER(ssfx_options8, ps_ssfx_options[8])
SSFX_BINDER(ssfx_options9, ps_ssfx_options[9])
SSFX_BINDER(ssfx_options10, ps_ssfx_options[10])
SSFX_BINDER(ssfx_options11, ps_ssfx_options[11])
SSFX_BINDER(ssfx_options12, ps_ssfx_options[12])
SSFX_BINDER(ssfx_options13, ps_ssfx_options[13])
SSFX_BINDER(ssfx_options14, ps_ssfx_options[14])
SSFX_BINDER(ssfx_options15, ps_ssfx_options[15])
SSFX_BINDER(ssfx_options16, ps_ssfx_options[16])
SSFX_BINDER(ssfx_options17, ps_ssfx_options[17])
SSFX_BINDER(ssfx_options18, ps_ssfx_options[18])
SSFX_BINDER(ssfx_options19, ps_ssfx_options[19])
SSFX_BINDER(ssfx_options20, ps_ssfx_options[20])
SSFX_BINDER(ssfx_options21, ps_ssfx_options[21])
SSFX_BINDER(ssfx_options22, ps_ssfx_options[22])
SSFX_BINDER(ssfx_options23, ps_ssfx_options[23])
SSFX_BINDER(ssfx_options24, ps_ssfx_options[24])
SSFX_BINDER(ssfx_options25, ps_ssfx_options[25])
SSFX_BINDER(ssfx_options26, ps_ssfx_options[26])
SSFX_BINDER(ssfx_options27, ps_ssfx_options[27])
SSFX_BINDER(ssfx_options28, ps_ssfx_options[28])
SSFX_BINDER(ssfx_options29, ps_ssfx_options[29])
SSFX_BINDER(ssfx_options30, ps_ssfx_options[30])
SSFX_BINDER(ssfx_options31, ps_ssfx_options[31])
SSFX_BINDER(ssfx_options32, ps_ssfx_options[32])
SSFX_BINDER(ssfx_options33, ps_ssfx_options[33])
SSFX_BINDER(ssfx_options34, ps_ssfx_options[34])
SSFX_BINDER(ssfx_options35, ps_ssfx_options[35])
SSFX_BINDER(ssfx_options36, ps_ssfx_options[36])
SSFX_BINDER(ssfx_options37, ps_ssfx_options[37])
SSFX_BINDER(ssfx_options38, ps_ssfx_options[38])
SSFX_BINDER(ssfx_options39, ps_ssfx_options[39])
SSFX_BINDER(ssfx_options40, ps_ssfx_options[40])
SSFX_BINDER(ssfx_options41, ps_ssfx_options[41])
SSFX_BINDER(ssfx_options42, ps_ssfx_options[42])
SSFX_BINDER(ssfx_options43, ps_ssfx_options[43])
SSFX_BINDER(ssfx_options44, ps_ssfx_options[44])
SSFX_BINDER(ssfx_options45, ps_ssfx_options[45])
SSFX_BINDER(ssfx_options46, ps_ssfx_options[46])
SSFX_BINDER(ssfx_options47, ps_ssfx_options[47])
SSFX_BINDER(ssfx_options48, ps_ssfx_options[48])
SSFX_BINDER(ssfx_options49, ps_ssfx_options[49])
SSFX_BINDER(ssfx_options50, ps_ssfx_options[50])
SSFX_BINDER(ssfx_options51, ps_ssfx_options[51])
SSFX_BINDER(ssfx_options52, ps_ssfx_options[52])
SSFX_BINDER(ssfx_options53, ps_ssfx_options[53])
SSFX_BINDER(ssfx_options54, ps_ssfx_options[54])
SSFX_BINDER(ssfx_options55, ps_ssfx_options[55])
SSFX_BINDER(ssfx_options56, ps_ssfx_options[56])
SSFX_BINDER(ssfx_options57, ps_ssfx_options[57])
SSFX_BINDER(ssfx_options58, ps_ssfx_options[58])
SSFX_BINDER(ssfx_options59, ps_ssfx_options[59])
SSFX_BINDER(ssfx_options60, ps_ssfx_options[60])
// COP_ENGINE_PATCH: COP-ENG-187 - independent grass wind switch.
SSFX_BINDER(ssfx_options61, ps_ssfx_options[61])
#undef SSFX_BINDER
#endif
// matrices
#define	BIND_DECLARE(xf)	\
class cl_xform_##xf	: public R_constant_setup {	virtual void setup (R_constant* C) { RCache.xforms.set_c_##xf (C); } }; \
	static cl_xform_##xf	binder_##xf

BIND_DECLARE(invw);
BIND_DECLARE(invv);

BIND_DECLARE(w);
BIND_DECLARE(v);
BIND_DECLARE(p);
BIND_DECLARE(wv);
BIND_DECLARE(vp);
BIND_DECLARE(wvp);

#ifdef USE_DX11
BIND_DECLARE(w_old);
BIND_DECLARE(v_old);
BIND_DECLARE(p_old);
BIND_DECLARE(wv_old);
BIND_DECLARE(vp_old);
BIND_DECLARE(wvp_old);
#endif

#define DECLARE_TREE_BIND(c)	\
	class cl_tree_##c: public R_constant_setup	{virtual void setup(R_constant* C) {RCache.tree.set_c_##c(C);} };	\
	static cl_tree_##c	tree_binder_##c

DECLARE_TREE_BIND(m_xform_v);
DECLARE_TREE_BIND(m_xform);

DECLARE_TREE_BIND(consts);
DECLARE_TREE_BIND(wave);
DECLARE_TREE_BIND(wind);

#ifdef USE_DX11
DECLARE_TREE_BIND(consts_old);
DECLARE_TREE_BIND(wave_old);
DECLARE_TREE_BIND(wind_old);
#endif

DECLARE_TREE_BIND(c_scale);
DECLARE_TREE_BIND(c_bias);
DECLARE_TREE_BIND(c_sun);

class cl_hemi_cube_pos_faces: public R_constant_setup
{
	virtual void setup(R_constant* C) {RCache.hemi.set_c_pos_faces(C);}
};

static cl_hemi_cube_pos_faces binder_hemi_cube_pos_faces;

class cl_hemi_cube_neg_faces: public R_constant_setup
{
	virtual void setup(R_constant* C) {RCache.hemi.set_c_neg_faces(C);}
};

static cl_hemi_cube_neg_faces binder_hemi_cube_neg_faces;

class cl_material: public R_constant_setup
{
	virtual void setup(R_constant* C) {RCache.hemi.set_c_material(C);}
};

static cl_material binder_material;

class cl_texgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;

#ifdef USE_DX11
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f,				0.5f,				0.0f,			1.0f
		};
#else //USE_DX11
		float	_w						= float(RCache.get_width());
		float	_h						= float(RCache.get_height());
		float	o_w						= (.5f / _w);
		float	o_h						= (.5f / _h);
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f + o_w,			0.5f + o_h,			0.0f,			1.0f
		};
#endif

		mTexgen.mul	(mTexelAdjust,RCache.xforms.m_wvp);

		RCache.set_c( C, mTexgen);
	}
};
static cl_texgen		binder_texgen;

class cl_VPtexgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;

#ifdef USE_DX11
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f,				0.5f,				0.0f,			1.0f
		};
#else //USE_DX11
		float	_w						= float(RCache.get_width());
		float	_h						= float(RCache.get_height());
		float	o_w						= (.5f / _w);
		float	o_h						= (.5f / _h);
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f + o_w,			0.5f + o_h,			0.0f,			1.0f
		};
#endif

		mTexgen.mul	(mTexelAdjust,RCache.xforms.m_vp);

		RCache.set_c( C, mTexgen);
	}
};
static cl_VPtexgen		binder_VPtexgen;

// fog
#if 1 //ndef _EDITOR
class cl_fog_plane	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C)
	{
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker!=Device.dwFrame)
		{
			// Plane
			Fvector4		plane;
			Fmatrix&	M	= Device.mFullTransform;
			plane.x			= -(M._14 + M._13);
			plane.y			= -(M._24 + M._23);
			plane.z			= -(M._34 + M._33);
			plane.w			= -(M._44 + M._43);
			float denom		= -1.0f / _sqrt(_sqr(plane.x)+_sqr(plane.y)+_sqr(plane.z));
			plane.mul		(denom);

			// Near/Far
			float A			= g_pGamePersistent->Environment().CurrentEnv->fog_near;
			float fog_far	= g_pGamePersistent->Environment().CurrentEnv->fog_far;
			// COP_ENGINE_PATCH: COP-ENG-010
			// COP_ENGINE_PATCH: COP-ENG-013
			// Shift only visual weather fog in the fogged extension profiles. The
			// clear profile places it outside the camera. CurrentEnv remains authored,
			// so NPC vision and gameplay do not inherit the player's extra range.
			if (psFarViewProfile != PF_FAR_VIEW_STANDARD && psDeviceFlags.test(rsR4))
			{
				if (!PFFarViewHasVisualFog(psFarViewProfile))
				{
					A = g_pGamePersistent->Environment().CurrentEnv->far_plane + 1.f;
					fog_far = A + 1.f;
				}
				else
				{
					// COP_ENGINE_PATCH: COP-ENG-017
					// COP_ENGINE_PATCH: COP-ENG-018
					// Fixed-wall profiles ignore authored fog_distance because it can
					// exceed the weather far plane by more than a kilometre.
					if (!PFFarViewAbsoluteVisualFog(psFarViewProfile, A, fog_far))
					{
						// COP_ENGINE_PATCH: COP-ENG-014
						// Scale the shader-only weather fog together with the fixed 0.65
						// camera horizon. CurrentEnv remains authored for gameplay systems.
						const float fog_scale = PFFarViewDistanceScale(psFarViewProfile);
						const float fog_extension = PFFarViewFogExtension(psFarViewProfile);
						A = A * fog_scale + fog_extension;
						fog_far = fog_far * fog_scale + fog_extension;
					}
				}
			}
			float B			= 1/(fog_far - A);
			result.set		(-plane.x*B, -plane.y*B, -plane.z*B, 1 - (plane.w-A)*B	);								// view-plane
		}
		RCache.set_c	(C,result);
	}
};
static cl_fog_plane		binder_fog_plane;

// fog-params
class cl_fog_params	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C)
	{
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker!=Device.dwFrame)
		{
			// Near/Far
			float	n		= g_pGamePersistent->Environment().CurrentEnv->fog_near;
			float	f		= g_pGamePersistent->Environment().CurrentEnv->fog_far;
			// COP_ENGINE_PATCH: COP-ENG-010
			// COP_ENGINE_PATCH: COP-ENG-013
			// Only shader-visible fog is shifted or disabled. Gameplay continues to
			// read the unmodified CurrentEnv fog_near/fog_far values.
			if (psFarViewProfile != PF_FAR_VIEW_STANDARD && psDeviceFlags.test(rsR4))
			{
				if (!PFFarViewHasVisualFog(psFarViewProfile))
				{
					n = g_pGamePersistent->Environment().CurrentEnv->far_plane + 1.f;
					f = n + 1.f;
				}
				else
				{
					// COP_ENGINE_PATCH: COP-ENG-017
					// COP_ENGINE_PATCH: COP-ENG-018
					if (!PFFarViewAbsoluteVisualFog(psFarViewProfile, n, f))
					{
						// COP_ENGINE_PATCH: COP-ENG-014
						const float fog_scale = PFFarViewDistanceScale(psFarViewProfile);
						const float fog_extension = PFFarViewFogExtension(psFarViewProfile);
						n = n * fog_scale + fog_extension;
						f = f * fog_scale + fog_extension;
					}
				}
			}
			float	r		= 1/(f-n);
			result.set		(-n*r, n, f, r);
		}
		RCache.set_c	(C,result);
	}
};	static cl_fog_params	binder_fog_params;

// fog-color
class cl_fog_color	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
#if RENDER == R_R1
			result.set(desc.fog_color.x * ps_r1_fog_luminance, desc.fog_color.y * ps_r1_fog_luminance, desc.fog_color.z * ps_r1_fog_luminance, 0);
#else
			result.set(desc.fog_color.x, desc.fog_color.y, desc.fog_color.z, 0);
#endif // RENDER==R_R1
		}
		RCache.set_c	(C,result);
	}
};	static cl_fog_color		binder_fog_color;
#endif

// times
class cl_times		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		float 		t	= RDEVICE.fTimeGlobal;
		RCache.set_c	(C,t,t*10,t/10,_sin(t))	;
	}
};
static cl_times		binder_times;

// eye-params
class cl_eye_P		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= RDEVICE.vCameraPosition;
		RCache.set_c	(C,V.x,V.y,V.z,1);
	}
};
static cl_eye_P		binder_eye_P;

// eye-params
class cl_eye_D		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= RDEVICE.vCameraDirection;
		RCache.set_c	(C,V.x,V.y,V.z,0);
	}
};
static cl_eye_D		binder_eye_D;

// eye-params
class cl_eye_N		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= RDEVICE.vCameraTop;
		RCache.set_c	(C,V.x,V.y,V.z,0);
	}
};
static cl_eye_N		binder_eye_N;

// eye-params
class cl_hud_project : public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		RCache.set_c (C, Device.mProject_hud);
	}
};
static cl_hud_project binder_hud_project;

#if 1 //ndef _EDITOR
// TAA Jiter
class cl_taa_jitter : public R_constant_setup {
	virtual void setup(R_constant* C) {
		Fvector& V = ps_r_taa_jitter;
		// COP_ENGINE_PATCH: COP-ENG-011 - world keeps TAA jitter, HUD stays spatially stable for SMAA.
		const bool suppressHudJitter = ps_r2_aa_type == 4 && ps_r_scale_mode < 2 && RDEVICE.hudViewportData.isRenderProcess;
		RCache.set_c(C, suppressHudJitter ? 0.0f : V.x, suppressHudJitter ? 0.0f : V.y, V.z, float(RDEVICE.dwFrame));
	}
};
static cl_taa_jitter binder_taa_jitter;
#endif

#if 1 //ndef _EDITOR
// D-Light0
class cl_sun0_color : public R_constant_setup {
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C) {
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker != Device.dwFrame) {
			CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
#if defined(_EDITOR) || RENDER != R_R1
			result.set(desc.sun_color.x * ps_r2_sun_lumscale, desc.sun_color.y * ps_r2_sun_lumscale, desc.sun_color.z * ps_r2_sun_lumscale, 0);
#else
			result.set(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z, 0);
#endif
		}
		RCache.set_c(C, result);
	}
};	static cl_sun0_color binder_sun0_color;

class cl_sun0_dir_w : public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C) {
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker != Device.dwFrame) {
			CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc.sun_dir.x, desc.sun_dir.y, desc.sun_dir.z, 0);
		}
		RCache.set_c(C, result);
	}
};	static cl_sun0_dir_w binder_sun0_dir_w;

class cl_sun0_dir_e : public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C) {
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker != Device.dwFrame) {
			Fvector D;
			CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
			Device.mView.transform_dir(D, desc.sun_dir);
			D.normalize();
			result.set(D.x, D.y, D.z, 0);
		}
		RCache.set_c(C, result);
	}
};	static cl_sun0_dir_e binder_sun0_dir_e;

class cl_amb_color : public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C) {
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker != Device.dwFrame) {
			CEnvDescriptorMixer& desc = *g_pGamePersistent->Environment().CurrentEnv;

#if defined(_EDITOR) || RENDER != R_R1
			result.set(desc.ambient.x * ps_r2_sun_lumscale_amb * 2.0f,
				desc.ambient.y * ps_r2_sun_lumscale_amb * 2.0f, desc.ambient.z * ps_r2_sun_lumscale_amb * 2.0f, desc.weight);
#else
			result.set(desc.ambient.x, desc.ambient.y, desc.ambient.z, desc.weight);
#endif
		}
		RCache.set_c(C, result);
	}
};	static cl_amb_color binder_amb_color;

class cl_hemi_color : public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C) {
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker != Device.dwFrame) {
			CEnvDescriptorMixer& desc = *g_pGamePersistent->Environment().CurrentEnv;
#if defined(_EDITOR) || RENDER != R_R1
			result.set(desc.hemi_color.x * ps_r2_sun_lumscale_hemi * 4.0f,
				desc.hemi_color.y * ps_r2_sun_lumscale_hemi * 4.0f, desc.hemi_color.z * ps_r2_sun_lumscale_hemi * 4.0f, desc.weight);
#else
			result.set(desc.hemi_color);
#endif
		}
		RCache.set_c(C, result);
	}
}; static cl_hemi_color binder_hemi_color;
#endif

class cl_sky_color : public R_constant_setup {
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C) {
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		if (marker != Device.dwFrame) {
			CEnvDescriptorMixer& desc = *g_pGamePersistent->Environment().CurrentEnv;
#if defined(_EDITOR) || RENDER != R_R1
			result.set(desc.sky_color.x * ps_r2_sun_lumscale_sky, 
				desc.sky_color.y * ps_r2_sun_lumscale_sky, desc.sky_color.z * ps_r2_sun_lumscale_sky, desc.sky_rotation);
#else
			result.set(desc.sky_color.x, desc.sky_color.y, desc.sky_color.z, desc.sky_rotation);
#endif
		}
		RCache.set_c(C, result);
	}
}; static cl_sky_color binder_sky_color;

static class cl_screen_res : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		RCache.set_c(C, RCache.get_target_width(), RCache.get_target_height(), 1.0f / RCache.get_target_width(), 1.0f / RCache.get_target_height());
	}
}	binder_screen_res;

static class cl_scaled_screen_res : public R_constant_setup {
	virtual void setup(R_constant* C) {
		RCache.set_c(C, RCache.get_width(), RCache.get_height(), 1.0f / RCache.get_width(), 1.0f / RCache.get_height());
	}
}	binder_scaled_screen_res;

static class cl_target_screen_res : public R_constant_setup {
	virtual void setup(R_constant* C) {
		RCache.set_c(C, (float)::Render->getTarget()->get_width(), (float)::Render->getTarget()->get_height(),
			1.0f / (float)::Render->getTarget()->get_width(), 1.0f / (float)::Render->getTarget()->get_height());
	}
}	binder_target_screen_res;

static class cl_screen_scale : public R_constant_setup {
	virtual void setup(R_constant* C) {
		RCache.set_c(C, RDEVICE.RenderScale, 0.0f, 0.0f, 0.0f);
	}
} binder_screen_scale;

static class cl_def_aref : public R_constant_setup
{
	virtual void setup(R_constant* C) override {
#ifdef _EDITOR
		float def_aref_cmd = 100 / 255.0f;
#else
		float def_aref_cmd = ps_r2_def_aref_quality / 255.0f;
#endif
	#ifdef USE_DX11
		RCache.set_c(C, def_aref_cmd);
	#else
		RCache.set_c(C, def_aref_cmd, 0.0f, 0.0f, 0.0f);
	#endif
	}
} binder_def_aref;

static class cl_rain_params : public R_constant_setup {
	u32 marker;
	Fvector4 result;

	virtual void setup(R_constant* C)
	{
#ifdef _EDITOR
		if (!g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv) {
			RCache.set_c(C, 0, 0, 0.0f, 0.0f);
			return;
		}
#endif
		float rainDensity = g_pGamePersistent->Environment().CurrentEnv->rain_density;
		float rainWetness = g_pGamePersistent->Environment().wetness_factor;
		RCache.set_c(C, rainDensity, rainWetness, 0.0f, (float)g_pGameLevel->UseSnowmask);
	}
} binder_rain_params;

static class cl_inv_v : public R_constant_setup
{
	u32	marker;
	Fmatrix	result;

	virtual void setup(R_constant* C)
	{
		result.invert(Device.mView);

		RCache.set_c(C, result);
	}
} binder_inv_v;

static class cl_m_hud_params : public R_constant_setup
{
	virtual void setup(R_constant* C) {
		RCache.set_c(C, RDEVICE.hudViewportData.isRenderProcess, RDEVICE.hudViewportData.isRenderActive, 0.0f, RDEVICE.hudViewportData.renderZoomRotateFactor);
	}
}    binder_m_hud_params;

static class cl_m_hybrid_aa_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		// COP_ENGINE_PATCH: COP-ENG-011 - x marks the HUD G-buffer pass; y enables the hybrid resolve.
		const bool hybridEnabled = (ssfx_active() ? ssfx_hybrid_aa() : ps_r2_aa_type == 4) && ps_r_scale_mode < 2; // COP_ENGINE_PATCH: COP-ENG-128
		RCache.set_c(C, hybridEnabled && RDEVICE.hudViewportData.isRenderProcess, hybridEnabled, 0.0f, 0.0f);
	}
} binder_m_hybrid_aa_params;

static class cl_m_zoom_deviation : public R_constant_setup
{
	virtual void setup(R_constant* C) {
		RCache.set_c(C, 0.0f, 0.0f, RDEVICE.hudViewportData.renderScopeBrightnessValue, RDEVICE.hudViewportData.renderScopeBrightnessJitterValue);
	}
} binder_m_zoom_deviation;

static class cl_affects : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float decr = 0.0f;

		if (RDEVICE.hudViewportData.IsElectronicsProblemsDecreasing)
			decr = 1.0f;

		RCache.set_c(C, RDEVICE.hudViewportData.CurrentElectronicsProblemsCnt/10.0f, ::Random.randF(0.0f, 1.0f), RDEVICE.hudViewportData.TargetElectronicsProblemsCnt/10.0f, decr);
	}
} binder_affects;

// COP_ENGINE_PATCH: COP-ENG-168 - late HDR bodycam, outside menu and scope viewports.
#ifdef USE_DX11
static class cl_bodycam_control : public R_constant_setup {
    void setup(R_constant* C) override {
        const bool active=ps_r4_bodycam.test(1) && ps_r4_bodycam_state.x>0 && g_pGameLevel &&
            !RDEVICE.hudViewportData.isRenderProcess &&
            !(g_pGamePersistent && g_pGamePersistent->OnRenderPPUI_query()) &&
            u32(Device.dwTimeGlobal-ps_r4_bodycam_update_time)<1000;
        RCache.set_c(C,active?1.f:0.f,0.f,0.f,0.f);
    }
} binder_bodycam_control;
static class cl_bodycam_state : public R_constant_setup {
    void setup(R_constant* C) override { RCache.set_c(C,ps_r4_bodycam_state); }
} binder_bodycam_state;
#endif

static class cl_actor_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, RDEVICE.hudViewportData.ActorHealth, RDEVICE.hudViewportData.ActorOutfitCondition, RDEVICE.hudViewportData.ActorWeaponCondition, RDEVICE.hudViewportData.ActorWeaponLoading);
	}
} binder_actor_states;

static class cl_m_timearrow : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
		split_time(g_pGameLevel->GetGameTime(), year, month, day, hours, mins, secs, milisecs);

		float s_f = secs / 60.f;
		float s_angle = PI_MUL_2 * s_f;

		float m_f = (s_f + float(mins)) / 60.f;
		float m_angle = PI_MUL_2 * m_f;

		float h_f = (m_f + float(hours)) / 12.f;
		float h_angle = PI_MUL_2 * h_f;

		RCache.set_c(C, sin(h_angle), cos(h_angle), sin(m_angle), cos(m_angle));
	}
} binder_m_timearrow;

static class cl_m_timearrow2 : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
		split_time(g_pGameLevel->GetGameTime(), year, month, day, hours, mins, secs, milisecs);

		float s_f = secs / 60.f;
		float s_angle = PI_MUL_2 * s_f;

		float h, p;
		RDEVICE.vCameraDirection.getHP(h, p);

		RCache.set_c(C, sin(s_angle), cos(s_angle), sin(h), cos(h));
	}
} binder_m_timearrow2;

static class cl_digiclock : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
		split_time(g_pGameLevel->GetGameTime(), year, month, day, hours, mins, secs, milisecs);

		float hh = (hours / 10) / 10.0f;
		float hl = (hours % 10) / 10.0f;
		float mh = (month / 10) / 10.0f;
		float ml = (month % 10) / 10.0f;

		RCache.set_c(C, hh, hl, mh, ml);
	}
} binder_digiclock;

// Standart constant-binding
void	CBlender_Compile::SetMapping()
{
	// matrices
	r_Constant("m_W", &binder_w);
	r_Constant("m_invW", &binder_invw);
	r_Constant("m_V", &binder_v);
	r_Constant("m_P", &binder_p);
	r_Constant("m_WV", &binder_wv);
	r_Constant("m_VP", &binder_vp);
	r_Constant("m_WVP", &binder_wvp);
	r_Constant("m_invV", &binder_invv);

	r_Constant("m_P_hud", &binder_hud_project);

#ifdef USE_DX11
	r_Constant("m_W_old", &binder_w_old);
	r_Constant("m_V_old", &binder_v_old);
	r_Constant("m_P_old", &binder_p_old);
	r_Constant("m_WV_old", &binder_wv_old);
	r_Constant("m_VP_old", &binder_vp_old);
	r_Constant("m_WVP_old", &binder_wvp_old);
#endif

	r_Constant("m_xform_v", &tree_binder_m_xform_v);
	r_Constant("m_xform", &tree_binder_m_xform);

	r_Constant("consts", &tree_binder_consts);
	r_Constant("wave", &tree_binder_wave);
	r_Constant("wind", &tree_binder_wind);

#ifdef USE_DX11
	r_Constant("consts_old", &tree_binder_consts_old);
	r_Constant("wave_old", &tree_binder_wave_old);
	r_Constant("wind_old", &tree_binder_wind_old);
#endif

	r_Constant("c_scale", &tree_binder_c_scale);
	r_Constant("c_bias", &tree_binder_c_bias);
	r_Constant("c_sun", &tree_binder_c_sun);

	//hemi cube
	r_Constant("L_material", &binder_material);
	r_Constant("hemi_cube_pos_faces", &binder_hemi_cube_pos_faces);
	r_Constant("hemi_cube_neg_faces", &binder_hemi_cube_neg_faces);

	//	Igor	temp solution for the texgen functionality in the shader
	r_Constant("m_texgen", &binder_texgen);
	r_Constant("mVPTexgen", &binder_VPtexgen);

	// fog-params
	r_Constant("fog_plane", &binder_fog_plane);
	r_Constant("fog_params", &binder_fog_params);
	r_Constant("fog_color", &binder_fog_color);

	r_Constant("timers", &binder_times);

	// eye-params
	r_Constant("eye_position", &binder_eye_P);
	r_Constant("eye_direction", &binder_eye_D);
	r_Constant("eye_normal", &binder_eye_N);

	r_Constant("L_sun_color", &binder_sun0_color);
	r_Constant("L_sun_dir_w", &binder_sun0_dir_w);
	r_Constant("L_sun_dir_e", &binder_sun0_dir_e);

	r_Constant("m_taa_jitter", &binder_taa_jitter);
#ifdef USE_DX11
    // COP_ENGINE_PATCH: COP-ENG-126
    r_Constant("pf_sss_control", &binder_ssfx_control);
    r_Constant("pf_sss_sun_ready", &binder_sss_sun_ready);
    r_Constant("pf_sss_local_ready", &binder_sss_local_ready);
    r_Constant("pf_sss_local_lights", &binder_sss_local_lights);
    r_Constant("pf_sss_terrain_map", &binder_sss_terrain_map);
    r_Constant("pf_sss_lut_state", &binder_sss_lut_transition);
    r_Constant("pf_sss_bloom_weather", &binder_sss_bloom_weather);
    r_Constant("pf_sss_hud_rain_state", &binder_sss_hud_rain);
    r_Constant("pf_sss_weather_rain", &binder_pf_sss_weather_rain);
    r_Constant("pf_sss_wind", &binder_ssfx_wind);
    r_Constant("pf_sss_wind_anim", &binder_sss_wind_animation);
    r_Constant("pf_sss_wind_anim_previous", &binder_sss_wind_previous);
    r_Constant("pf_sss_benders", &binder_sss_benders);
    r_Constant("pf_sss_benders_previous", &binder_sss_benders_previous);
    r_Constant("pf_sss_shadow", &binder_ssfx_shadow);
    r_Constant("pf_sss_quality", &binder_ssfx_quality);
    r_Constant("pf_sss_ao", &binder_ssfx_ao);
    r_Constant("pf_sss_ao2", &binder_ssfx_ao2);
    r_Constant("pf_sss_options0", &binder_ssfx_options0);
    r_Constant("pf_sss_options1", &binder_ssfx_options1);
    r_Constant("pf_sss_options2", &binder_ssfx_options2);
    r_Constant("pf_sss_options3", &binder_ssfx_options3);
    r_Constant("pf_sss_options4", &binder_ssfx_options4);
    r_Constant("pf_sss_options5", &binder_ssfx_options5);
    r_Constant("pf_sss_options6", &binder_ssfx_options6);
    r_Constant("pf_sss_options7", &binder_ssfx_options7);
    r_Constant("pf_sss_options8", &binder_ssfx_options8);
    r_Constant("pf_sss_options9", &binder_ssfx_options9);
    r_Constant("pf_sss_options10", &binder_ssfx_options10);
    r_Constant("pf_sss_options11", &binder_ssfx_options11);
    r_Constant("pf_sss_options12", &binder_ssfx_options12);
    r_Constant("pf_sss_options13", &binder_ssfx_options13);
    r_Constant("pf_sss_options14", &binder_ssfx_options14);
    r_Constant("pf_sss_options15", &binder_ssfx_options15);
    r_Constant("pf_sss_options16", &binder_ssfx_options16);
    r_Constant("pf_sss_options17", &binder_ssfx_options17);
    r_Constant("pf_sss_options18", &binder_ssfx_options18);
    r_Constant("pf_sss_options19", &binder_ssfx_options19);
    r_Constant("pf_sss_options20", &binder_ssfx_options20);
    r_Constant("pf_sss_options21", &binder_ssfx_options21);
    r_Constant("pf_sss_options22", &binder_ssfx_options22);
    r_Constant("pf_sss_options23", &binder_ssfx_options23);
    r_Constant("pf_sss_options24", &binder_ssfx_options24);
    r_Constant("pf_sss_options25", &binder_ssfx_options25);
    r_Constant("pf_sss_options26", &binder_ssfx_options26);
    r_Constant("pf_sss_options27", &binder_ssfx_options27);
    r_Constant("pf_sss_options28", &binder_ssfx_options28);
    r_Constant("pf_sss_options29", &binder_ssfx_options29);
    r_Constant("pf_sss_options30", &binder_ssfx_options30);
    r_Constant("pf_sss_options31", &binder_ssfx_options31);
    r_Constant("pf_sss_options32", &binder_ssfx_options32);
    r_Constant("pf_sss_options33", &binder_ssfx_options33);
    r_Constant("pf_sss_options34", &binder_ssfx_options34);
    r_Constant("pf_sss_options35", &binder_ssfx_options35);
    r_Constant("pf_sss_options36", &binder_ssfx_options36);
    r_Constant("pf_sss_options37", &binder_ssfx_options37);
    r_Constant("pf_sss_options38", &binder_ssfx_options38);
    r_Constant("pf_sss_options39", &binder_ssfx_options39);
    r_Constant("pf_sss_options40", &binder_ssfx_options40);
    r_Constant("pf_sss_options41", &binder_ssfx_options41);
    r_Constant("pf_sss_options42", &binder_ssfx_options42);
    r_Constant("pf_sss_options43", &binder_ssfx_options43);
    r_Constant("pf_sss_options44", &binder_ssfx_options44);
    r_Constant("pf_sss_options45", &binder_ssfx_options45);
    r_Constant("pf_sss_options46", &binder_ssfx_options46);
    r_Constant("pf_sss_options47", &binder_ssfx_options47);
    r_Constant("pf_sss_options48", &binder_ssfx_options48);
    r_Constant("pf_sss_options49", &binder_ssfx_options49);
    r_Constant("pf_sss_options50", &binder_ssfx_options50);
    r_Constant("pf_sss_options51", &binder_ssfx_options51);
    r_Constant("pf_sss_options52", &binder_ssfx_options52);
    r_Constant("pf_sss_options53", &binder_ssfx_options53);
    r_Constant("pf_sss_options54", &binder_ssfx_options54);
    r_Constant("pf_sss_options55", &binder_ssfx_options55);
    r_Constant("pf_sss_options56", &binder_ssfx_options56);
    r_Constant("pf_sss_options57", &binder_ssfx_options57);
    r_Constant("pf_sss_options58", &binder_ssfx_options58);
    r_Constant("pf_sss_options59", &binder_ssfx_options59);
    r_Constant("pf_sss_options60", &binder_ssfx_options60);
    r_Constant("pf_sss_options61", &binder_ssfx_options61);
#endif

	r_Constant("L_sky_color", &binder_sky_color);

	r_Constant("L_hemi_color", &binder_hemi_color);
	r_Constant("L_ambient", &binder_amb_color);

	r_Constant("screen_res", &binder_screen_res);
	r_Constant("def_aref", &binder_def_aref);

	r_Constant("scaled_screen_res", &binder_scaled_screen_res);
	r_Constant("target_screen_res", &binder_target_screen_res);

	r_Constant("screen_scale", &binder_screen_scale);
	
	// Rain
	r_Constant				("rain_params",		&binder_rain_params);

	//LVutner: Gunslinger...
	r_Constant("m_hud_params", &binder_m_hud_params);
	r_Constant("m_hybrid_aa_params", &binder_m_hybrid_aa_params); // COP_ENGINE_PATCH: COP-ENG-011
	r_Constant("m_zoom_deviation", &binder_m_zoom_deviation);
	r_Constant("m_affects", &binder_affects);
	r_Constant("m_actor_params", &binder_actor_states);
	// COP_ENGINE_PATCH: COP-ENG-172
	r_Constant("game_time", &watchTime172);
	r_Constant("watch_actor_params_1", &watchActor172);
	r_Constant("watch_actor_params_2", &watchCondition172);
	r_Constant("watch_color_params", &watchColor172);
#ifdef USE_DX11
    // COP_ENGINE_PATCH: COP-ENG-168
    r_Constant("pf_bodycam_control", &binder_bodycam_control);
    r_Constant("pf_bodycam_state", &binder_bodycam_state);
#endif
	r_Constant("m_timearrow", &binder_m_timearrow);
	r_Constant("m_timearrow2", &binder_m_timearrow2);
	r_Constant("m_digiclock", &binder_digiclock);

	// detail
	//if (bDetail	&& detail_scaler)
	//	Igor: bDetail can be overridden by no_detail_texture option.
	//	But shader can be deatiled implicitly, so try to set this parameter
	//	anyway.
	if (detail_scaler)
		r_Constant			("dt_params",		detail_scaler);

	// other common
	for (u32 it=0; it<DEV->v_constant_setup.size(); it++)
	{
		std::pair<shared_str,R_constant_setup*>	cs	= DEV->v_constant_setup[it];
		r_Constant			(*cs.first,cs.second);
	}
}
