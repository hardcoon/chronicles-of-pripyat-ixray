// COP_ENGINE_PATCH: COP-ENG-254 - validate the actual transactional heat buffer.
#include "../src/Layers/xrRender/WinterHeat.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <thread>

using namespace cop_winter_heat;
static unsigned checks = 0;
static void require(bool value)
{
    ++checks;
    if (!value) { std::fprintf(stderr, "check %u failed\n", checks); std::exit(1); }
}
static bool empty(const Buffer& values)
{
    for (const auto& row : values) for (float value : row) if (value != 0) return false;
    return true;
}
int main()
{
    State state;
    Buffer out{};
    const Vector centre{1,2,3,2.15f}, setup{0.75f,0.2f,-0.3f,0};
    state.snapshot(out,0,false); require(empty(out));
    require(!state.commit(1,0)); require(!state.commit(capacity+1,0));
    require(!state.set(capacity,centre,setup));
    require(state.set(0,centre,setup));
    state.snapshot(out,1,false); require(empty(out));
    require(state.commit(1,2));
    state.snapshot(out,3,false);
    require(out[0]==centre && out[1]==setup && out[header_index][3]==protocol+1);
    require(!state.commit(1,4)); // No old pending row may become a new transaction.
    state.snapshot(out,5002,false); require(out[0]==centre);
    state.snapshot(out,5003,false); require(empty(out));
    require(state.set(0,centre,setup)); require(state.commit(1,10));
    state.snapshot(out,999999,true); require(out[0]==centre);
    state.snapshot(out,1000000,false); require(out[0]==centre);
    require(state.commit(0,1000001)); state.snapshot(out,1000002,false); require(empty(out));
    for (unsigned axis=0;axis<4;++axis)
    {
        for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                             std::numeric_limits<float>::infinity(),
                             -std::numeric_limits<float>::infinity()})
        {
            auto bad=centre;bad[axis]=invalid;require(!state.set(0,bad,setup));
            auto meta=setup;meta[axis]=invalid;require(!state.set(0,centre,meta));
        }
    }
    for (float invalid : {-1.0f,0.0f,8.01f})
    { auto bad=centre;bad[3]=invalid;require(!state.set(0,bad,setup)); }
    for (float invalid : {-0.01f,1.01f})
    { auto bad=setup;bad[0]=invalid;require(!state.set(0,centre,bad)); }
    for (unsigned axis=1;axis<3;++axis)
    { auto bad=setup;bad[axis]=0.801f;require(!state.set(0,centre,bad)); }
    require(state.set(0,centre,setup));
    auto invalid=centre;invalid[0]=1000000;require(!state.set(0,invalid,setup));
    require(!state.commit(1,0)); // An invalid replacement invalidates that pending row.
    for (unsigned count=0;count<=capacity;++count)
    {
        for (unsigned index=0;index<count;++index) require(state.set(index,centre,setup));
        require(state.commit(count,100));state.snapshot(out,101,false);
        require(count ? out[header_index][3]==protocol+float(count) : empty(out));
        for (unsigned index=count*2;index<header_index;++index) require(out[index]==Vector{});
    }
    require(state.set(0,centre,setup));require(state.commit(1,0xfffffff0u));
    state.snapshot(out,0x00000010u,false);require(out[0]==centre); // Process-clock wrap.
    std::atomic<bool> done{false}, coherent{true};
    std::thread reader([&] {
        Buffer sample{};
        while (!done.load())
        {
            state.snapshot(sample,200,true);
            const unsigned count=unsigned(sample[header_index][3]-protocol);
            if (count==capacity)
                for (unsigned index=1;index<capacity;++index)
                    if (sample[index*2][0]!=sample[0][0]) coherent=false;
        }
    });
    for (unsigned turn=0;turn<1000;++turn)
    {
        auto row=centre;row[0]=float(turn);
        for (unsigned index=0;index<capacity;++index) state.set(index,row,setup);
        require(state.commit(capacity,200));
    }
    done=true;reader.join();require(coherent.load());
    std::printf("PASS %u heat-buffer checks\n",checks);
}
