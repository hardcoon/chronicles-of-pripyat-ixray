#include "../src/xrGame/server_name252.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#define CHECK(x) do { if(!(x)){std::printf("FAIL %d: %s\n",__LINE__,#x);std::exit(1);} } while(0)
int main()
{
    std::vector<std::string> names={"SKANER","kord - быстрое сохранение [1]",
        "kord - Р±С‹СЃС‚СЂРѕРµ СЃРѕС…СЂР°РЅРµРЅРёРµ [1]", "Éclair déjà sauvegardé",
        "\xEA\xEE\xF0\xE4 - \xF1\xE5\xE9\xE2"};
    for(size_t size:{62,63,64,65,83,127,128,129,254,255,256,4096})names.emplace_back(size,'x');
    for(const auto& name:names)
    {
        CHECK(server_name252::first((name+"/single/alife/load").c_str())==name);
        CHECK(server_name252::first(name.c_str())==name);
        CHECK(server_name252::first((" \t"+name+"\t /single/alife/load").c_str())==name);
    }
    CHECK(server_name252::first("")=="");CHECK(server_name252::first(nullptr)=="");
    CHECK(server_name252::first("/single")=="");CHECK(server_name252::first(" \t /single")=="");
    std::puts("PASS: actual 83-byte broken save name, UTF8/CP1251/French, 64/128/256-byte boundaries, exact bytes and legacy trim semantics.");
}
