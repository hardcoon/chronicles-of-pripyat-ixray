#include "stdafx.h"
#include "UIPdaWnd.h"
#include "../PDA.h"

#include "../../xrUI/xrUIXmlParser.h"
#include "../../xrUI/UIXmlInit.h"
#include "UIInventoryUtilities.h"

#include "../Level.h"
#include "../map_manager.h"
#include "../map_location.h"
#include "UIGameCustom.h"

#include "../../xrUI/Widgets/UIStatic.h"
#include "../../xrUI/Widgets/UIFrameWindow.h"
#include "../../xrUI/Widgets/UITabControl.h"
#include "UIMapWnd.h"
#include "../../xrUI/Widgets/UIFrameLineWnd.h"
#include "object_broker.h"
#include "UIMessagesWindow.h"
#include "UIMainIngameWnd.h"
#include "../../xrUI/Widgets/UITabButton.h"
#include "../../xrUI/Widgets/UIAnimatedStatic.h"

#include "../../xrUI/UIHelper.h"
#include "../../xrUI/Widgets/UIHint.h"
#include "../../xrUI/Widgets/UIBtnHint.h"
#include "UITaskWnd.h"
#include "UIRankingWnd.h"
#include "UILogsWnd.h"
#include "UIFactionWarWnd.h"
#include "UIScriptWnd.h"
#include "UIPdaContactsWnd.h"
#include "../../xrEngine/xr_ioconsole.h"

#define PDA_XML		"pda.xml"

// COP_ENGINE_PATCH: COP-ENG-128 - native 2D PDA overlay; no Anomaly PDA scripts or class ABI changes.
static xr_map<const CUIPdaWnd*,CUIStatic*> pf_sss_pda_overlays;
static void DrawSSSPdaDrops(const CUIPdaWnd* owner,CUIStatic* screen) {
    if(!Console || !Console->GetBool("ssfx_enabled") || !screen)return;
    float low=0,high=0;
    if(Console->GetFloat("ssfx_hud_rain_enabled",low,high)<.5f)return;
    string_path path;
    if(!FS.exist(path,"$game_shaders$","d3d11\\pf_sss_pda_drops.ps.hlsl"))return;
    CUIStatic*& overlay=pf_sss_pda_overlays[owner];
    if(!overlay){
        overlay=new CUIStatic();
        overlay->InitTextureEx("pf_sss\\fx\\hud_rain","hud\\pf_sss_pda");
        overlay->SetStretchTexture(true);
    }
    overlay->SetWndPos(screen->GetWndPos());overlay->SetWndSize(screen->GetWndSize());
    overlay->Draw();
}

u32 g_pda_info_state = 0;

void RearrangeTabButtons(CUITabControl* pTab);

CUIPdaWnd::CUIPdaWnd()
{
	LoadCallbackGlobals(m_isSetActiveSubdialog, m_onSetActiveSubdialog, "OnSetActiveSubdialog");

	pUITaskWnd       = nullptr;
	pUIFactionWarWnd = nullptr;
	pUIRankingWnd    = nullptr;
	pUILogsWnd       = nullptr;
	UIPdaContactsWnd = nullptr;
	m_hint_wnd       = nullptr;

	LoadCallbackGlobals(m_isSetActiveSubdialog, m_onSetActiveSubdialog, "OnSetActiveSubdialog");
	Init();
}

CUIPdaWnd::~CUIPdaWnd()
{
    auto overlay=pf_sss_pda_overlays.find(this);
    if(overlay!=pf_sss_pda_overlays.end()){xr_delete(overlay->second);pf_sss_pda_overlays.erase(overlay);}
	delete_data( pUITaskWnd );
	delete_data( pUIFactionWarWnd );
	delete_data( UIPdaContactsWnd );
	delete_data( pUIRankingWnd );
	delete_data( pUILogsWnd );
	delete_data( m_hint_wnd );
	delete_data( UINoice );
}

void CUIPdaWnd::Init()
{
	CUIXml					uiXml;
	uiXml.Load				(CONFIG_PATH, UI_PATH, PDA_XML);

	m_pActiveDialog			= nullptr;
	m_sActiveSection		= "";

	CUIXmlInit::InitWindow	(uiXml, "main", 0, this);

	UIMainPdaFrame			= UIHelper::CreateStatic	( uiXml, "background_static", this );
	m_caption				= UIHelper::CreateStatic	( uiXml, "caption_static", this );
	m_caption_const			= ( m_caption->TextItemControl()->GetText() );
	if (uiXml.NavigateToNode("clock_wnd"))
	m_clock					= UIHelper::CreateTextWnd	( uiXml, "clock_wnd", this );

	if (uiXml.NavigateToNode("anim_static"))
	{
		m_anim_static = new CUIAnimatedStatic();
		AttachChild(m_anim_static);
		m_anim_static->SetAutoDelete(true);
		CUIXmlInit::InitAnimatedStatic(uiXml, "anim_static", 0, m_anim_static);
	}
	m_btn_close				= UIHelper::Create3tButton( uiXml, "close_button", this );
	m_hint_wnd				= UIHelper::CreateHint( uiXml, "hint_wnd" );

	UITabControl					= new CUITabControl();
	UITabControl->SetAutoDelete		(true);
	AttachChild						(UITabControl);
	CUIXmlInit::InitTabControl		(uiXml, "tab", 0, UITabControl);
	UITabControl->SetMessageTarget	(this);

	pUITaskWnd					= new CUITaskWnd();
	pUITaskWnd->hint_wnd		= m_hint_wnd;
	pUITaskWnd->Init			();

	if (UITabControl->GetButtonById("eptFractionWar"))
	{
		pUIFactionWarWnd = new CUIFactionWarWnd();
		pUIFactionWarWnd->hint_wnd = m_hint_wnd;
		pUIFactionWarWnd->Init();
	}

	if (UITabControl->GetButtonById("eptContacts"))
	{
		UIPdaContactsWnd = new CUIPdaContactsWnd();
		UIPdaContactsWnd->Init();
	}
	pUIRankingWnd					= new CUIRankingWnd();
	pUIRankingWnd->Init				();

	pUILogsWnd						= new CUILogsWnd();
	pUILogsWnd->Init				();


	UINoice					= new CUIStatic();
	UINoice->SetAutoDelete	( true );
	CUIXmlInit::InitStatic	( uiXml, "noice_static", 0, UINoice );

	const static bool rearrangeButtons = EngineExternal()[EEngineExternalUI::PdaRearrangeTabButtons];
	if (rearrangeButtons)
		RearrangeTabButtons		(UITabControl);
}

void CUIPdaWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	switch ( msg )
	{
	case TAB_CHANGED:
		{
			if ( pWnd == UITabControl )
			{
				SetActiveSubdialog			(UITabControl->GetActiveId());
			}
			break;
		}
	case BUTTON_CLICKED:
		{
			if ( pWnd == m_btn_close )
			{
				HideDialog();
			}
			break;
		}
	default:
		{
		if (CUIWindow* active = GetActiveDialog())
			active->SendMessage(pWnd, msg, pData);
		}
	};
}

void CUIPdaWnd::Show(bool status)
{
	inherited::Show						(status);
	if(status)
	{
		InventoryUtilities::SendInfoToActor	("ui_pda");
		
		if (m_sActiveSection == nullptr || strcmp(m_sActiveSection.c_str(), "") == 0)
		{
			SetActiveSubdialog				("eptTasks");
			UITabControl->SetActiveTab		("eptTasks");
		}
		else
			SetActiveSubdialog(m_sActiveSection);
	}else
	{
		InventoryUtilities::SendInfoToActor	("ui_pda_hide");
		CurrentGameUI()->UIMainIngameWnd->SetFlashIconState_(CUIMainIngameWnd::efiPdaTask, false);
		if (CUIWindow* active = GetActiveDialog())
			active->Show(false);
		m_pActiveDialog = pUITaskWnd; // native fallback, including a released Lua page
		g_btnHint->Discard					();
		g_statHint->Discard					();
	}
}

void CUIPdaWnd::Update()
{
	inherited::Update();
	if (CUIWindow* active = GetActiveDialog())
		active->Update();
	if (m_clock)
		m_clock->TextItemControl().SetText(InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());

	Device.seqParallel.push_back(xr_make_delegate(pUILogsWnd, &CUILogsWnd::PerformWork));
}

// COP_ENGINE_PATCH: COP-ENG-191 - a Lua page is borrowed only while attached.
// CUIWindow's destructor removes it from the frame; compare addresses before
// any virtual call, without dereferencing a potentially freed script window.
CUIWindow* CUIPdaWnd::GetActiveDialog()
{
    if (!m_pActiveDialog)
        return nullptr;
    if (m_pActiveDialog == pUITaskWnd || m_pActiveDialog == pUIFactionWarWnd ||
        m_pActiveDialog == UIPdaContactsWnd || m_pActiveDialog == pUIRankingWnd ||
        m_pActiveDialog == pUILogsWnd || UIMainPdaFrame->IsChild(m_pActiveDialog))
        return m_pActiveDialog;
    return nullptr;
}

void CUIPdaWnd::SetActiveSubdialog(const shared_str& section)
{
    CUIWindow* previous = GetActiveDialog();
    m_pActiveDialog = nullptr;
    if (previous)
    {
        // DetachChild can delete an auto-owned page: hide it before detaching.
        previous->Show(false);
        if (UIMainPdaFrame->IsChild(previous))
            UIMainPdaFrame->DetachChild(previous);
    }

	if ( section == "eptTasks" )
	{
		m_pActiveDialog = pUITaskWnd;
	}
	else if ( section == "eptFractionWar" )
	{
		m_pActiveDialog = pUIFactionWarWnd;
	}
	else if (section == "eptContacts")
	{
		m_pActiveDialog = UIPdaContactsWnd;
	}
	else if (section == "eptRanking")
	{
		if (IsGameTypeSingle()) {
			m_pActiveDialog = pUIRankingWnd;
		}
	}
	else if ( section == "eptLogs" )
	{
		m_pActiveDialog = pUILogsWnd;
	}
	if (m_isSetActiveSubdialog)
	{
		luabind::functor<CUIDialogWndEx*> funct;
		R_ASSERT2(ai().script_engine().functor(m_onSetActiveSubdialog, funct), "failed to get OnSetActiveSubdialog functor");

		CUIDialogWndEx* ret = funct((LPCSTR)section.c_str());
		CUIWindow* pScriptWnd = ret ? smart_cast<CUIWindow*>(ret) : (0);
		if (pScriptWnd)
			m_pActiveDialog = pScriptWnd;
		
			if (m_pActiveDialog)
			{
				if (!UIMainPdaFrame->IsChild(m_pActiveDialog))
					UIMainPdaFrame->AttachChild(m_pActiveDialog);
				m_pActiveDialog->Show(true);
				m_sActiveSection = section;
				SetActiveCaption();
			}
			else {
				m_sActiveSection = "";
			}
	}
	else
	{
		if (!UIMainPdaFrame->IsChild(m_pActiveDialog))
			UIMainPdaFrame->AttachChild(m_pActiveDialog);
		m_pActiveDialog->Show(true);

		if (UITabControl->GetActiveId() != section)
		{
			UITabControl->SetActiveTab(section);
		}
		m_sActiveSection = section;
		SetActiveCaption();
	}
}

void CUIPdaWnd::SetActiveCaption()
{
	TABS_VECTOR*	btn_vec		= UITabControl->GetButtonsVector();
	TABS_VECTOR::iterator it_b	= btn_vec->begin();
	TABS_VECTOR::iterator it_e	= btn_vec->end();
	for ( ; it_b != it_e; ++it_b )
	{
		if ( (*it_b)->m_btn_id == m_sActiveSection )
		{
			LPCSTR cur = (*it_b)->TextItemControl()->GetText();
			string256 buf;
			xr_strconcat(buf, m_caption_const.c_str(), cur );
			SetCaption( buf );
			return;
		}
	}
}

void CUIPdaWnd::Show_SecondTaskWnd( bool status )
{
	if ( status )
	{
		SetActiveSubdialog( "eptTasks" );
	}
	pUITaskWnd->Show_TaskListWnd( status );
}

void CUIPdaWnd::Show_MapLegendWnd( bool status )
{
	if ( status )
	{
		SetActiveSubdialog( "eptTasks" );
	}
	pUITaskWnd->ShowMapLegend( status );
}

void CUIPdaWnd::Draw()
{
	inherited::Draw();
//.	DrawUpdatedSections();
	DrawHint();
	UINoice->Draw(); // over all
    DrawSSSPdaDrops(this,UINoice);
}

void CUIPdaWnd::DrawHint()
{
	if (m_sActiveSection == "eptTasks")
	{
		pUITaskWnd->DrawHint();
	}
	else if (m_sActiveSection == "eptFractionWar")
	{
		//m_hint_wnd->Draw();
	}
	else if (m_sActiveSection == "eptRanking")
	{
		pUIRankingWnd->DrawHint();
	}
	else if (m_sActiveSection == "eptLogs")
	{

	}
	else if (m_sActiveSection == "eptContacts")
	{

	}
	m_hint_wnd->Draw();
}

// COP_ENGINE_PATCH: COP-ENG-153 - native PDA centering on an existing marker.
// Copy the target before activating the map; Show/Activated may recenter on the actor.
bool CUIPdaWnd::FocusMapSpot(u16 object_id, LPCSTR spot_type)
{
    if (!g_pGameLevel || !pUITaskWnd || !spot_type || !*spot_type)
        return false;

    CUIMapWnd* map = pUITaskWnd->GetMapWnd();
    CMapLocation* location = Level().MapManager().GetMapLocation(spot_type, object_id);
    if (!map || !location || !location->Update())
        return false;

    const shared_str map_name = location->GetLevelName();
    const Fvector2 position = location->CalcPosition();
    if (!map_name.size() || map->GetIdxByName(map_name) == u16(-1) ||
        !_valid(position.x) || !_valid(position.y))
        return false;

    const float zoom = map->GetZoom();
    SetActiveSubdialog("eptTasks");
    if (UITabControl)
        UITabControl->SetActiveTab("eptTasks");

    // Keep the player's current zoom (COP-ENG-105), including the first map activation.
    map->SetZoom(zoom);
    map->SetTargetMap(map_name, position, false);
    return true;
}

void CUIPdaWnd::UpdatePda()
{
	pUILogsWnd->UpdateNews();

	if (m_sActiveSection == "eptTasks")
	{
		pUITaskWnd->ReloadTaskInfo();
	}
}

void CUIPdaWnd::UpdateRankingWnd()
{
	pUIRankingWnd->Update();
}

void CUIPdaWnd::Reset()
{
    // COP_ENGINE_PATCH: COP-ENG-191 - detach borrowed pages while Lua is alive.
    // Preserve the selected section/button, so opening the PDA creates its
    // fresh page through the usual callback after reload, without a map flash.
    m_pActiveDialog = nullptr;
    UIMainPdaFrame->DetachAll();

	inherited::ResetAll		();

	if ( pUITaskWnd )		pUITaskWnd->ResetAll();
	if ( pUIFactionWarWnd )	pUIFactionWarWnd->ResetAll();
	if ( UIPdaContactsWnd )	UIPdaContactsWnd->ResetAll();
	if ( pUIRankingWnd )	pUIRankingWnd->ResetAll();
	if ( pUILogsWnd )		pUILogsWnd->ResetAll();
}

void CUIPdaWnd::SetCaption( LPCSTR text )
{
	m_caption->TextItemControl()->SetText( text );
}

void RearrangeTabButtons(CUITabControl* pTab)
{
	TABS_VECTOR *	btn_vec		= pTab->GetButtonsVector();
	TABS_VECTOR::iterator it	= btn_vec->begin();
	TABS_VECTOR::iterator it_e	= btn_vec->end();

	Fvector2					pos;
	pos.set						((*it)->GetWndPos());
	float						size_x;

	for ( ; it != it_e; ++it )
	{
		(*it)->SetWndPos		(pos);
		(*it)->AdjustWidthToText();
		size_x					= (*it)->GetWndSize().x + 30.0f;
		(*it)->SetWidth			(size_x);
		pos.x					+= size_x - 6.0f;
	}
	
	pTab->SetWidth( pos.x + 5.0f );
	pos.x = pTab->GetWndPos().x - pos.x;
	pos.y = pTab->GetWndPos().y;
	pTab->SetWndPos( pos );
}

bool CUIPdaWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	if ( is_binded(kACTIVE_JOBS, dik) )
	{
		if ( WINDOW_KEY_PRESSED == keyboard_action )
			HideDialog();

		return true;
	}	

	return inherited::OnKeyboardAction(dik,keyboard_action);
}
