#pragma once
struct lua_State;
namespace ScriptEditorSupport
{
    enum InputEvent { Press = 0, Hold = 1, Release = 2, MouseMove = 3, MouseWheel = 4 };
    void Register(lua_State* state);
    void Render();
    void Clear();
    bool Input(int event, int key, int action, int dx = 0, int dy = 0);
}
