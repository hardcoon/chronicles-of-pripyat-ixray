#pragma once
// COP_ENGINE_PATCH: COP-ENG-128 - optional private SSS game effects.
class CActor;
bool SSSRainStepOwns();
void SSSRainStep(CActor* actor,u32 kind,bool hudView);
void ResetSSSRainSounds();

class CCustomZone;
void ResetSSSGrassEvents();
void UpdateSSSGrassEvents(CActor* actor);
void SSSGrassExplosion(u16 source,const Fvector& position,float radius,bool mutant=false);
void SSSGrassShot(u16 source,const Fvector& position,const Fvector& direction);
void SSSGrassZoneBlowout(CCustomZone* zone);
