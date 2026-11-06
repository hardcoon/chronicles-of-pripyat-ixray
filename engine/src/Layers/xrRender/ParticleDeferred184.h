#pragma once
// COP_ENGINE_PATCH: COP-ENG-184 - render-local owner lifetime and queue mutation
// share one recursive gate. Lock it BEFORE an owner's onframe_lock. Destruction
// and recompile cancel the owner while holding that same gate, including entries
// already moved into the current drain batch. No SItem addresses escape a group.
#include <algorithm>
#include <mutex>
#include <vector>

namespace PS {
template<class Owner> class DeferredOwners184
{
    std::recursive_mutex gate;
    std::vector<Owner*> pending;
    std::vector<Owner*> batch;
    bool draining = false;
public:
    std::unique_lock<std::recursive_mutex> Lock()
    {
        return std::unique_lock<std::recursive_mutex>(gate);
    }
    void Enqueue(Owner* owner)
    {
        auto lock = Lock();
        if (std::find(pending.begin(), pending.end(), owner) == pending.end())
            pending.push_back(owner);
    }
    void Cancel(Owner* owner)
    {
        auto lock = Lock();
        pending.erase(std::remove(pending.begin(), pending.end(), owner), pending.end());
        for (auto& entry : batch)
            if (entry == owner) entry = nullptr;
    }
    template<class Callback> void Drain(Callback callback)
    {
        auto lock = Lock();
        if (draining) return;
        draining = true;
        batch.swap(pending);
        // A callback may cancel another owner or schedule next-frame work.
        // The gate remains held until the batch is finished, so destruction
        // from another thread cannot invalidate a selected owner.
        try {
            for (std::size_t i = 0; i < batch.size(); ++i) {
                Owner* owner = batch[i];
                batch[i] = nullptr;
                if (owner) callback(owner);
            }
        } catch (...) {
            batch.clear();
            draining = false;
            throw;
        }
        batch.clear();
        draining = false;
    }
};
}
