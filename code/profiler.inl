#ifndef PROFILER_INL_H
#define PROFILER_INL_H

#ifdef __cplusplus

#ifdef TRACY_ENABLE

#undef function
#include <Tracy.hpp>
#define function static

/* RAII zone for the rest of the current scope. Ends automatically on return. */
#define Profile_Scope(name) ZoneScopedN(#name)

/* RAII frame section plus zone; C++ equivalent of Begin_Profile_Frame / End_Profile_Frame. */
#define Profile_Frame(name) \
    FrameMarkStart(#name); \
    ZoneScopedN(#name); \
    struct _profile_frame_guard_##__LINE__ { \
        ~_profile_frame_guard_##__LINE__() { FrameMarkEnd(#name); } \
    } _profile_frame_guard_##__LINE__

#else

#define Profile_Scope(name)
#define Profile_Frame(name)

#endif

#endif /* __cplusplus */

#endif /* PROFILER_INL_H */
