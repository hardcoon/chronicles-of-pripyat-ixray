#pragma once

class XREUI_API CUIThemeManager :
	public IEditorWnd
{

	CUIThemeManager();
	~CUIThemeManager() = default;
	
public:
	static CUIThemeManager& Get();
	void InitDefault(bool Force = false);
	void Show(bool value);

	void Save();
	void Load();
	void SaveTo();
	void LoadFrom();

protected:
	virtual void Draw();
	bool IsLoaded = false;

public:
	float TransparentDefault = 1.f;
	float TransparentUnfocused = 0.33f;

	// St4lker0k765: customizable log message colors
	ImVec4 log_color_default;
	ImVec4 log_color_error;
	ImVec4 log_color_warning;
	ImVec4 log_color_debug;
};