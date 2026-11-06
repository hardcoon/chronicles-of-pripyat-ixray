#pragma once

// A small Release interface. It does not change the existing render vtables.
struct ScriptDebugVertex
{
    Fvector position;
    u32 color;
};

struct ScriptPreviewVertex
{
    Fvector position;
    u32 color;
    Fvector2 uv;
};

class IScriptDebugRender
{
public:
    virtual void DrawLines(const ScriptDebugVertex* vertices, u32 vertexCount) = 0;
    virtual void DrawTriangles(const ScriptPreviewVertex* vertices, u32 vertexCount, LPCSTR texture) = 0;
    virtual void ClearModels() = 0;
protected:
    ~IScriptDebugRender() = default;
};
