#pragma once

#ifndef IXR_ARM64
#include <xmmintrin.h>
#endif

namespace TaskCpuMode
{
// COP_ENGINE_PATCH: COP-ENG-255 - PPL tasks bypass xrCore::thread_entry.
inline void Ensure()
{
#ifndef IXR_ARM64
#ifdef IXR_WINDOWS
    constexpr unsigned requiredMode = 0x8040u; // FTZ and DAZ, as in _initialize_cpu_thread.
#else
    constexpr unsigned requiredMode = 0x8000u; // The existing non-Windows policy sets FTZ.
#endif
    // A reused pool worker can lose its mode in another task. Check each entry,
    // but do not reinstall the CRT handlers on the normal per-frame path.
    if ((_mm_getcsr() & requiredMode) == requiredMode)
        return;
#endif
    _initialize_cpu_thread();
}
}
