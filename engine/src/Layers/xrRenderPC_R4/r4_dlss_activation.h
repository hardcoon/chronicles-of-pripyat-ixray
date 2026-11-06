#pragma once
#include "r4_dlss_history.h"
#include <cstring>
// COP_ENGINE_PATCH: COP-ENG-177 - NGX lifetime follows a ready, contiguous scene.
namespace dlss_activation {
inline bool finite(float value) {
    std::uint32_t bits; std::memcpy(&bits,&value,sizeof(bits));
    return (bits&0x7f800000u)!=0x7f800000u;
}
inline bool valid_camera(const DLSSFrameState& frame) {
    if (!frame.width || !frame.height || !finite(frame.fov) || frame.fov<=0.f || frame.fov>=180.f) return false;
    float norm=0.f;
    for (unsigned i=0;i<3;++i) {
        if (!finite(frame.position[i]) || !finite(frame.direction[i])) return false;
        norm+=frame.direction[i]*frame.direction[i];
    }
    return norm>.25f && norm<4.f;
}
class SceneGate {
    bool seen=false;
    std::uint32_t previous=0;
    unsigned stable=0;
public:
    bool update(std::uint32_t frame,bool eligible) {
        if (!eligible) { seen=false;stable=0;return false; }
        if (!seen) stable=1;
        else if (frame!=previous) stable=frame-previous==1u ? (stable<2 ? stable+1:2):1;
        seen=true;previous=frame;return stable>=2;
    }
};
enum class Action { Defer, Create, Recreate, Draw };
inline Action action(bool ready,bool created,bool configurationChanged) {
    if (!ready) return Action::Defer;
    if (!created) return Action::Create;
    return configurationChanged ? Action::Recreate:Action::Draw;
}
}
