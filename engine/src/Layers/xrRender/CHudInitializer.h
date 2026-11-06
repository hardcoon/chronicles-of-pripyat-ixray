#pragma once

class CHudInitializer {
public:
	CHudInitializer(bool);
	~CHudInitializer();

	void SetHudMode();
	void SetDefaultMode();
private:
	bool b_auto_setup;
	bool m_hud_render_process_saved; // COP_ENGINE_PATCH: COP-ENG-011
	Fmatrix mView_saved;
	Fmatrix mProject_saved;
	Fmatrix mFullTransform_saved;
};