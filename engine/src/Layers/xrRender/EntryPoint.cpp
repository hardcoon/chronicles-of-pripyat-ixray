#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxUIRender.h"
#include "dxDebugRender.h"
#include "dxScriptDebugRender.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		::Render = &RImplementation;
		::RenderFactory = &RenderFactoryImpl;
		::DU = &DUImpl;
		UIRender = &UIRenderImpl;
#ifdef USE_DX11
        ScriptDRender = &ScriptDebugRenderImpl;
#endif

#ifdef DEBUG_DRAW
		DRender = &DebugRenderImpl;
#endif // DEBUG

		xrRender_initconsole();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
#ifdef USE_DX11
        if (ScriptDRender == &ScriptDebugRenderImpl) ScriptDRender = nullptr;
#endif
		break;
	}
	return TRUE;
}
