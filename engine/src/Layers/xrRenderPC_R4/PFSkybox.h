#pragma once

class CRenderTarget;

// COP_ENGINE_PATCH: COP-ENG-244 COP-ENG-246: v1/v2 level data; no ALife/save registration.
void PFSkyboxLoad();
void PFSkyboxUnload();
void PFSkyboxResetGPU();
void PFSkyboxRender(CRenderTarget& target);
