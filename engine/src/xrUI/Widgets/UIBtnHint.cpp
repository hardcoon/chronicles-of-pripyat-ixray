#include "stdafx.h"
#include "UIBtnHint.h"
#include "UIFrameWindow.h"
#include "UIStatic.h"
#include "UIXmlInit.h"
#include "../ui_base.h"

UI_API CUIButtonHint* g_btnHint = nullptr; 
UI_API CUIButtonHint* g_statHint = nullptr;

CUIButtonHint::CUIButtonHint	()
:m_ownerWnd(nullptr),m_enabledOnFrame(false)
{
//	Device.seqRender.Add		(this, REG_PRIORITY_LOW-1000);

	CUIXmlInit					xml_init;
	CUIXml						uiXml;
	uiXml.Load					(CONFIG_PATH, UI_PATH, "hint_item.xml");
	xml_init.InitFrameWindow	(uiXml,"button_hint",0,this);
	
	m_text						= new CUITextWnd();
	m_text->SetAutoDelete		(true);
	AttachChild					(m_text);
	xml_init.InitTextWnd		(uiXml,"button_hint:description",0,m_text);
}

CUIButtonHint::~CUIButtonHint	()
{
//	Device.seqRender.Remove		(this);
}

// COP_ENGINE_PATCH: COP-ENG-279 - consume each hint on the canvas that queued it.
void CUIButtonHint::Draw_()
{
    m_enabledOnFrame = true;
    m_frameOnDevice = PdaUiCanvasScope::Active();
}

void CUIButtonHint::OnRender()
{
    if (!m_enabledOnFrame) return;
    m_enabledOnFrame = false;
    if (!m_ownerWnd || m_frameOnDevice != PdaUiCanvasScope::Active()) return;

    m_text->Update();
    SetTextureColor(color_rgba(255, 255, 255, color_get_A(m_text->GetTextColor())));
    Draw();
}

void CUIButtonHint::SetHintText	(CUIWindow* w, LPCSTR text)
{
	m_ownerWnd					= w;
	m_text->SetTextST			(text);

	m_text->AdjustHeightToText	();

	Fvector2					new_size;
	new_size.x					= GetWndSize().x;
	new_size.y					= m_text->GetWndSize().y+20.0f;

	SetWndSize					(new_size);

	m_text->ResetColorAnimation	();
}
