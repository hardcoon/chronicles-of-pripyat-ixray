#pragma once
#include "../../xrUI/Widgets/UIWindow.h"
#include "../../xrUI/Widgets/UIWndCallback.h"
#include "../../xrCore/associative_vector.h"
#include "../GameTaskDefs.h"
#include "../../xrUI/Widgets/UICheckButton.h"
#include "../PdaMapFilters.h"

class CUIMapWnd;
class CUIStatic;
class CGameTask;
class CUIXml;
class CUITaskItem;
class CUI3tButton;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUICheckButton;
class CPdaMapFilterButton;
class CGameFont;
class UITaskListWnd;
class UIMapLegend;
class UIHint;

class CUITaskWnd			:	public CUIWindow, 
								public CUIWndCallback
{
private:
	typedef CUIWindow		inherited;

	CUIFrameWindow*			m_background;
	CUIFrameLineWnd*		m_background2;

	CUIStatic*				m_center_background;
	CUIStatic*				m_right_bottom_background;
	CUIFrameLineWnd*		m_task_split;

	CUIMapWnd*				m_pMapWnd;
	CUITaskItem*			m_pStoryLineTaskItem;
	CUITaskItem*			m_pSecondaryTaskItem;

	CUI3tButton*			m_BtnTaskListWnd;
	CUIStatic*				m_second_task_index;
	CUIStatic*				m_devider;
	u32						m_actual_frame;

	CUI3tButton*			m_btn_focus;
	CUI3tButton*			m_btn_focus2;

    // COP_ENGINE_PATCH: COP-ENG-260 - native controls owned by the Tasks page.
    struct MapFilterButton
    {
        CPdaMapFilterButton* control;
        PdaMapFilters::Category category;
        float captionLeft;
        float captionRight;
    };
    xr_vector<MapFilterButton> m_mapFilterButtons;
    CGameFont* m_mapFilterCaptionFont = nullptr;

	UITaskListWnd*			m_task_wnd;
	bool					m_task_wnd_show;
	UIMapLegend*			m_map_legend_wnd;

public:
	UIHint*					hint_wnd;

public:
								CUITaskWnd				();
	virtual						~CUITaskWnd				();
	virtual void				SendMessage				(CUIWindow* pWnd, s16 msg, void* pData);
			void				Init					();
	virtual void				Update					();
	virtual void				Draw					();
			void				DrawHint				();
	virtual void				Show					(bool status);
	virtual void				Reset					();

			void                UserMapTaskRemoved();
			void				ReloadTaskInfo			();
	// COP_ENGINE_PATCH: COP-ENG-153 - expose the existing map to the PDA focus command.
	CUIMapWnd* GetMapWnd() const { return m_pMapWnd; }
			void				ShowMapLegend			(bool status);
			void				Switch_ShowMapLegend	();

    void SyncMapFilters();

			void				Show_TaskListWnd		(bool status);

			virtual CUIWindow* ui_cast_window() { return this; }

private:
	void						TaskSetTargetMap		(CGameTask* task);
	void						TaskShowMapSpot			(CGameTask* task, bool show);

	void						OnNextTaskClicked		();
	void 						OnPrevTaskClicked		();
	void 				OnShowTaskListWnd		(CUIWindow* w, void* d);
	void 				OnTask1DbClicked		(CUIWindow*, void*);
	void				OnTask2DbClicked		(CUIWindow*, void*);

    void OnMapFilterClicked(CUIWindow*, void*);
    void InitMapFilters(CUIXml& xml);
    void FitMapFilterCaptions();

};

class CUITaskItem : public CUIWindow
{
private:
	typedef CUIWindow			inherited;

	associative_vector<shared_str, CUIStatic*>			m_info;
	CGameTask*											m_owner;
    Frect m_taskIconRect; // COP_ENGINE_PATCH: COP-ENG-107 - restore stock bounds between tasks.
public:
								CUITaskItem				();
	virtual						~CUITaskItem			() = default;

	virtual void 				OnFocusReceive			();
	virtual void	 			OnFocusLost				();
	virtual void				Update					();
	virtual void				OnMouseScroll			(float iDirection);
	virtual bool				OnMouseAction					(float x, float y, EUIMessages mouse_action);
	virtual void				SendMessage				(CUIWindow* pWnd, s16 msg, void* pData);

	void						Init					(CUIXml& uiXml, LPCSTR path);
	void						InitTask				(CGameTask* task);
	CGameTask*					OwnerTask				()							{return m_owner;}

	virtual CUIWindow* ui_cast_window() { return this; }

public:
	bool						show_hint_can;
	bool						show_hint;

protected:
	u32							m_hint_wt;
};
