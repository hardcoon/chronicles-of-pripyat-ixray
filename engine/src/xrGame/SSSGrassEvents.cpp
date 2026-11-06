#include "StdAfx.h"
#include "SSSGameEffects.h"
#include "SSSGrassAnimation.h"
#include "Actor.h"
#include "CustomZone.h"
#include "Level.h"
#include "../xrEngine/xr_ioconsole.h"
// COP_ENGINE_PATCH: COP-ENG-128 - SSS 23.5 events by ascii1457, private IX-Ray adapter.
namespace {
struct GrassEvent {
    Fvector position={},direction={};SSSGrassAnimation animation;
    u32 source=0,serial=0,frame=0;bool active=false,zone=false;
};
GrassEvent events[8];u32 nextSerial=1;
xr_map<u16,u32> zoneBlowoutUntil;
float grassValue(LPCSTR name){float low=0,high=0;return Console->GetFloat(name,low,high);}
u32 eventLimit(){return clampr(u32(grassValue("ssfx_grass_entities")),0u,8u);}
bool ownsGrass(){return Console && g_pGameLevel && Actor() && Console->GetBool("ssfx_enabled") && grassValue("ssfx_grass_enabled")>.5f && eventLimit()>0;}
bool inRange(const Fvector& position,float radius=0){return Actor()->Position().distance_to_xz_sqr(position)<=_sqr(grassValue("ssfx_grass_distance")+radius);}
float zoneValue(CCustomZone* zone,LPCSTR field,float fallback){return pSettings->line_exist(zone->cNameSect(),field)?pSettings->r_float(zone->cNameSect(),field):fallback;}
bool zoneBool(CCustomZone* zone,LPCSTR field){return pSettings->line_exist(zone->cNameSect(),field) && pSettings->r_bool(zone->cNameSect(),field);}
GrassEvent* findEvent(u32 source,bool zone) {
    for(u32 i=0;i<eventLimit();++i)if(events[i].active && events[i].source==source && events[i].zone==zone)return &events[i];
    return nullptr;
}
GrassEvent* allocate(bool replaceZone=false) {
    for(u32 i=0;i<eventLimit();++i)if(!events[i].active || (replaceZone && events[i].zone))return &events[i];
    return nullptr;
}
void initialize(GrassEvent& e,u32 source,const Fvector& position,const Fvector& direction,float radius,float strength,float speed,float fade,bool zone) {
    e=GrassEvent();e.position=position;e.direction=direction;e.source=source;e.zone=zone;e.active=true;
    e.serial=nextSerial++;if(nextSerial>65535)nextSerial=1;
    e.animation.radius=radius;e.animation.target=strength;e.animation.speed=speed;e.animation.fade=fade;e.frame=Device.dwFrame;
}
// Two-octave 1D Perlin curve with a private hash; never reseed gameplay's rand/Random.
float gradient(int n){u32 h=u32(n)*1664525u+1013904223u;h^=h>>13;h*=1274126177u;return float(h&511)/256.f-1.f;}
float noise1(float x){const int n=int(std::floor(x));const float t=x-n,s=t*t*(3-2*t);return (1-s)*t*gradient(n)+s*(t-1)*gradient(n+1);}
float grassNoise(float x){return noise1(x)*.66666f+noise1(x*2)*.33333f;}
void updateZone(CCustomZone* zone,float dt) {
    if(!zone->IsEnabled() || zone->getDestroy() || !inRange(zone->Position()) ||
       Actor()->Position().distance_to_xz_sqr(zone->Position())>_sqr(grassValue("ssfx_grass_anomaly_distance")))return;
    const int idle=int(zoneValue(zone,"pf_sss_bend_grass_idle_anim",-1));
    const int active=int(zoneValue(zone,"pf_sss_bend_grass_whenactive_anim",-1));
    if(idle<0 && active<0)return;
    const auto timer=zoneBlowoutUntil.find(zone->ID());
    const bool triggered=timer!=zoneBlowoutUntil.end()?timer->second>Device.dwTimeGlobal:zone->ZoneState()!=CCustomZone::eZoneStateIdle;
    const int wanted=triggered && active>=0?active:idle;
    GrassEvent* e=findEvent(zone->ID(),true);
    if(!e && wanted<=0)return;
    if(!e){e=allocate();if(!e)return;initialize(*e,zone->ID(),zone->Position(),Fvector().set(0,-99,0),zoneValue(zone,"pf_sss_bend_grass_idle_radius",1),0,1,0,true);e->animation.kind=wanted;}
    e->frame=Device.dwFrame;e->position=zone->Position();auto& a=e->animation;
    if(a.kind!=wanted){a.target=SSSGrassEase(a.target,0,7.5f,dt,false);if(a.target<=.05f)a.kind=wanted;}
    else {
        const float speed=zoneValue(zone,triggered?"pf_sss_bend_grass_whenactive_speed":"pf_sss_bend_grass_idle_speed",zoneValue(zone,"pf_sss_bend_grass_idle_speed",1));
        const float strength=zoneValue(zone,triggered?"pf_sss_bend_grass_whenactive_str":"pf_sss_bend_grass_idle_str",zoneValue(zone,"pf_sss_bend_grass_idle_str",1));
        a.speed=SSSGrassEase(a.speed,speed,10,dt);a.target=SSSGrassEase(a.target,wanted>0?strength:0,10,dt);
    }
}
}
void ResetSSSGrassEvents() {
    for(auto& e:events)e=GrassEvent();zoneBlowoutUntil.clear();
    if(!Console)return;
    for(u32 i=16;i<24;++i){string64 name;xr_sprintf(name,"ssfx_bender_metadata%u",i);if(auto* meta=Console->GetFVectorPtr(name))meta->set(0,0,0);}
}
void SSSGrassExplosion(u16 source,const Fvector& position,float radius,bool mutant) {
    if(!ownsGrass() || (mutant && grassValue("ssfx_grass_mutants")<.5f) || !inRange(position,radius))return;
    const float strength=mutant?1.f:grassValue("ssfx_grass_explosion_strength");if(strength<=0 || radius<=0)return;
    auto* e=allocate(true);if(!e)return;
    initialize(*e,source,position,Fvector().set(0,-99,0),radius,strength,mutant?5.f:grassValue("ssfx_grass_explosion_speed"),1.33f,false);
    e->animation.strength=strength;
}
void SSSGrassShot(u16 source,const Fvector& position,const Fvector& direction) {
    if(!ownsGrass() || !inRange(position))return;
    const float strength=grassValue("ssfx_grass_shooting_strength");if(strength<=0)return;
    auto* e=findEvent(source,false);
    if(e){e->position=position;e->direction=direction;e->animation.target+=strength;return;}
    e=allocate();if(e)initialize(*e,source,position,direction,grassValue("ssfx_grass_shooting_radius"),strength,20,3,false);
}
void SSSGrassZoneBlowout(CCustomZone* zone) {
    if(!zone || !ownsGrass() || grassValue("ssfx_grass_anomalies")<.5f || !zoneBool(zone,"pf_sss_bend_grass_blowout") || !inRange(zone->Position()))return;
    const float duration=zoneValue(zone,"pf_sss_bend_grass_blowout_duration",-1);
    if(duration>=0)zoneBlowoutUntil[zone->ID()]=Device.dwTimeGlobal+u32(duration);
    auto* e=allocate(true);if(!e)return;
    initialize(*e,zone->ID()+65536u,zone->Position(),Fvector().set(0,-99,0),zoneValue(zone,"pf_sss_bend_grass_blowout_radius",5),1,zoneValue(zone,"pf_sss_bend_grass_blowout_speed",5),1.33f,false);
    e->animation.strength=1;
}
void UpdateSSSGrassEvents(CActor* actor) {
    if(!actor || !ownsGrass()){ResetSSSGrassEvents();return;}
    const float dt=clampr(Device.fTimeDelta,0.f,.1f);const u32 count=eventLimit();
    if(grassValue("ssfx_grass_anomalies")>.5f) {
        for(u32 i=0;i<Level().Objects.o_count();++i)if(auto* zone=smart_cast<CCustomZone*>(Level().Objects.o_get_by_iterator(i)))updateZone(zone,dt);
    }
    for(auto it=zoneBlowoutUntil.begin();it!=zoneBlowoutUntil.end();) {
        // Expired entries retain the explicit-duration semantics until the zone is idle.
        if(it->second+30000u<Device.dwTimeGlobal)it=zoneBlowoutUntil.erase(it);else ++it;
    }
    for(u32 i=0;i<8;++i) {
        auto& e=events[i];if(i>=count || (e.zone && e.frame!=Device.dwFrame))e.active=false;
        if(e.active){SSSGrassAnimate(e.animation,dt,grassNoise(e.animation.time));if(!e.zone && e.animation.strength<0)e.active=false;}
        string64 p,m,d;xr_sprintf(p,"ssfx_bender_position%u",i+16);xr_sprintf(m,"ssfx_bender_metadata%u",i+16);xr_sprintf(d,"ssfx_bender_direction%u",i);
        auto* position=Console->GetFVectorPtr(p);auto* meta=Console->GetFVectorPtr(m);auto* direction=Console->GetFVectorPtr(d);
        if(!position || !meta || !direction)continue;
        *position=e.position;*direction=e.direction;meta->set(e.active?e.animation.currentRadius:0,e.animation.strength,float(e.serial));
    }
}
