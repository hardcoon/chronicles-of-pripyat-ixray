#define IXR_WINDOWS
#include <xmmintrin.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <thread>
#include <vector>

static thread_local unsigned initializationCalls = 0;
static std::atomic<unsigned> assertions = 0;
static std::atomic<unsigned> failures = 0;

// Only the existing xrCore initializer is substituted. The tested entry guard
// below is the actual production header, including its fast per-task path.
void _initialize_cpu_thread()
{
    ++initializationCalls;
    _mm_setcsr(_mm_getcsr() | 0x8040u);
}
#include "../src/xrEngine/TaskCpuMode.h"

void check(bool value,const char* message)
{
    ++assertions;
    if (!value) { ++failures; std::printf("FAIL: %s\n",message); }
}
using Matrix = std::array<float,16>;
__declspec(noinline) Matrix multiply(const Matrix& a,const Matrix& b)
{
    Matrix result{};
    for (unsigned row=0;row<4;++row) for (unsigned column=0;column<3;++column) {
        result[row*4+column]=a[column]*b[row*4]+a[4+column]*b[row*4+1]+a[8+column]*b[row*4+2];
        if (row==3) result[row*4+column]+=a[12+column];
    }
    result[15]=1;
    return result;
}
Matrix fromBits(const std::array<std::uint32_t,16>& bits)
{
    Matrix result;
    std::memcpy(result.data(),bits.data(),sizeof(result));
    return result;
}
unsigned invalidBits(const Matrix& matrix)
{
    unsigned count=0;
    for (float value:matrix) {
        std::uint32_t bits;std::memcpy(&bits,&value,4);
        auto exponent=bits&0x7f800000u, fraction=bits&0x7fffffu;
        if (exponent==0x7f800000u || (exponent==0 && fraction)) ++count;
    }
    return count;
}
bool valid(float value)
{
    int kind=std::fpclassify(value);
    return kind!=FP_NAN && kind!=FP_INFINITE && kind!=FP_SUBNORMAL;
}
void workerCase(const Matrix& parent,const Matrix& bind)
{
    const unsigned previous=_mm_getcsr();
    _mm_setcsr(0x1fb7u); // Actual failing PPL worker mode, with FTZ/DAZ absent.
    check(invalidBits(multiply(parent,bind))==2,"captured failure reproduces");
    TaskCpuMode::Ensure();
    check((_mm_getcsr()&0x8040u)==0x8040u,"first pool task initializes FTZ/DAZ");
    check(initializationCalls==1,"first task initializes once");
    check(invalidBits(multiply(parent,bind))==0,"real display matrices contain no subnormal output");
    for (unsigned entry=0;entry<1000;++entry) TaskCpuMode::Ensure();
    check(initializationCalls==1,"normal task entries avoid CRT reinitialization");
    _mm_setcsr(0x5f80u); // Another task resets the mode and selects upward rounding.
    TaskCpuMode::Ensure();
    check(initializationCalls==2,"later task restores a reset worker");
    check((_mm_getcsr()&0xe040u)==0xc040u,"unrelated rounding control is preserved");
    check(invalidBits(multiply(parent,bind))==0,"restored worker handles the captured case");
    _mm_setcsr(previous);
}
int main()
{
    const Matrix parent=fromBits({0x3f800001u,0x80107c64u,0x33c00000u,0,0x00107c5du,0x3f800001u,0x01ab3cdau,0,0xb3c00000u,0x81ab3cdbu,0x3f800002u,0,0x3a100000u,0x3d1fd900u,0xb8800000u,0x3f800000u});
    const Matrix bind=fromBits({0x3f7fffffu,0,0,0,0,0x3f7fffffu,0x80000000u,0,0x80000000u,0,0x3f7ffffeu,0,0xba0e212eu,0x3e65ee57u,0x388b9c4cu,0x3f800000u});
    std::vector<std::thread> workers;
    for (unsigned index=0;index<8;++index) workers.emplace_back([&]{workerCase(parent,bind);});
    for (auto& worker:workers) worker.join();
    const unsigned previous=_mm_getcsr();
    Matrix identity{};identity[0]=identity[5]=identity[10]=identity[15]=1;
    Matrix normal=identity;normal[12]=128.5f;normal[13]=-7.25f;normal[14]=3.f;
    _mm_setcsr(0x1f80u);Matrix before=multiply(normal,identity);
    TaskCpuMode::Ensure();Matrix after=multiply(normal,identity);
    check(std::memcmp(before.data(),after.data(),sizeof(before))==0,"ordinary transforms remain bit-identical");
    check(!valid(std::numeric_limits<float>::quiet_NaN()),"NaN remains invalid");
    check(!valid(std::numeric_limits<float>::infinity()),"positive infinity remains invalid");
    check(!valid(-std::numeric_limits<float>::infinity()),"negative infinity remains invalid");
    _mm_setcsr(previous);
    std::printf("task_cpu_mode255: %u checks, %u failures; real crash matrices, 8 reused pool workers\n",assertions.load(),failures.load());
    return failures.load()?1:0;
}
