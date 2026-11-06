#include "stdafx.h"
#include "Render.h"

ENGINE_API IScriptDebugRender* ScriptDRender = nullptr;

// COP-ENG-HF-001: retain the installed RelWithDebInfo data export in Release.
#ifndef DEBUG_DRAW
#undef bDebug
ENGINE_API BOOL bDebug = FALSE;
#define bDebug 0
#endif


//ENGINE_API	IRender_interface*	Render		= nullptr;

// resources
IRender_Light::~IRender_Light	()			{	
	::Render->light_destroy		(this	);
}
IRender_Glow::~IRender_Glow		()			{	
	::Render->glow_destroy		(this	);	
}
