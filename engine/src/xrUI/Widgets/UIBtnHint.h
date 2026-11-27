#pragma once
#include "UIFrameWindow.h"

class CUITextWnd;

class UI_API CUIButtonHint :
	public CUIFrameWindow
{
	CUIWindow*			m_ownerWnd;

	CUITextWnd*			m_text;
	bool				m_enabledOnFrame;
    bool                m_frameOnDevice = false;
public:
					CUIButtonHint	();
	virtual			~CUIButtonHint	();
	CUIWindow*		Owner			()	{return m_ownerWnd;}
	void			Discard			()	{m_ownerWnd=NULL;};
	void			OnRender		();
	void			Draw_			();
	void			SetHintText		(CUIWindow* w, LPCSTR text);

	virtual CUIWindow* ui_cast_window() { return this; }

};

extern UI_API CUIButtonHint* g_btnHint; 
extern UI_API CUIButtonHint* g_statHint;
