#pragma once
#include <algorithm>
#include <cmath>
// COP_ENGINE_PATCH: COP-ENG-128 - original SSS grass curves, independent of game state.
struct SSSGrassAnimation {
    float time=0, radius=1, currentRadius=.01f, strength=.001f, target=1, speed=1, fade=1.33f;
    int kind=0; // Explosion, push, wavy, suck, blow, pulse.
};
inline float SSSGrassEase(float current,float target,float rate,float dt,bool easing=true) {
    const float diff=std::abs(current-target);
    const float amount=dt*rate*(easing?std::min(.5f,diff):1.f);
    return diff<=amount?target:current+(current<target?amount:-amount);
}
inline void SSSGrassAnimate(SSSGrassAnimation& a,float dt,float noise) {
    switch(a.kind) {
    case 0: {
        a.time+=dt*a.speed;a.currentRadius=a.radius*std::min(1.f,a.time);a.target=std::min(1.f,a.target);
        const float diff=std::max(.1f,std::abs(a.strength-a.target));
        if(a.target<=a.strength)a.strength-=dt*a.fade*diff;
        else {a.strength+=dt*a.speed*diff;if(a.strength>=a.target)a.target=0;}
        break;
    }
    case 2: {a.time+=dt*1.5f*a.speed;const float c=std::sin(a.time);a.strength=c*std::cos(c*1.4f)*1.8f*a.target;break;}
    case 3: a.time+=dt*a.speed;a.strength=-std::clamp(noise+.5f,0.f,1.f)*a.target;break;
    case 4: a.time+=dt*1.2f*a.speed;a.strength=std::clamp(noise+1.f,0.f,2.f)*.25f*a.target;break;
    case 5:
        a.time+=dt*a.speed;a.currentRadius=a.radius*std::min(1.f,a.time);
        if(a.currentRadius>=a.radius)a.strength=SSSGrassEase(a.strength,0,a.speed*.6f,dt);
        if(a.strength<=.001f){a.strength=a.target;a.currentRadius=0;a.time=0;}
        break;
    default:a.strength=SSSGrassEase(a.strength,a.target,2,dt);break;
    }
    if(a.kind!=0 && a.kind!=5)a.currentRadius=a.radius;
}
