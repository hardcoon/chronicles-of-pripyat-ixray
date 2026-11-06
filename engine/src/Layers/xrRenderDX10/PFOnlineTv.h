#pragma once
// COP_ENGINE_PATCH: COP-ENG-194 - bounded online video for furniture screens.
namespace PFOnlineTv
{
    constexpr unsigned Width = 640, Height = 360;
    bool IsTexture(const char* name);
    void Command(const char* args);
    void Status(char* output, unsigned size); // COP_ENGINE_PATCH: COP-ENG-195
    void Tick();
    bool CopyFrame(void* target, unsigned stride);
    void Stop();
    void Shutdown();
}
