#include "stdafx.h"
#include "UITaskWnd.h"
#include "PdaGpsLayout.h"
#include "PdaMapFilterButton.h"
#include "UIMapWnd.h"
#include "object_broker.h"
#include "../../xrEngine/FontManager.h"
#include "../../xrUI/UIXmlInit.h"
#include "../../xrUI/Widgets/UIStatic.h"
#include "../../xrUI/Widgets/UI3tButton.h"
#include "../../xrUI/Widgets/UIFrameLineWnd.h"
#include "UISecondTaskWnd.h"
#include "UIMapLegend.h"
#include "../../xrUI/UIHelper.h"
#include "../../xrUI/Widgets/UIHint.h"

#include "../GameTask.h"
#include "../map_location.h"
#include "../map_location_defs.h"
#include "../map_manager.h"
#include "UIInventoryUtilities.h"
#include "../Level.h"
#include "../GametaskManager.h"
#include "../Actor.h"
#include "../../xrUI/Widgets/UICheckButton.h"

CUITaskWnd::CUITaskWnd()
	: m_background(nullptr), m_background2(nullptr),
	m_center_background(nullptr), m_right_bottom_background(nullptr),
	m_task_split(nullptr), m_pMapWnd(nullptr),
	m_pStoryLineTaskItem(nullptr), m_pSecondaryTaskItem(nullptr),
	m_BtnTaskListWnd(nullptr), m_second_task_index(nullptr),
	m_devider(nullptr), m_actual_frame(0),
	m_btn_focus(nullptr), m_btn_focus2(nullptr),
	m_task_wnd(nullptr), m_task_wnd_show(false),
	m_map_legend_wnd(nullptr), hint_wnd(nullptr)
{
}
CUITaskWnd::~CUITaskWnd()
{
	delete_data						(m_pMapWnd);
}

void CUITaskWnd::Init()
{
	CUIXml							xml;
	xml.Load						(CONFIG_PATH, UI_PATH, PDA_TASK_XML);
	VERIFY							(hint_wnd);

	CUIXmlInit::InitWindow			(xml, "main_wnd", 0, this);

	m_background					= UIHelper::CreateFrameWindow( xml, "background", this, false);

	m_background2					= UIHelper::CreateFrameLine(xml, "background", this, false);

	if (xml.NavigateToNode("task_split"))
		m_task_split = UIHelper::CreateFrameLine(xml, "task_split", this);

    InitMapFilters(xml);

	m_pMapWnd						= new CUIMapWnd(); 
	m_pMapWnd->SetAutoDelete		(false);
	m_pMapWnd->hint_wnd				= hint_wnd;
	m_pMapWnd->Init					(PDA_TASK_XML,"map_wnd");
	AttachChild						(m_pMapWnd);

	m_center_background				= UIHelper::CreateStatic( xml, "center_background", this );
	if (xml.NavigateToNode("line_devider"))
	{
		m_devider = UIHelper::CreateStatic(xml, "line_devider", this);
	}

	m_pStoryLineTaskItem			= new CUITaskItem();
	m_pStoryLineTaskItem->Init		(xml,"storyline_task_item");
	AttachChild						(m_pStoryLineTaskItem);
	m_pStoryLineTaskItem->SetAutoDelete(true);
	AddCallback						(m_pStoryLineTaskItem, WINDOW_LBUTTON_DB_CLICK,   CUIWndCallback::void_function(this,&CUITaskWnd::OnTask1DbClicked));

	if (xml.NavigateToNode("secondary_task_item"))
    {
        Level().GameTaskManager()->AllowMultipleTask(true);
        m_pSecondaryTaskItem = new CUITaskItem();
        m_pSecondaryTaskItem->Init(xml, "secondary_task_item");
        AttachChild(m_pSecondaryTaskItem);
        m_pSecondaryTaskItem->SetAutoDelete(true);
        AddCallback(m_pSecondaryTaskItem, WINDOW_LBUTTON_DB_CLICK, CUIWndCallback::void_function(this, &CUITaskWnd::OnTask2DbClicked));
    }

	m_btn_focus		= UIHelper::Create3tButton( xml, "btn_task_focus", this );
	Register		(m_btn_focus);
	AddCallback		(m_btn_focus,  BUTTON_DOWN, CUIWndCallback::void_function(this,&CUITaskWnd::OnTask1DbClicked));

    if (xml.NavigateToNode("btn_task_focus2"))
    {
		m_btn_focus2 = UIHelper::Create3tButton(xml, "btn_task_focus2", this);
        Register(m_btn_focus2);
        AddCallback(m_btn_focus2, BUTTON_DOWN, CUIWndCallback::void_function(this, &CUITaskWnd::OnTask2DbClicked));
    }
	m_BtnTaskListWnd		= UIHelper::Create3tButton( xml, "btn_second_task", this );
    m_mapFilterCaptionFont = m_BtnTaskListWnd->TextItemControl()->GetFont();
	AddCallback				(m_BtnTaskListWnd, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITaskWnd::OnShowTaskListWnd));

	if (xml.NavigateToNode("second_task_index"))
		m_second_task_index = UIHelper::CreateStatic(xml, "second_task_index", this);

	m_task_wnd					= new UITaskListWnd(); 
	m_task_wnd->SetAutoDelete	(true);
	m_task_wnd->hint_wnd		= hint_wnd;
	m_task_wnd->init_from_xml	(xml, "second_task_wnd");
	m_task_wnd->ShowOnlySecondaryTasks(m_pSecondaryTaskItem != nullptr);

	m_pMapWnd->AttachChild		(m_task_wnd);
	m_task_wnd->SetMessageTarget(this);
	m_task_wnd->Show			(false);
	m_task_wnd_show				= false;

	m_map_legend_wnd					= new UIMapLegend(); 
	m_map_legend_wnd->SetAutoDelete		(true);
	m_map_legend_wnd->init_from_xml		(xml, "map_legend_wnd");
	m_pMapWnd->AttachChild				(m_map_legend_wnd);
	m_map_legend_wnd->SetMessageTarget	(this);
	m_map_legend_wnd->Show				(false);
    FitMapFilterCaptions();
}

void CUITaskWnd::Update()
{
	if(Level().GameTaskManager()->ActualFrame() != m_actual_frame)
	{
		ReloadTaskInfo();
	}

	if ( m_pStoryLineTaskItem->show_hint && m_pStoryLineTaskItem->OwnerTask() )
	{
		m_pMapWnd->ShowHintTask( m_pStoryLineTaskItem->OwnerTask(), m_pStoryLineTaskItem );
	}
	else if (m_pSecondaryTaskItem && m_pSecondaryTaskItem->show_hint && m_pSecondaryTaskItem->OwnerTask())
	{
		m_pStoryLineTaskItem->show_hint = false;
		m_pMapWnd->ShowHintTask(m_pSecondaryTaskItem->OwnerTask(), m_pSecondaryTaskItem);
	}
	else
	{
		m_pMapWnd->HideCurHint();
	}
	inherited::Update				();
}

void CUITaskWnd::Draw()
{
	inherited::Draw					();
}

void CUITaskWnd::DrawHint()
{
	m_pMapWnd->DrawHint();
}


void CUITaskWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	if ( msg == PDA_TASK_SET_TARGET_MAP && pData )
	{
		CGameTask* task = static_cast<CGameTask*>( pData );
		TaskSetTargetMap( task );
		return;
	}
	if ( msg == PDA_TASK_SHOW_MAP_SPOT && pData)
	{
		CGameTask* task = static_cast<CGameTask*>( pData );
		TaskShowMapSpot( task, true );
		return;
	}
	if ( msg == PDA_TASK_HIDE_MAP_SPOT && pData )
	{
		CGameTask* task = static_cast<CGameTask*>( pData );
		TaskShowMapSpot( task, false );
		return;
	}
	
	if ( msg == PDA_TASK_SHOW_HINT && pData )
	{
		CGameTask* task = static_cast<CGameTask*>( pData );
		m_pMapWnd->ShowHintTask( task, pWnd );
		return;
	}
	if ( msg == PDA_TASK_HIDE_HINT )
	{
		m_pMapWnd->HideCurHint();
		return;
	}

	inherited::SendMessage(  pWnd, msg, pData );
	CUIWndCallback::OnEvent( pWnd, msg, pData );
}

// COP_ENGINE_PATCH: COP-ENG-106 - refresh visible and hidden task controls after a GPS task change.
void CUITaskWnd::UserMapTaskRemoved()
{
    m_pMapWnd->HideCurHint();
    ReloadTaskInfo();
    m_task_wnd->UpdateList();
}

void CUITaskWnd::ReloadTaskInfo()
{
    CGameTask* storyTask = Level().GameTaskManager()->ActiveTask(eTaskTypeStoryline);
    m_pStoryLineTaskItem->InitTask(storyTask);

    CGameTask* additionalTask = nullptr;
    if (m_pSecondaryTaskItem)
    {
        additionalTask = Level().GameTaskManager()->ActiveTask(eTaskTypeAdditional);
        m_pSecondaryTaskItem->InitTask(additionalTask);
    }

    if (!storyTask || (storyTask->m_map_object_id == u16(-1) || storyTask->m_map_location.size() == 0))
		m_btn_focus->Show(false);
	else
		m_btn_focus->Show(true);

	if (m_btn_focus2)
	{
		if (!additionalTask || (additionalTask->m_map_object_id == u16(-1) || additionalTask->m_map_location.size() == 0))
			m_btn_focus2->Show(false);
		else
			m_btn_focus2->Show(true);
	}

    FitMapFilterCaptions();
    SyncMapFilters();

	if (storyTask || additionalTask)
	{
		m_actual_frame = Level().GameTaskManager()->ActualFrame();
		if (m_task_wnd->IsShown())
			m_task_wnd->UpdateList();
	}

	if (!m_second_task_index)
		return;

	if (storyTask && !additionalTask)
	{
		const auto task_count = Level().GameTaskManager()->GetTaskCount(eTaskStateInProgress, eTaskTypeStoryline);
		if (task_count)
		{
			const auto task_index = Level().GameTaskManager()->GetTaskIndex(storyTask, eTaskStateInProgress, eTaskTypeStoryline);
			string32 buf;
			xr_sprintf(buf, sizeof(buf), "%d / %d", task_index, task_count);

			m_second_task_index->SetVisible(true);
			m_second_task_index->TextItemControl()->SetText(buf);
		}
		else
		{
			m_second_task_index->SetVisible(false);
			m_second_task_index->TextItemControl()->SetText("");
		}
	}

	if (additionalTask)
	{
		const auto task2_count = Level().GameTaskManager()->GetTaskCount(eTaskStateInProgress, eTaskTypeAdditional);

		if (task2_count)
		{
			const auto task2_index = Level().GameTaskManager()->GetTaskIndex(additionalTask, eTaskStateInProgress, eTaskTypeAdditional);
			string32 buf;
			xr_sprintf(buf, sizeof(buf), "%d / %d", task2_index, task2_count);

			m_second_task_index->SetVisible(true);
			m_second_task_index->TextItemControl()->SetText(buf);
		}
		else
		{
			m_second_task_index->SetVisible(false);
			m_second_task_index->TextItemControl()->SetText("");
		}
	}
}
void CUITaskWnd::Show(bool status)
{
	inherited::Show			(status);
	m_pMapWnd->Show			(status);
	m_pMapWnd->HideCurHint	();
	m_map_legend_wnd->Show	(false);
	if ( status )
	{
		ReloadTaskInfo();
		m_task_wnd->Show( m_task_wnd_show );
	}
	else
	{
//		m_task_wnd_show = false;
		m_task_wnd->Show(false);
	}
}

void CUITaskWnd::Reset()
{
	inherited::Reset	();
}

void CUITaskWnd::OnNextTaskClicked()
{
}

void CUITaskWnd::OnPrevTaskClicked()
{
}

void CUITaskWnd::OnShowTaskListWnd( CUIWindow* w, void* d )
{
	m_task_wnd_show = !m_task_wnd_show;
	m_task_wnd->Show( !m_task_wnd->IsShown() );
}

void CUITaskWnd::Show_TaskListWnd(bool status)
{
	m_task_wnd->Show( status );
	m_task_wnd_show = status;
}

void CUITaskWnd::TaskSetTargetMap( CGameTask* task )
{
	if (!task)
	{
		return;
	}
	
	TaskShowMapSpot( task, true );
	CMapLocation* ml = task->LinkedMapLocation();
	if ( ml && ml->SpotEnabled() )
	{
		ml->CalcPosition();
		m_pMapWnd->SetTargetMap( ml->GetLevelName(), ml->GetPosition(), true );
	}
}

void CUITaskWnd::TaskShowMapSpot( CGameTask* task, bool show )
{
	CMapLocation* ml = task ? task->LinkedMapLocation() : nullptr;
	if (ml && ml->PdaFilterEnabled())
	{
		if ( show )
		{
			ml->EnableSpot();
			ml->CalcPosition();
			m_pMapWnd->SetTargetMap( ml->GetLevelName(), ml->GetPosition(), true );
		}
		else
		{
			ml->DisableSpot();
		}
	}
}

void CUITaskWnd::OnTask1DbClicked(CUIWindow*, void*)
{
    CGameTask* task = Level().GameTaskManager()->ActiveTask(eTaskTypeStoryline);
    TaskSetTargetMap(task);
}

void CUITaskWnd::OnTask2DbClicked(CUIWindow*, void*)
{
    CGameTask* task = Level().GameTaskManager()->ActiveTask(eTaskTypeAdditional);
    TaskSetTargetMap(task);
}

void CUITaskWnd::ShowMapLegend( bool status )
{
	m_map_legend_wnd->Show( status );
}

void CUITaskWnd::Switch_ShowMapLegend()
{
	m_map_legend_wnd->Show( !m_map_legend_wnd->IsShown() );
}


// COP_ENGINE_PATCH: COP-ENG-263 - a layout lists its native controls and group IDs.
void CUITaskWnd::InitMapFilters(CUIXml& xml)
{
    using namespace PdaMapFilters;
    if (!g_pGameLevel) return;
    auto& manager = Level().MapManager();
    Mask controls = 0;
    if (auto* row = xml.NavigateToNode("map_filters"))
    {
        auto* savedRoot = xml.GetLocalRoot();
        xml.SetLocalRoot(row);
        const int count = xml.GetNodesNum(row, "filter");
        for (int i = 0; i < count; ++i)
        {
            const auto* name = xml.ReadAttrib("filter", i, "group", "");
            const auto category = manager.PdaFilterGroup(name);
            if (category == Category::None || Enabled(controls, category))
            {
                Msg("! Unknown or repeated PDA map filter button: %s", name);
                continue;
            }
            auto* button = new CPdaMapFilterButton();
            button->SetAutoDelete(true);
            AttachChild(button);
            CUIXmlInit::InitCheck(xml, "filter", i, button);
            const float captionLeft = std::max(0.f, xml.ReadAttribFlt("filter:text", i, "padding_left",
                PdaFilterLayout::LeftSkinCap + PdaFilterLayout::TextPadding));
            const float captionRight = std::max(0.f, xml.ReadAttribFlt("filter:text", i, "padding_right",
                PdaFilterLayout::RightSkinCap + PdaFilterLayout::TextPadding));
            m_mapFilterButtons.push_back({button, category, captionLeft, captionRight});
            controls = With(controls, category, true);
            AddCallback(button, BUTTON_CLICKED,
                CUIWndCallback::void_function(this, &CUITaskWnd::OnMapFilterClicked));
        }
        xml.SetLocalRoot(savedRoot);
    }
    manager.SetPdaFilterControls(controls);
    SyncMapFilters();
}

// COP_ENGINE_PATCH: COP-ENG-260 - native controls live and die with CUITaskWnd.
void CUITaskWnd::SyncMapFilters()
{
    for (const auto& filter : m_mapFilterButtons)
        filter.control->SetCheck(Level().MapManager().PdaFilterEnabled(filter.category));
}


// COP_ENGINE_PATCH: COP-ENG-263 - button bounds come from the PDA XML template.
// Fit one font size to the configured row and center captions clear of the indicators.
void CUITaskWnd::FitMapFilterCaptions()
{
    if (!m_mapFilterCaptionFont) return;
    const float kx = UI().get_current_kx();
    const auto captionFits = [](CGameFont* font, CUIStatic* control, float available)
    {
        float captionWidth = font->SizeOf_(control->TextItemControl()->GetText());
        UI().ClientToScreenScaledWidth(captionWidth);
        return captionWidth <= available;
    };
    const auto captionsFit = [&](CGameFont* font)
    {
        if (!captionFits(font, m_BtnTaskListWnd,
            m_BtnTaskListWnd->GetWidth() - 2.f * PdaFilterLayout::TextPadding * kx)) return false;
        for (const auto& filter : m_mapFilterButtons)
            if (!captionFits(font, filter.control,
                filter.control->PlaceCaption(filter.captionLeft, filter.captionRight, kx).width))
                return false;
        return true;
    };
    auto* font = m_mapFilterCaptionFont;
    if (!captionsFit(font))
    {
        font = UI().Font().GetFont("font_default");
        if (!captionsFit(font))
            font = UI().Font().pFontStat;
    }
    m_BtnTaskListWnd->TextItemControl()->SetFont(font);
    for (const auto& filter : m_mapFilterButtons)
    {
        auto* text = filter.control->TextItemControl();
        text->SetFont(font);
        text->m_TextOffset.x = filter.control->PlaceCaption(filter.captionLeft, filter.captionRight, kx).offset;
    }
}

void CUITaskWnd::OnMapFilterClicked(CUIWindow* control, void*)
{
    for (const auto& filter : m_mapFilterButtons)
        if (control == filter.control)
        {
            Level().MapManager().SetPdaFilter(filter.category, filter.control->GetCheck());
            m_pMapWnd->HideCurHint();
            ReloadTaskInfo();
            return;
        }
}
// --------------------------------------------------------------------------------------------------
CUITaskItem::CUITaskItem() : m_owner(nullptr), show_hint_can(false), show_hint(false), m_hint_wt(500) {}

CUIStatic* init_static_field(CUIXml& uiXml, LPCSTR path, LPCSTR path2);

void CUITaskItem::Init(CUIXml& uiXml, LPCSTR path)
{
	CUIXmlInit::InitWindow			(uiXml,path,0,this);
	m_hint_wt						= uiXml.ReadAttribInt(path, 0, "hint_wt", 500);

	string256		buff;
	CUIStatic* S					= nullptr;

	xr_strconcat(buff, path, ":t_icon" );
	if ( uiXml.NavigateToNode( buff ) )
	{
		S = init_static_field		(uiXml, path, "t_icon");
		AttachChild					(S);
	}
	m_info["t_icon"]				= S;
    if (S)
        m_taskIconRect = S->GetWndRect();
	
	xr_strconcat(buff, path, ":t_icon_over" );
	if ( uiXml.NavigateToNode( buff ) )
	{
		S = init_static_field		(uiXml, path, "t_icon_over");
		AttachChild					(S);
	}
	m_info["t_icon_over"]			= S;
	
	S = init_static_field			(uiXml, path, "t_caption");
	AttachChild						(S);
	m_info["t_caption"]				= S;

	show_hint_can = false;
	show_hint     = false;
}

void CUITaskItem::InitTask(CGameTask* task)
{
	m_owner							= task;
	CUIStatic* S					= m_info["t_icon"];
	if ( S )
	{
        S->SetWndRect(m_taskIconRect);
		if ( task )
		{
			S->InitTexture			(task->m_icon_texture_name.c_str());
            // COP_ENGINE_PATCH: COP-ENG-107 - GPS glyph fits the task's thumbnail without stretching.
            if (task->IsUserMapTask())
                S->SetWndRect(PdaGpsIconRect(m_taskIconRect, UI().get_current_kx()));
			S->TextureOn();
			S->SetStretchTexture	(true);
			m_info["t_icon_over"]->Show(true);
		}
		else
		{
			S->TextureOff			();
			m_info["t_icon_over"]->Show(false);
		}
	}

	S								= m_info["t_caption"];
	S->TextItemControl()->SetTextST	((task) ? task->m_Title.c_str() : "");
}

void CUITaskItem::OnFocusReceive()
{
	inherited::OnFocusReceive();
	show_hint_can = true;
	show_hint     = false;
}

void CUITaskItem::OnFocusLost()
{
	inherited::OnFocusLost();
	show_hint_can = false;
	show_hint     = false;
}

void CUITaskItem::Update()
{
	inherited::Update();
	if ( m_owner && m_bCursorOverWindow && show_hint_can )
	{
		if ( Device.dwTimeContinual > ( m_dwFocusReceiveTime + m_hint_wt ) )
		{
			show_hint = true;
			return;
		}
	}
}

void CUITaskItem::OnMouseScroll( float iDirection )
{
}

bool CUITaskItem::OnMouseAction( float x, float y, EUIMessages mouse_action )
{
	if ( inherited::OnMouseAction( x, y, mouse_action ) )
	{
		//return true;
	}

	switch ( mouse_action )
	{
	case WINDOW_LBUTTON_DOWN:
	case WINDOW_RBUTTON_DOWN:
	case BUTTON_DOWN:
		show_hint_can = false;
		show_hint     = false;
		break;
	}//switch

	return true;
}

void CUITaskItem::SendMessage( CUIWindow* pWnd, s16 msg, void* pData )
{
	inherited::SendMessage( pWnd, msg, pData );
}
