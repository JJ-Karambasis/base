#ifndef PROFILER_H
#define PROFILER_H

#ifdef TRACY_ENABLE
#pragma comment(lib, "TracyClient.lib")
#undef function
#include <TracyC.h>
#define function static

/* Named frame boundary at a single point (e.g. end of the main loop). */
#define Profile_Frame_Mark(name) TracyCFrameMarkNamed(#name)

/* Frame section with manual begin/end; also opens a zone between them. */
#define Begin_Profile_Frame(name) TracyCFrameMarkStart(#name) TracyCZoneN(name_##ctx, #name, true)
#define End_Profile_Frame(name) TracyCFrameMarkEnd(#name) TracyCZoneEnd(name_##ctx)

#define Begin_Profile_Scope(name) TracyCZoneN(name_##ctx, #name, true)
#define End_Profile_Scope(name) TracyCZoneEnd(name_##ctx)

#else

#define Profile_Frame_Mark(name)
#define Begin_Profile_Frame(name)
#define End_Profile_Frame(name)
#define Begin_Profile_Scope(name)
#define End_Profile_Scope(name)

#endif

#include "profiler.inl"

#endif
