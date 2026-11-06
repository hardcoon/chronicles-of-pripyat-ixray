#include "stdafx.h"
#include "UIPdaSpot.h"
#include "PdaGpsLayout.h"
#include <dinput.h>
#include "Level.h"
#include "map_manager.h"
#include "map_location.h"
#include "GameTask.h"
#include "GametaskManager.h"
#include "../../xrUI/Widgets/UIEditBox.h"
#include "../../xrUI/Widgets/UIStatic.h"
#include "../../xrUI/UIXmlInit.h"
#include "../../xrUI/UIHelper.h"
#include "../../xrUI/Widgets/UI3tButton.h"
#include "../xrEngine/string_table.h"

CUIPdaSpot::CUIPdaSpot()
{
	m_background = nullptr;
	m_editBox = nullptr;
	m_btn_ok = nullptr;
	m_btn_cancel = nullptr;
    m_btn_gps = nullptr;
    m_placeholder = nullptr;
    m_title = nullptr;

	for (u32 i = 0; i < marker_button_count; ++i)
		m_markerButtons[i] = nullptr;

	m_mainWnd = false;
	m_levelName = nullptr;
	m_position = Fvector();

	m_spotID = u16(-1);

	m_defaultSpotType = READ_IF_EXISTS(pSettings, r_string, "user_spots", "spot_type", "treasure");
	// FFx0001 override spot section from global config
	if (pGameGlobals->section_exist("pda_map") && pGameGlobals->line_exist("pda_map", "hand_spot_icon_xml_section"))
	{
		const shared_str override_spot_name = pGameGlobals->r_string("pda_map", "hand_spot_icon_xml_section");

		if (override_spot_name.c_str() != nullptr) {
			m_defaultSpotType = override_spot_name;
		}
	}
	m_spotType = m_defaultSpotType;

	InitControls();
}

CUIPdaSpot::~CUIPdaSpot()
{
}

// COP_ENGINE_PATCH: COP-ENG-108 - GetType returns LPCSTR; compare its contents, never pointer addresses.
void CUIPdaSpot::Init(u16 spot_id, LPCSTR level_name, Fvector pos, bool main_wnd)
{
	m_mainWnd = main_wnd;
	m_levelName = level_name;
	m_position = pos;

    // COP_ENGINE_PATCH: COP-ENG-106 - editing a GPS title cannot retype it as an ordinary marker.
    for (auto* button : m_markerButtons)
        button->Enable(true);
    if (m_btn_gps)
        m_btn_gps->Enable(m_mainWnd);
    if (m_title)
        m_title->TextItemControl()->SetTextST(m_mainWnd ? "pf_gps_window_title" : "pf_gps_edit_title");
    if (m_btn_gps)
        m_btn_ok->TextItemControl()->SetTextST(m_mainWnd ? "pf_gps_add_marker" : "pf_gps_save_marker");
	m_spotID = !m_mainWnd ? spot_id : u16(-1);
	m_originalSpotType = nullptr;

	if (m_mainWnd)
	{
		SetMarkerType(m_defaultSpotType);
	}
	else
	{
		CMapLocation* ml = FindUserLocation();
		if (!ml) return;

        if (0 == xr_strcmp(ml->GetType(), g_user_map_task_location))
            for (auto* button : m_markerButtons)
                button->Enable(false);
		m_originalSpotType = ml->GetType();
		SetMarkerType(m_originalSpotType);
		m_editBox->SetText(ml->GetHint());
		ml->HighlightSpot(true, Fcolor().set(255.f, 36.f, 0.f, 255.f));
	}
}

void CUIPdaSpot::InitControls()
{
	this->SetWndRect(Frect().set(0.0f, 0.0f, 1024.f, 768.f));

	CUIXml uiXml;
	uiXml.Load(CONFIG_PATH, UI_PATH, "pda_spot.xml");

	m_background = UIHelper::CreateStatic(uiXml, "background", this);
    // COP_ENGINE_PATCH: COP-ENG-106 - optional controls retain compatibility with old XML.
    for (LPCSTR node : { "name_background", "name_label", "gps_hint" })
        if (uiXml.NavigateToNode(node))
            UIHelper::CreateStatic(uiXml, node, this)->Enable(false);
    if (uiXml.NavigateToNode("title_label"))
        m_title = UIHelper::CreateStatic(uiXml, "title_label", this);
	m_editBox = UIHelper::CreateEditBox(uiXml, "spot_name_edit", this);
    if (uiXml.NavigateToNode("spot_placeholder"))
    {
        m_placeholder = UIHelper::CreateStatic(uiXml, "spot_placeholder", this);
        m_placeholder->Enable(false);
    }
	UIHelper::CreateStatic(uiXml, "marker_label", this);

	// COP_ENGINE_PATCH: COP-ENG-089 - data-driven PDA user marker icon selector.
	R_ASSERT2(uiXml.GetNodesNum("", 0, "marker_button") == marker_button_count,
		"pda_spot.xml must define exactly eight marker_button nodes");
	for (u32 i = 0; i < marker_button_count; ++i)
	{
		m_markerSpotTypes[i] = uiXml.ReadAttrib("marker_button", i, "spot_type", "");
		const shared_str icon = uiXml.ReadAttrib("marker_button", i, "icon", "");
		R_ASSERT2(m_markerSpotTypes[i].size(), "marker_button requires a non-empty spot_type attribute");
		R_ASSERT2(icon.size(), "marker_button requires a non-empty icon attribute");

		for (u32 previous = 0; previous < i; ++previous)
			R_ASSERT2(m_markerSpotTypes[previous] != m_markerSpotTypes[i], "marker_button spot_type values must be unique");

		CUI3tButton* button = new CUI3tButton();
		button->SetAutoDelete(true);
		AttachChild(button);
		CUIXmlInit::Init3tButton(uiXml, "marker_button", i, button);
		button->SetButtonAsSwitch(true);

		button->AddStatic();
		CUIStatic* icon_static = button->GetBtnStatic();
		icon_static->SetWndPos(Fvector2().set(3.f, 3.f));
		icon_static->SetWndSize(Fvector2().set(button->GetWidth() - 6.f, button->GetHeight() - 6.f));
		icon_static->InitTexture(icon.c_str());
		icon_static->SetStretchTexture(true);
		icon_static->Enable(false);

		m_markerButtons[i] = button;
		Register(button);
		AddCallback(button, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIPdaSpot::OnMarkerType));
	}
	SetMarkerType(m_defaultSpotType);

	m_btn_ok = UIHelper::Create3tButton(uiXml, "btn_apply", this);
	m_btn_ok->Enable(CanApply());
	Register(m_btn_ok);
	AddCallback(m_btn_ok, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIPdaSpot::OnApply));

    if (uiXml.NavigateToNode("btn_gps"))
    {
        m_btn_gps = UIHelper::Create3tButton(uiXml, "btn_gps", this);
        m_btn_gps->AddStatic();
        CUIStatic* gps_icon = m_btn_gps->GetBtnStatic();
        // COP_ENGINE_PATCH: COP-ENG-109 - share the button's vertical center with its caption.
        gps_icon->SetWndPos(Fvector2().set(14.f, (m_btn_gps->GetHeight() - 18.f) * 0.5f));
        gps_icon->SetWndSize(Fvector2().set(18.f, 18.f));
        // COP_ENGINE_PATCH: COP-ENG-108 - the dialog can keep its copied GPS glyph in its own atlas.
        gps_icon->InitTexture(uiXml.ReadAttrib("btn_gps", 0, "icon", "ui_inGame2_PDA_icon_Primary_mission"));
        gps_icon->SetStretchTexture(true);
        gps_icon->Enable(false);
        Register(m_btn_gps);
        AddCallback(m_btn_gps, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIPdaSpot::OnAddGPS));
    }
	m_btn_cancel = UIHelper::Create3tButton(uiXml, "btn_cancel", this);
	Register(m_btn_cancel);
	AddCallback(m_btn_cancel, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIPdaSpot::OnExit));

    // COP_ENGINE_PATCH: COP-ENG-107 - opt-in layout scales every background, hitbox and icon once.
    if (uiXml.ReadAttribInt("background", 0, "aspect_correct", 0))
        ScalePdaGpsDialog(this, m_background,
            uiXml.ReadAttribFlt("background", 0, "layout_scale", 1.f), UI().get_current_kx());
}

CMapLocation* CUIPdaSpot::FindUserLocation() const
{
	Locations& locations = Level().MapManager().Locations();
	for (Locations_it it = locations.begin(); it != locations.end(); ++it)
	{
		if (it->actual && it->object_id == m_spotID && it->location && it->location->IsUserDefined())
			return it->location;
	}

	return nullptr;
}

bool CUIPdaSpot::SetMarkerType(const shared_str& spot_type)
{
	m_spotType = spot_type;
	bool matched = false;
	for (u32 i = 0; i < marker_button_count; ++i)
	{
		const bool selected = m_markerSpotTypes[i] == spot_type;
		m_markerButtons[i]->SetButtonState(selected ? CUIButton::BUTTON_PUSHED : CUIButton::BUTTON_NORMAL);
		matched = matched || selected;
	}

	if (m_btn_ok)
		m_btn_ok->Enable(CanApply());
	return matched;
}

// COP_ENGINE_PATCH: COP-ENG-160 - reject absent, ambiguous or stale visual selection.
bool CUIPdaSpot::HasSelectedMarker() const
{
	if (!m_spotType.size())
		return false;

	u32 selected_count = 0;
	bool matched = false;
	for (u32 i = 0; i < marker_button_count; ++i)
	{
		if (m_markerButtons[i] && m_markerButtons[i]->GetButtonState() == CUIButton::BUTTON_PUSHED)
		{
			++selected_count;
			matched = matched || m_markerSpotTypes[i] == m_spotType;
		}
	}
	return selected_count == 1 && matched;
}

bool CUIPdaSpot::CanApply() const
{
	// Existing GPS tasks have their own fixed icon and only allow renaming here.
	if (!m_mainWnd && m_originalSpotType.size() &&
		0 == xr_strcmp(m_originalSpotType.c_str(), g_user_map_task_location))
		return true;
	return HasSelectedMarker();
}

void CUIPdaSpot::OnMarkerType(CUIWindow* w, void* d)
{
	for (u32 i = 0; i < marker_button_count; ++i)
	{
		if (m_markerButtons[i] == w)
		{
			SetMarkerType(m_markerSpotTypes[i]);
			return;
		}
	}
}

// COP_ENGINE_PATCH: COP-ENG-106 - empty or whitespace-only names receive a localized default.
shared_str CUIPdaSpot::MarkerName() const
{
    const LPCSTR raw = m_editBox->GetText();
    xr_string name(raw ? raw : "");
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == xr_string::npos)
        return CStringTable().translate("pf_gps_default_name");
    const auto last = name.find_last_not_of(" \t\r\n");
    return shared_str(name.substr(first, last - first + 1).c_str());
}

void CUIPdaSpot::Update()
{
    base_class::Update();
	// COP_ENGINE_PATCH: COP-ENG-160 - also catch a selection reset after Init.
	m_btn_ok->Enable(CanApply());
    if (m_placeholder)
        m_placeholder->Show(!m_editBox->GetText() || !m_editBox->GetText()[0]);
}

void CUIPdaSpot::OnAddGPS(CUIWindow* ui, void* d)
{
    if (!m_mainWnd)
        return;
    Level().GameTaskManager()->AddUserMapTask(MarkerName(), m_levelName, m_position);
    OnExit(ui, d);
}

void CUIPdaSpot::OnAdd(CUIWindow* ui, void* d)
{
	// COP_ENGINE_PATCH: COP-ENG-160 - direct callbacks cannot bypass the UI gate.
	if (!HasSelectedMarker())
		return;
	CMapLocation* ml = Level().MapManager().AddUserLocation(m_spotType, m_levelName, m_position);
	ml->SetHint(MarkerName());
	ml->SetSerializable(true);

	OnExit(ui, d);
}

void CUIPdaSpot::OnApply(CUIWindow* ui, void* d)
{
	if (!CanApply())
		return;
	if (m_mainWnd)
	{
		OnAdd(ui, d);
		return;
	}

	CMapLocation* ml = FindUserLocation();
	if (!ml)
		return;

    if (0 == xr_strcmp(ml->GetType(), g_user_map_task_location))
    {
        Level().GameTaskManager()->RenameUserMapTask(ml, MarkerName());
        OnExit(ui, d);
        return;
    }
	if (m_originalSpotType != m_spotType)
	{
		CMapLocation* replacement = Level().MapManager().AddUserLocation(m_spotType, m_levelName, m_position);
		replacement->SetHint(MarkerName());
		replacement->SetSerializable(true);

		ml->HighlightSpot(false, Fcolor().set(0.f, 0.f, 0.f, 0.f));
		Level().MapManager().RemoveMapLocation(ml);
	}
	else if (MarkerName() != ml->GetHint())
		ml->SetHint(MarkerName());

	OnExit(ui, d);
}

void CUIPdaSpot::OnExit(CUIWindow* w, void* d)
{
	Exit();
}

void CUIPdaSpot::Exit()
{
	if (!m_mainWnd)
	{
		CMapLocation* ml = FindUserLocation();
		if (ml)
			ml->HighlightSpot(false, Fcolor().set(0.f, 0.f, 0.f, 0.f));
	}

	m_mainWnd = false;
	m_levelName = nullptr;
	m_position = Fvector();
	m_spotID = u16(-1);
	m_originalSpotType = nullptr;
	SetMarkerType(m_defaultSpotType);

	m_editBox->ClearText();

	HideDialog();
}

bool CUIPdaSpot::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	switch (dik)
	{
	case DIK_ESCAPE:
	{
		if (IsShown())
		{
			Exit();
			return true;
		}
	}break;
	}

	return base_class::OnKeyboardAction(dik, keyboard_action);
}

void CUIPdaSpot::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}
