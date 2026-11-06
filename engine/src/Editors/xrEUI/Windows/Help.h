#pragma once

class XREUI_API CUIHelp:
	public IEditorWnd
{

private:
	CUIHelp();
	virtual void Draw() override;

public:
	static CUIHelp& Instance();
	
	void Show();
};