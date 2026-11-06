#pragma once
#include "../xrRender/Shader.h"

// COP_ENGINE_PATCH: COP-ENG-192 - retained terrain data has a level lifetime.
void SSSPuddleContactBegin(IReader* level, const xr_vector<ref_shader>& shaders);
void SSSPuddleContactVertices(u32 id, const void* bytes, u32 count, u32 stride, const D3DVERTEXELEMENT9* declaration);
void SSSPuddleContactIndices(u32 id, const u16* indices, u32 count);
void SSSPuddleContactFinish();
void SSSPuddleContactReset();
