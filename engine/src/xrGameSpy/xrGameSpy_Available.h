#pragma once
#include "xrGameSpy_MainDefs.h"

#include "gamespy/nonport.h"
#include "gamespy/common/gsAvailable.h"

extern "C"
{
//	EXPORT_FN_DECL(void,	GSIStartAvailableCheck, (const gsi_char * gamename));
	EXPORT_FN_DECL(void,	GSIStartAvailableCheckA, ());
	EXPORT_FN_DECL(GSIACResult, GSIAvailableCheckThink, ());
	EXPORT_FN_DECL(void, msleep, (gsi_time msec));
}