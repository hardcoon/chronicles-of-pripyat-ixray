#pragma once

#include "../../xrGameSpy/gamespy/common/gsCommon.h"
#include "../../xrGameSpy/gamespy/common/gsAvailable.h"
#include "../../xrGameSpy/gamespy/ghttp/ghttp.h"

#include "../../xrGameSpy/gamespy/qr2/qr2.h"
#include "../../xrGameSpy/gamespy/gcdkey/gcdkeyc.h"
#include "../../xrGameSpy/gamespy/gcdkey/gcdkeys.h"
#include "../../xrGameSpy/gamespy/serverbrowsing/sb_serverbrowsing.h"

//#include "../../xrGameSpy/GameSpy/pt/pt.h"
#include "../../xrGameSpy/gamespy/GP/gp.h"
#include "../../xrGameSpy/gamespy/sake/sake.h"
#include "../../xrGameSpy/gamespy/sc/sc.h"

#undef max

extern "C" {

#define GAMESPY_TFN_DECL(r, f, p) typedef DLL_API r __cdecl t_fn_xrGS_##f p

};

#define GAMESPY_FN_VAR_DECL(r, f, p) GAMESPY_TFN_DECL(r, f, p); t_fn_xrGS_##f* xrGS_##f;
#define GAMESPY_LOAD_FN(f)  if (Engine.External.hGameSpy != 0) {  f = (t_fn_##f*)GetProcAddress(Engine.External.hGameSpy, #f); R_ASSERT2(f, "No such func in xrGameSpy.dll"); }
