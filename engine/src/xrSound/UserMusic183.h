#pragma once
#include <cstdint>

// COP_ENGINE_PATCH: COP-ENG-183 - explicit, versioned opt-in for the furniture radio.
// No sound-manager vtable or existing public class layout changes.
#ifndef XRSOUND_API
#define XRSOUND_API
#endif
extern "C" {
XRSOUND_API unsigned xrUserMusic183_Count();
XRSOUND_API const char* xrUserMusic183_Name(unsigned index);
XRSOUND_API const char* xrUserMusic183_Token(unsigned index);
XRSOUND_API bool xrUserMusic183_Valid(const char* token);
}
struct UserMusic183Decoder;
void UserMusic183_SetRoot(const char* root);
bool UserMusic183_IsToken(const char* name);
bool UserMusic183_Info(const char* name, unsigned& rate, std::uint64_t& frames);
UserMusic183Decoder* UserMusic183_Open(const char* name);
void UserMusic183_Close(UserMusic183Decoder*& decoder);
void UserMusic183_Read(UserMusic183Decoder* decoder, std::uint64_t frame, short* output, unsigned frames);
