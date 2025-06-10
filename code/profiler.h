#ifndef PROFILER_H
#define PROFILER_H

#ifdef TRACY_ENABLE
#pragma comment(lib, "TracyClient.lib")
#undef function
#include <TracyC.h>
#define function static

#define Begin_Profile_Frame(name) TracyCFrameMarkStart(#name) TracyCZoneN(name_##ctx, #name, true)
#define End_Profile_Frame(name) TracyCFrameMarkEnd(#name) TracyCZoneEnd(name_##ctx)

#define Begin_Profile_Scope(name) TracyCZoneN(name_##ctx, #name, true)
#define End_Profile_Scope(name) TracyCZoneEnd(name_##ctx)

#else
#define Begin_Profile_Frame(name) 
#define End_Profile_Frame(name)

#define Begin_Profile_Scope(name) 
#define End_Profile_Scope(name)
#endif

#endif