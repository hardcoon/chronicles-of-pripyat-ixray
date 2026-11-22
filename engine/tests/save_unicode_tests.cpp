#include "../src/xrGame/save_pair_transaction.h"
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::printf("FAIL line %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
using namespace save_pair239;
struct FailMove : Files
{
    inline static int calls = 0, fail_at = 0;
    static bool move(const std::string& a, const std::string& b) { return ++calls != fail_at && Files::move(a,b); }
};
struct FailBackup : Files { static bool copy(const std::string&,const std::string&) { return false; } };
struct FailJournal : Files { static bool write_journal(const std::string&,Journal) { return false; } };
struct FailRestore : Files { static bool copy(const std::string&,const std::string&) { return false; } };
static void put(const std::string& name, const std::string& value) { CHECK(save_path251::write(name,value.data(),value.size())); }
static std::string get(const std::string& name) { std::string data;CHECK(save_path251::read(name,data));return data; }
int main(int argc,char** argv)
{
    CHECK(argc==2);
    const std::string root=argv[1];
    std::filesystem::create_directories(save_path251::wide(root));
    const std::vector<std::string> names={"old-ascii", "Тест И Ё", "\xD2\xE5\xF1\xF2", "Éclair déjà sauvegardé", "Р±С‹СЃС‚СЂРѕРµ"};
    for (const auto& name:names)
    {
        const Paths p((root+"/"+name+".scop").c_str());
        auto clean=[&]() { for(const auto& path:{p.main,p.data,p.staged_main,p.staged_data,p.backup_main,p.backup_data,p.journal}) CHECK(Files::remove(path)); };
        auto seed=[&]() { clean();put(p.main,"OLD_MAIN");put(p.data,"OLD_DATA");put(p.staged_main,"NEW_MAIN");put(p.staged_data,"NEW_DATA"); };
        auto old=[&]() { CHECK(get(p.main)=="OLD_MAIN");CHECK(get(p.data)=="OLD_DATA"); };
        seed();CHECK(commit(p,true));CHECK(get(p.main)=="NEW_MAIN");CHECK(get(p.data)=="NEW_DATA");CHECK(!Files::exists(p.journal));
        for(int fail=1;fail<=2;++fail) { seed();FailMove::calls=0;FailMove::fail_at=fail;CHECK(!commit<FailMove>(p,true));old(); }
        seed();CHECK(!commit<FailBackup>(p,true));old();
        seed();CHECK(!commit<FailJournal>(p,true));old();
        seed();CHECK(Files::copy(p.main,p.backup_main));CHECK(Files::copy(p.data,p.backup_data));
        Journal journal;journal.old_files=3;CHECK(Files::write_journal(p.journal,journal));CHECK(Files::move(p.staged_main,p.main));
        CHECK(!recover<FailRestore>(p));CHECK(Files::exists(p.journal));CHECK(recover(p));old();
        clean();journal.old_files=0;CHECK(Files::write_journal(p.journal,journal));put(p.main,"PARTIAL");CHECK(recover(p));CHECK(!Files::exists(p.main));
        seed();put(p.journal,"broken");CHECK(!recover(p));old();CHECK(Files::remove(p.journal));
        seed();CHECK(commit(p,false));CHECK(!Files::exists(p.data));
        // Save payloads stay byte-for-byte, including embedded NULs and extended packet headers.
        const uint32_t header[3]={0xfffffffe,6,80000};const std::string binary("A\0B\xFF",4);
        CHECK(Files::write_save(p.staged_main,header,binary.data(),(uint32_t)binary.size()));
        CHECK(get(p.staged_main)==std::string((const char*)header,sizeof(header))+binary);
        HANDLE locked=CreateFileW(save_path251::wide(p.staged_main).c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        CHECK(locked!=INVALID_HANDLE_VALUE);CHECK(!Files::write_save(p.staged_main,header,binary.data(),(uint32_t)binary.size()));CloseHandle(locked);
        clean();
    }
    // The old ANSI writer created a different physical file. Restoring last-save
    // settings must pick its newest pair automatically, without renaming saves.
    const std::string name="kord - быстрое сохранение [1]";
    const std::string base=root+"/"+name+".scop";
    put(base,"OLD");
    HANDLE legacy=CreateFileA(base.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    CHECK(legacy!=INVALID_HANDLE_VALUE);DWORD written;CHECK(WriteFile(legacy,"NEW",3,&written,nullptr));
    FILETIME newer={0,0x01F00000};CHECK(SetFileTime(legacy,nullptr,nullptr,&newer));CloseHandle(legacy);
    const auto selected=save_path251::restored_last_name(name,root+"/",".scop");CHECK(selected!=name);
    CHECK(get(root+"/"+selected+".scop")=="NEW");CHECK(get(base)=="OLD");
    CHECK(Files::remove(base));CHECK(Files::remove(root+"/"+selected+".scop"));
    std::puts("PASS: five filename forms, complete save pairs, both rename failures, journal/backup failures, interrupted commits, retry recovery, malformed journal, missing sidecar, binary payload, locked file, automatic old-ANSI last-save selection");
}
