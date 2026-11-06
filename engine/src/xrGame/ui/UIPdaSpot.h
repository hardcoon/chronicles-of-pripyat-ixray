#pragma once

#include "../../xrUI/Widgets/UIDialogWnd.h"
#include "../../xrUI/Widgets/UIWndCallback.h"

class CUIXml;
class CUI3tButton;
class CUIStatic;
class CUIEditBox;
class UIHint;
class CMapLocation;

class CUIPdaSpot : 
	public CUIDialogWnd, 
	public CUIWndCallback
{
	typedef CUIDialogWnd base_class;

	CUIStatic* m_background;
	CUIEditBox* m_editBox;
	CUI3tButton* m_btn_ok;
	CUI3tButton* m_btn_cancel;
    // COP_ENGINE_PATCH: COP-ENG-106 - GPS action and optional atlas dialog labels.
    CUI3tButton* m_btn_gps;
    CUIStatic* m_placeholder;
    CUIStatic* m_title;
    shared_str MarkerName() const;

	enum { marker_button_count = 8 };
	CUI3tButton* m_markerButtons[marker_button_count];
	shared_str m_markerSpotTypes[marker_button_count];

	bool m_mainWnd;
	LPCSTR m_levelName;
	Fvector m_position;
	u16 m_spotID;
	shared_str m_spotType;
	shared_str m_originalSpotType;
	shared_str m_defaultSpotType;

	CMapLocation* FindUserLocation() const;
	bool SetMarkerType(const shared_str& spot_type);
	// COP_ENGINE_PATCH: COP-ENG-160 - require one visibly selected ordinary marker icon.
	bool HasSelectedMarker() const;
	bool CanApply() const;

public:
	CUIPdaSpot();
	~CUIPdaSpot();

	void Init(u16 spot_id, LPCSTR level_name, Fvector pos, bool main_wnd);
	void InitControls();

	void OnAdd(CUIWindow* w, void* d);
    void OnAddGPS(CUIWindow* w, void* d);
    virtual void Update();
	void OnApply(CUIWindow* w, void* d);
	void OnExit(CUIWindow* w, void* d);
	void OnMarkerType(CUIWindow* w, void* d);
	void Exit();

	virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
	virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
	virtual CUIWindow* ui_cast_window() { return this; }
};
