#ifndef BUILD_FRAMEWORK_H
#define BUILD_FRAMEWORK_H

/*
================================================================================
  build_framework.h — single-header C build helper (stb-style)

  Drop this into a build.c:

      #define BUILD_FRAMEWORK_IMPLEMENTATION
      #include "build_framework.h"

      int main(int ArgCount, char** Args) {
          build_context Ctx = Build_Init(ArgCount, Args);
          // ... build steps ...
          Build_Quit(&Ctx);
          return Ctx.Failed ? 1 : 0;
      }

  Compile that translation unit with your system compiler (cl / cc); the resulting
  binary is your project build driver. Prefer Build_Os() / toolchain fields at
  runtime over #ifdef for host differences.

--------------------------------------------------------------------------------
  Strings & arena
--------------------------------------------------------------------------------
  Paths and command args are build_string {Ptr, Size} — not required to be
  NUL-terminated. Literals:

      build_string S = Build_Str_Lit("hello");

  Almost every API that creates text takes build_context* first and bumps a
  virtual-reserved arena (stable pointers for the process lifetime):

      build_string Path = Build_Path_Join(Ctx, Ctx->Root, Build_Str_Lit("bin"));
      build_string Msg  = Build_Fmt(Ctx, "out=%.*s", (int)Path.Size, Path.Ptr);

  For C APIs that need a C string: Build_Cstr(Ctx, Path).

  Lists grow on the arena too:

      build_str_list Libs = {0};
      Build_Str_List_Push_Lit(Ctx, &Libs, "base.lib");
      Build_Str_List_Push_Many(Ctx, &Libs, "kernel32.lib", "user32.lib");
      Build_Str_List_PushV(Ctx, &Libs, SomePath, AnotherPath, BUILD_STR_END);

--------------------------------------------------------------------------------
  Context & common CLI flags
--------------------------------------------------------------------------------
  Build_Init parses shared flags into Ctx and leaves the rest in Ctx.UserArgs
  for your own option parser:

      -debug   -force   -asan   -tsan   -ubsan   -bin_path <dir>

  Ctx.Root     = project root (cwd, or parent of bin/ when the driver lives in bin/)
  Ctx.RepoRoot = git root for submodule helpers (defaults to Root; set if nested)
  Ctx.BinPath  = output directory (default <Root>/bin)

--------------------------------------------------------------------------------
  Toolchains & artifact paths
--------------------------------------------------------------------------------
      build_toolchain Tc = Build_Toolchain_Detect(Ctx, PreferClang);
      // or Build_Toolchain_MSVC() / _Clang() / _Gcc()

      build_string Obj = Build_Obj_Path(Ctx, &Tc, Dir, Build_Str_Lit("foo")); // foo.obj / foo.o
      build_string Lib = Build_Lib_Path(Ctx, &Tc, Dir, Build_Str_Lit("foo")); // foo.lib / libfoo.a
      build_string Exe = Build_Exe_Path(Ctx, &Tc, Dir, Build_Str_Lit("foo"));
      build_string Dll = Build_Dll_Path(Ctx, &Tc, Dir, Build_Str_Lit("foo"));

--------------------------------------------------------------------------------
  Compile → archive → link
--------------------------------------------------------------------------------
  One file:

      build_compile_opts Co = {0};
      Co.Toolchain = Tc; Co.Cpp = 1; Co.Debug = Ctx->Debug; Co.Standard = 20;
      Build_Str_List_Push(Ctx, &Co.Includes, CodeDir);
      if (!Build_Compile(Ctx, &Co, SrcPath, ObjPath)) return 0;

  Many files (batched into one cl/clang when possible — MSVC uses /FoObjDir\):

      build_str_list Sources = {0}, Objs = {0};
      Build_Str_List_Push(Ctx, &Sources, A_c);
      Build_Str_List_Push(Ctx, &Sources, B_c);
      if (!Build_Compile_Many(Ctx, &Co, &Sources, ObjDir, &Objs)) return 0;
      if (!Build_Archive(Ctx, &Tc, &Objs, LibPath)) return 0;

  Compile + link one TU into an exe/dll:

      build_link_opts Lo = {0};
      Lo.Toolchain = Tc; Lo.Cpp = 1; Lo.OutPath = ExePath;
      Build_Str_List_Push_Lit(Ctx, &Lo.Libs, "base.lib");
      if (!Build_Compile_And_Link(Ctx, &Co, &Lo, MainCpp)) return 0;

  Or fill a build_target and call Build_Target().

  Rebuild skipping uses mtimes unless Ctx->Force is set.

--------------------------------------------------------------------------------
  Shelling out
--------------------------------------------------------------------------------
  Raw argv builder (logged as "[build] > ..."):

      build_cmd Cmd = {0};
      Build_Cmd_Push_Many(Ctx, &Cmd, "cmake", "--build", "build");
      Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--config"), Config, BUILD_STR_END);
      if (Build_Cmd_Run(Ctx, &Cmd) != 0) return 0;

  Convenience:

      Build_Run(Ctx, ExePath, Build_Str_Lit("-only_shaders"), BUILD_STR_END);
      Build_Run_Script(Ctx, Build_Str_Lit("code/third_party/base/build"),
                       Build_Str_Lit("-cpp"), BUILD_STR_END);
      // Script stem has no extension — picks .bat / .sh for the host OS.

  Stage a built artifact into BinPath (or any dir):

      Build_Stage_File(Ctx, BuiltLib, Ctx->BinPath);

--------------------------------------------------------------------------------
  Git submodules & CMake deps
--------------------------------------------------------------------------------
  Ensure = init only when not already checked out (safe for everyday builds):

      if (!Build_Git_Submodule_Ensure(Ctx, Build_Str_Lit("code/third_party/SDL"))) return 0;

  Update = always `git submodule update --init` (opt-in when a pin moved):

      if (!Build_Git_Submodule_Update(Ctx, RelPath, 0)) return 0;  // 0 = not recursive

  If git is missing, Build_Git_Require() prints install hints and may offer
  winget/brew/apt when stdin is a TTY.

  Vendored CMake projects:

      build_cmake Cmake = {0};
      Cmake.SourceDir = SdlDir;
      Cmake.BuildDir = Build_Path_Join(Ctx, SdlDir, Build_Str_Lit("build"));
      Cmake.InstallPrefix = InstallDir;
      Cmake.Config = Build_Str_Lit("RelWithDebInfo");
      Build_Str_List_Push_Many(Ctx, &Cmake.ConfigureArgs, "-DSDL_SHARED=OFF", "-DSDL_STATIC=ON");
      if (!Build_Cmake(Ctx, &Cmake)) return 0;

--------------------------------------------------------------------------------
  Minimal static lib example
--------------------------------------------------------------------------------
      build_context Ctx = Build_Init(ArgCount, Args);
      build_toolchain Tc = Build_Toolchain_Detect(&Ctx, 0);
      build_compile_opts Co = {0};
      Co.Toolchain = Tc; Co.Standard = 11;
      build_str_list Sources = {0}, Objs = {0};
      Build_Str_List_Push(&Ctx, &Sources,
          Build_Path_Join(&Ctx, Ctx.Root, Build_Str_Lit("src/a.c")));
      Build_Str_List_Push(&Ctx, &Sources,
          Build_Path_Join(&Ctx, Ctx.Root, Build_Str_Lit("src/b.c")));
      Build_Compile_Many(&Ctx, &Co, &Sources, Ctx.BinPath, &Objs);
      Build_Archive(&Ctx, &Tc, &Objs,
          Build_Lib_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("mylib")));
      Build_Quit(&Ctx);

================================================================================
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int build_b32;

typedef struct build_string {
    const char* Ptr;
    size_t Size;
} build_string;

#ifdef __cplusplus
#define Build_Str_Lit(s) (build_string{(s), sizeof(s) - 1})
#else
#define Build_Str_Lit(s) ((build_string){(s), sizeof(s) - 1})
#endif

#define BUILD_STR_END ((build_string){NULL, 0})

typedef enum build_os {
    BUILD_OS_WIN32,
    BUILD_OS_OSX,
    BUILD_OS_LINUX,
} build_os;

typedef enum build_compiler {
    BUILD_COMPILER_MSVC,
    BUILD_COMPILER_CLANG,
    BUILD_COMPILER_GCC,
} build_compiler;

typedef enum build_artifact {
    BUILD_ARTIFACT_OBJECTS,
    BUILD_ARTIFACT_STATIC,
    BUILD_ARTIFACT_SHARED,
    BUILD_ARTIFACT_EXE,
} build_artifact;

typedef struct build_str_list {
    build_string* Items;
    int Count;
    int Capacity;
} build_str_list;

typedef struct build_cmd {
    build_str_list Args;
} build_cmd;

typedef struct build_toolchain {
    build_compiler Compiler;
    build_string CC;
    build_string CXX;
    build_string AR;
    build_string ObjExt;
    build_string LibExt;
    build_string LibPrefix;
    build_string DllExt;
    build_string ExeExt;
} build_toolchain;

typedef struct build_context {
    build_string Root;
    build_string RepoRoot;
    build_string BinPath;
    /* Virtual-reserved bump arena: base pointer never moves; pages committed on demand. */
    char* Arena;
    size_t ArenaUsed;
    size_t ArenaCommitted;
    size_t ArenaReserved;
    build_b32 Failed;
    build_b32 Debug;
    build_b32 Force;
    build_b32 Asan;
    build_b32 Tsan;
    build_b32 Ubsan;
    int ArgCount;
    char** Args;
    build_str_list UserArgs;
} build_context;

typedef struct build_compile_opts {
    build_toolchain Toolchain;
    build_b32 Cpp;
    build_b32 ObjC;
    build_b32 Debug;
    build_b32 WarningsAsErrors;
    build_b32 PIC;
    build_b32 NoRTTI;
    build_b32 NoExceptions;
    int Standard;
    build_str_list Includes;
    build_str_list Defines;
    build_str_list ExtraFlags;
    build_b32 Asan;
    build_b32 Tsan;
    build_b32 Ubsan;
    build_b32 MsvcWall;
} build_compile_opts;

typedef struct build_link_opts {
    build_toolchain Toolchain;
    build_b32 Shared;
    build_b32 Debug;
    build_b32 Cpp;
    build_str_list Objects;
    build_str_list Libs;
    build_str_list LibDirs;
    build_str_list Frameworks;
    build_str_list ExtraFlags;
    build_string OutPath;
    build_string PdbPath;
    build_string InstallName;
} build_link_opts;

typedef struct build_target {
    build_string Name;
    build_artifact Kind;
    build_compile_opts Compile;
    build_link_opts Link;
    build_str_list Sources;
    build_str_list ExtraInputs;
    build_string OutPath;
} build_target;

typedef struct build_cmake {
    build_string SourceDir;
    build_string BuildDir;
    build_string InstallPrefix;
    build_string Config;
    build_str_list ConfigureArgs;
    build_string ExtraBuildTarget;
    build_string StampFile;
} build_cmake;

build_string Build_Make_Str(const char* Ptr, size_t Size);
build_string Build_Str_From_Cstr(const char* Ptr);
build_b32 Build_Str_Empty(build_string S);
build_b32 Build_Str_Eq(build_string A, build_string B);
build_b32 Build_Str_Eq_Lit(build_string A, const char* Lit);
build_b32 Build_Str_Ends_With(build_string S, build_string Suf);
const char* Build_Cstr(build_context* Ctx, build_string S);

build_os Build_Os(void);
build_context Build_Init(int ArgCount, char** Args);
void Build_Quit(build_context* Ctx);
build_string Build_Fmt(build_context* Ctx, const char* Fmt, ...);
build_string Build_Path_Join(build_context* Ctx, build_string A, build_string B);
void Build_Log(const char* Fmt, ...);
void Build_Log_Error(build_context* Ctx, const char* Fmt, ...);
void Build_Die(build_context* Ctx, const char* Fmt, ...);
void Build_Str_List_Push(build_context* Ctx, build_str_list* List, build_string Item);
void Build_Str_List_Push_Fmt(build_context* Ctx, build_str_list* List, const char* Fmt, ...);
void Build_Str_List_Push_Lit(build_context* Ctx, build_str_list* List, const char* Lit);
void Build_Str_List_PushV(build_context* Ctx, build_str_list* List, ...); /* build_string..., BUILD_STR_END */
void Build_Str_List_Push_Lits(build_context* Ctx, build_str_list* List, ...); /* const char*..., NULL */
void Build_Str_List_Push_Cstrs(build_context* Ctx, build_str_list* List, const char* const* Items, int Count);

#define BUILD_COUNT_OF(Arr) ((int)(sizeof(Arr) / sizeof((Arr)[0])))
/* Compound-literal batch of C string literals — no NULL sentinel needed. */
#define Build_Str_List_Push_Many(Ctx, List, ...) \
    do { \
        const char* const Build__Items_[] = { __VA_ARGS__ }; \
        Build_Str_List_Push_Cstrs((Ctx), (List), Build__Items_, BUILD_COUNT_OF(Build__Items_)); \
    } while (0)

build_b32 Build_Path_Exists(build_string Path);
build_b32 Build_Is_File(build_string Path);
build_b32 Build_Is_Dir(build_string Path);
build_b32 Build_Mkdir_All(build_string Path);
build_b32 Build_Copy_File(build_string Src, build_string Dst);
build_b32 Build_Remove_File(build_string Path);
uint64_t Build_File_Mtime(build_string Path);

void Build_Cmd_Push(build_context* Ctx, build_cmd* Cmd, build_string Arg);
void Build_Cmd_Push_Lit(build_context* Ctx, build_cmd* Cmd, const char* Lit);
void Build_Cmd_PushV(build_context* Ctx, build_cmd* Cmd, ...); /* build_string..., BUILD_STR_END */
void Build_Cmd_Push_Lits(build_context* Ctx, build_cmd* Cmd, ...); /* const char*..., NULL */
void Build_Cmd_Push_Fmt(build_context* Ctx, build_cmd* Cmd, const char* Fmt, ...);

#define Build_Cmd_Push_Many(Ctx, Cmd, ...) \
    do { \
        const char* const Build__Items_[] = { __VA_ARGS__ }; \
        for (int Build__I_ = 0; Build__I_ < BUILD_COUNT_OF(Build__Items_); Build__I_++) { \
            Build_Cmd_Push_Lit((Ctx), (Cmd), Build__Items_[Build__I_]); \
        } \
    } while (0)
void Build_Cmd_Reset(build_cmd* Cmd);
int Build_Cmd_Run(build_context* Ctx, build_cmd* Cmd);
int Build_Cmd_Run_Cwd(build_context* Ctx, build_cmd* Cmd, build_string Cwd);
void Build_Cmd_Run_Or_Die(build_context* Ctx, build_cmd* Cmd);
void Build_Cmd_Run_Cwd_Or_Die(build_context* Ctx, build_cmd* Cmd, build_string Cwd);
build_b32 Build_Needs_Rebuild(build_context* Ctx, build_string Output, const build_string* Inputs, int InputCount);
build_b32 Build_Needs_Rebuild_List(build_context* Ctx, build_string Output, build_str_list* Inputs);

build_toolchain Build_Toolchain_MSVC(void);
build_toolchain Build_Toolchain_Clang(void);
build_toolchain Build_Toolchain_Gcc(void);
build_toolchain Build_Toolchain_Detect(build_context* Ctx, build_b32 PreferClang);
build_string Build_Obj_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem);
build_string Build_Lib_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem);
build_string Build_Exe_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem);
build_string Build_Dll_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem);

void Build_Compile_Opts_Apply_Defaults(build_compile_opts* Opts);
build_b32 Build_Compile(build_context* Ctx, build_compile_opts* Opts, build_string SrcPath, build_string ObjPath);
/* Compiles Sources into ObjDir (stem.obj/.o). Dirty files are batched into one driver
   invocation when possible (MSVC /FoDir\, clang/gcc -c with Cwd=ObjDir). */
build_b32 Build_Compile_Many(build_context* Ctx, build_compile_opts* Opts, build_str_list* Sources, build_string ObjDir, build_str_list* OutObjs);
build_b32 Build_Archive(build_context* Ctx, const build_toolchain* Tc, build_str_list* Objs, build_string LibPath);
build_b32 Build_Link(build_context* Ctx, build_link_opts* Opts);
build_b32 Build_Compile_And_Link(build_context* Ctx, build_compile_opts* CompileOpts, build_link_opts* LinkOpts, build_string SrcPath);
build_b32 Build_Target(build_context* Ctx, build_target* Target);

build_b32 Build_Run(build_context* Ctx, build_string Exe, ...);
build_b32 Build_Run_Cwd(build_context* Ctx, build_string Cwd, build_string Exe, ...);
/* ScriptStem has no extension — resolves to .bat/.sh and runs via cmd /c or bash. */
build_string Build_Script_Path(build_context* Ctx, build_string ScriptStem);
build_b32 Build_Run_Script(build_context* Ctx, build_string ScriptStem, ...);
build_b32 Build_Run_Script_Cwd(build_context* Ctx, build_string Cwd, build_string ScriptStem, ...);
build_b32 Build_Stage_File(build_context* Ctx, build_string Src, build_string DstDir);

build_b32 Build_Git_Is_Available(void);
/* Returns 1 if git is on PATH. Otherwise prints install instructions and, when stdin
   is a TTY, offers to run a package-manager install (winget / brew / apt). */
build_b32 Build_Git_Require(build_context* Ctx);
build_b32 Build_Git_Submodule_Is_Checked_Out(build_context* Ctx, build_string RelPath);
build_b32 Build_Git_Submodule_Ensure(build_context* Ctx, build_string RelPath);
build_b32 Build_Git_Submodule_Ensure_Recursive(build_context* Ctx, build_string RelPath);
/* Always runs git submodule update --init (even if already checked out). */
build_b32 Build_Git_Submodule_Update(build_context* Ctx, build_string RelPath, build_b32 Recursive);
build_b32 Build_Cmake(build_context* Ctx, build_cmake* Cmake);

#ifdef __cplusplus
}
#endif

#endif /* BUILD_FRAMEWORK_H */

#ifdef BUILD_FRAMEWORK_IMPLEMENTATION
#ifndef BUILD_FRAMEWORK_IMPLEMENTATION_DONE
#define BUILD_FRAMEWORK_IMPLEMENTATION_DONE

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#endif

/* Reserve a large VA range up front; commit pages only as the bump pointer advances.
   Base address is stable for the whole process, so pointers into the arena never move. */
#ifndef BUILD_ARENA_RESERVE_BYTES
#define BUILD_ARENA_RESERVE_BYTES ((size_t)1 << 30) /* 1 GiB virtual */
#endif
#ifndef BUILD_ARENA_COMMIT_BYTES
#define BUILD_ARENA_COMMIT_BYTES ((size_t)256 << 10) /* 256 KiB commit stride */
#endif

static void* Build__Mem_Reserve(size_t Size) {
#ifdef _WIN32
    return VirtualAlloc(NULL, Size, MEM_RESERVE, PAGE_READWRITE);
#else
    void* Ptr = mmap(NULL, Size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return Ptr == MAP_FAILED ? NULL : Ptr;
#endif
}

static build_b32 Build__Mem_Commit(void* Ptr, size_t Size) {
#ifdef _WIN32
    return VirtualAlloc(Ptr, Size, MEM_COMMIT, PAGE_READWRITE) != NULL;
#else
    return mmap(Ptr, Size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) != MAP_FAILED;
#endif
}

static void Build__Mem_Release(void* Ptr, size_t Size) {
    if (!Ptr) return;
#ifdef _WIN32
    (void)Size;
    VirtualFree(Ptr, 0, MEM_RELEASE);
#else
    munmap(Ptr, Size);
#endif
}

static void Build__Arena_Commit_Up_To(build_context* Ctx, size_t Needed) {
    if (Needed <= Ctx->ArenaCommitted) return;
    if (Needed > Ctx->ArenaReserved) {
        fprintf(stderr, "[build] arena reserve exhausted (need %zu, reserved %zu)\n",
                Needed, Ctx->ArenaReserved);
        exit(1);
    }
    size_t NewCommitted = (Needed + BUILD_ARENA_COMMIT_BYTES - 1) / BUILD_ARENA_COMMIT_BYTES * BUILD_ARENA_COMMIT_BYTES;
    if (NewCommitted > Ctx->ArenaReserved) NewCommitted = Ctx->ArenaReserved;
    size_t CommitAt = Ctx->ArenaCommitted;
    size_t CommitSize = NewCommitted - CommitAt;
    if (CommitSize && !Build__Mem_Commit(Ctx->Arena + CommitAt, CommitSize)) {
        fprintf(stderr, "[build] arena commit failed (%zu bytes at %zu)\n", CommitSize, CommitAt);
        exit(1);
    }
    Ctx->ArenaCommitted = NewCommitted;
}

static char* Build__Arena_Push(build_context* Ctx, size_t Size) {
    Size = (Size + 7u) & ~(size_t)7u;
    Build__Arena_Commit_Up_To(Ctx, Ctx->ArenaUsed + Size);
    char* Result = Ctx->Arena + Ctx->ArenaUsed;
    Ctx->ArenaUsed += Size;
    return Result;
}

build_string Build_Make_Str(const char* Ptr, size_t Size) {
    build_string S;
    S.Ptr = Ptr;
    S.Size = Size;
    return S;
}

build_string Build_Str_From_Cstr(const char* Ptr) {
    if (!Ptr) return BUILD_STR_END;
    return Build_Make_Str(Ptr, strlen(Ptr));
}

build_b32 Build_Str_Empty(build_string S) {
    return !S.Ptr || S.Size == 0;
}

build_b32 Build_Str_Eq(build_string A, build_string B) {
    if (A.Size != B.Size) return 0;
    if (A.Size == 0) return 1;
    return memcmp(A.Ptr, B.Ptr, A.Size) == 0;
}

build_b32 Build_Str_Eq_Lit(build_string A, const char* Lit) {
    return Build_Str_Eq(A, Build_Str_From_Cstr(Lit));
}

build_b32 Build_Str_Ends_With(build_string S, build_string Suf) {
    if (Suf.Size > S.Size) return 0;
    return memcmp(S.Ptr + (S.Size - Suf.Size), Suf.Ptr, Suf.Size) == 0;
}

/* Arena strings are always stored with a trailing NUL (not counted in Size). */
static build_string Build__Arena_Str(build_context* Ctx, const char* Ptr, size_t Size) {
    char* D = Build__Arena_Push(Ctx, Size + 1);
    if (Size) memcpy(D, Ptr, Size);
    D[Size] = 0;
    return Build_Make_Str(D, Size);
}

const char* Build_Cstr(build_context* Ctx, build_string S) {
    if (Build_Str_Empty(S)) return "";
    if (S.Ptr[S.Size] == 0) return S.Ptr;
    return Build__Arena_Str(Ctx, S.Ptr, S.Size).Ptr;
}

static void Build__Normalize_Slashes(char* Path, size_t Size) {
    for (size_t Index = 0; Index < Size; Index++) {
#ifdef _WIN32
        if (Path[Index] == '/') Path[Index] = '\\';
#else
        if (Path[Index] == '\\') Path[Index] = '/';
#endif
    }
}

static build_string Build__Get_Cwd(build_context* Ctx) {
#ifdef _WIN32
    char Buf[MAX_PATH];
    if (!_getcwd(Buf, MAX_PATH)) return Build_Str_Lit(".");
    return Build__Arena_Str(Ctx, Buf, strlen(Buf));
#else
    char Buf[4096];
    if (!getcwd(Buf, sizeof(Buf))) return Build_Str_Lit(".");
    return Build__Arena_Str(Ctx, Buf, strlen(Buf));
#endif
}

static build_string Build__Get_Executable_Dir(build_context* Ctx) {
#ifdef _WIN32
    char Buf[MAX_PATH];
    DWORD N = GetModuleFileNameA(NULL, Buf, MAX_PATH);
    if (N == 0 || N >= MAX_PATH) return Build_Str_Lit(".");
    char* Slash = strrchr(Buf, '\\');
    if (Slash) *Slash = 0;
    return Build__Arena_Str(Ctx, Buf, strlen(Buf));
#elif defined(__APPLE__)
    char Buf[4096];
    uint32_t Size = sizeof(Buf);
    if (_NSGetExecutablePath(Buf, &Size) != 0) return Build_Str_Lit(".");
    char* Slash = strrchr(Buf, '/');
    if (Slash) *Slash = 0;
    return Build__Arena_Str(Ctx, Buf, strlen(Buf));
#else
    char Buf[4096];
    ssize_t N = readlink("/proc/self/exe", Buf, sizeof(Buf) - 1);
    if (N < 0) return Build_Str_Lit(".");
    Buf[N] = 0;
    char* Slash = strrchr(Buf, '/');
    if (Slash) *Slash = 0;
    return Build__Arena_Str(Ctx, Buf, strlen(Buf));
#endif
}

static build_string Build__Filename(build_string Path) {
    size_t Last = 0;
    build_b32 Found = 0;
    for (size_t Index = 0; Index < Path.Size; Index++) {
        if (Path.Ptr[Index] == '/' || Path.Ptr[Index] == '\\') {
            Last = Index + 1;
            Found = 1;
        }
    }
    if (!Found) return Path;
    return Build_Make_Str(Path.Ptr + Last, Path.Size - Last);
}

static build_string Build__Stem(build_context* Ctx, build_string Path) {
    build_string Name = Build__Filename(Path);
    size_t N = Name.Size;
    for (size_t Index = N; Index > 0; Index--) {
        if (Name.Ptr[Index - 1] == '.') {
            if (Index != 1) N = Index - 1;
            break;
        }
    }
    return Build__Arena_Str(Ctx, Name.Ptr, N);
}

build_os Build_Os(void) {
#ifdef _WIN32
    return BUILD_OS_WIN32;
#elif defined(__APPLE__)
    return BUILD_OS_OSX;
#else
    return BUILD_OS_LINUX;
#endif
}

void Build_Str_List_Push(build_context* Ctx, build_str_list* List, build_string Item) {
    if (List->Count >= List->Capacity) {
        int NewCap = List->Capacity ? List->Capacity * 2 : 16;
        build_string* NewItems = (build_string*)Build__Arena_Push(Ctx, sizeof(build_string) * (size_t)NewCap);
        if (List->Items && List->Count) {
            memcpy(NewItems, List->Items, sizeof(build_string) * (size_t)List->Count);
        }
        List->Items = NewItems;
        List->Capacity = NewCap;
    }
    List->Items[List->Count++] = Item;
}

void Build_Str_List_Push_Lit(build_context* Ctx, build_str_list* List, const char* Lit) {
    Build_Str_List_Push(Ctx, List, Build_Str_From_Cstr(Lit));
}

void Build_Str_List_Push_Cstrs(build_context* Ctx, build_str_list* List, const char* const* Items, int Count) {
    for (int Index = 0; Index < Count; Index++) {
        Build_Str_List_Push_Lit(Ctx, List, Items[Index]);
    }
}

void Build_Str_List_PushV(build_context* Ctx, build_str_list* List, ...) {
    va_list Args;
    va_start(Args, List);
    for (;;) {
        build_string Item = va_arg(Args, build_string);
        if (!Item.Ptr) break;
        Build_Str_List_Push(Ctx, List, Item);
    }
    va_end(Args);
}

void Build_Str_List_Push_Lits(build_context* Ctx, build_str_list* List, ...) {
    va_list Args;
    va_start(Args, List);
    for (;;) {
        const char* Lit = va_arg(Args, const char*);
        if (!Lit) break;
        Build_Str_List_Push_Lit(Ctx, List, Lit);
    }
    va_end(Args);
}

void Build_Str_List_Push_Fmt(build_context* Ctx, build_str_list* List, const char* Fmt, ...) {
    va_list Args;
    va_start(Args, Fmt);
    char Tmp[4096];
    vsnprintf(Tmp, sizeof(Tmp), Fmt, Args);
    va_end(Args);
    Build_Str_List_Push(Ctx, List, Build__Arena_Str(Ctx, Tmp, strlen(Tmp)));
}

build_string Build_Fmt(build_context* Ctx, const char* Fmt, ...) {
    va_list Args;
    va_start(Args, Fmt);
    char Tmp[8192];
    vsnprintf(Tmp, sizeof(Tmp), Fmt, Args);
    va_end(Args);
    size_t N = strlen(Tmp);
    Build__Normalize_Slashes(Tmp, N);
    return Build__Arena_Str(Ctx, Tmp, N);
}

build_string Build_Path_Join(build_context* Ctx, build_string A, build_string B) {
#ifdef _WIN32
    return Build_Fmt(Ctx, "%.*s\\%.*s", (int)A.Size, A.Ptr, (int)B.Size, B.Ptr);
#else
    return Build_Fmt(Ctx, "%.*s/%.*s", (int)A.Size, A.Ptr, (int)B.Size, B.Ptr);
#endif
}

void Build_Log(const char* Fmt, ...) {
    va_list Args;
    va_start(Args, Fmt);
    printf("[build] ");
    vprintf(Fmt, Args);
    printf("\n");
    va_end(Args);
    fflush(stdout);
}

void Build_Log_Error(build_context* Ctx, const char* Fmt, ...) {
    va_list Args;
    va_start(Args, Fmt);
    fprintf(stderr, "[build] ERROR: ");
    vfprintf(stderr, Fmt, Args);
    fprintf(stderr, "\n");
    va_end(Args);
    if (Ctx) Ctx->Failed = 1;
}

void Build_Die(build_context* Ctx, const char* Fmt, ...) {
    va_list Args;
    va_start(Args, Fmt);
    fprintf(stderr, "[build] FATAL: ");
    vfprintf(stderr, Fmt, Args);
    fprintf(stderr, "\n");
    va_end(Args);
    if (Ctx) Ctx->Failed = 1;
    exit(1);
}

static void Build__Usage(void) {
    fprintf(stderr,
            "Common build options:\n"
            "  -debug              Debug build\n"
            "  -force              Ignore mtimes / stamps\n"
            "  -asan / -tsan / -ubsan\n"
            "  -bin_path <dir>     Output directory (default: <root>/bin)\n"
            "  -h, --help          Show this help\n"
            "Recipe-specific flags are parsed by each build.c.\n");
}

build_context Build_Init(int ArgCount, char** Args) {
    build_context Ctx = {0};
    Ctx.ArenaReserved = BUILD_ARENA_RESERVE_BYTES;
    Ctx.Arena = (char*)Build__Mem_Reserve(Ctx.ArenaReserved);
    if (!Ctx.Arena) {
        fprintf(stderr, "[build] virtual reserve failed (%zu bytes)\n", Ctx.ArenaReserved);
        exit(1);
    }
    Ctx.ArgCount = ArgCount;
    Ctx.Args = Args;

    build_string Cwd = Build__Get_Cwd(&Ctx);
    build_string ExeDir = Build__Get_Executable_Dir(&Ctx);
    Ctx.Root = Cwd;
    Ctx.RepoRoot = Cwd;

    if (ArgCount > 0 && Args[0]) {
        build_string Arg0 = Build__Arena_Str(&Ctx, Args[0], strlen(Args[0]));
        Build__Normalize_Slashes((char*)Arg0.Ptr, Arg0.Size);
        /* strip filename */
        build_string Dir = Arg0;
        for (size_t Index = Dir.Size; Index > 0; Index--) {
            if (Dir.Ptr[Index - 1] == '\\' || Dir.Ptr[Index - 1] == '/') {
                Dir.Size = Index - 1;
                ((char*)Dir.Ptr)[Dir.Size] = 0;
                break;
            }
        }
        if (Build_Str_Ends_With(Dir, Build_Str_Lit("bin"))) {
            build_string Parent = Dir;
            for (size_t Index = Parent.Size; Index > 0; Index--) {
                if (Parent.Ptr[Index - 1] == '\\' || Parent.Ptr[Index - 1] == '/') {
                    Parent.Size = Index - 1;
                    ((char*)Parent.Ptr)[Parent.Size] = 0;
                    Ctx.Root = Parent;
                    Ctx.RepoRoot = Parent;
                    break;
                }
            }
        } else if (Build_Is_Dir(Dir)) {
            Ctx.Root = Dir;
            Ctx.RepoRoot = Dir;
        }
    }

    if (Build_Str_Ends_With(ExeDir, Build_Str_Lit("bin"))) {
        build_string Parent = ExeDir;
        for (size_t Index = Parent.Size; Index > 0; Index--) {
            if (Parent.Ptr[Index - 1] == '\\' || Parent.Ptr[Index - 1] == '/') {
                Parent.Size = Index - 1;
                ((char*)Parent.Ptr)[Parent.Size] = 0;
                Ctx.Root = Parent;
                Ctx.RepoRoot = Parent;
                break;
            }
        }
    }

    Ctx.BinPath = Build_Path_Join(&Ctx, Ctx.Root, Build_Str_Lit("bin"));

    for (int ArgIndex = 1; ArgIndex < ArgCount; ArgIndex++) {
        const char* Arg = Args[ArgIndex];
        if (strcmp(Arg, "-debug") == 0) {
            Ctx.Debug = 1;
        } else if (strcmp(Arg, "-force") == 0) {
            Ctx.Force = 1;
        } else if (strcmp(Arg, "-asan") == 0) {
            Ctx.Asan = 1;
        } else if (strcmp(Arg, "-tsan") == 0) {
            Ctx.Tsan = 1;
        } else if (strcmp(Arg, "-ubsan") == 0) {
            Ctx.Ubsan = 1;
        } else if (strcmp(Arg, "-bin_path") == 0) {
            if (ArgIndex + 1 >= ArgCount) {
                Build__Usage();
                Build_Die(&Ctx, "-bin_path requires a directory");
            }
            Ctx.BinPath = Build__Arena_Str(&Ctx, Args[++ArgIndex], strlen(Args[ArgIndex]));
            Build__Normalize_Slashes((char*)Ctx.BinPath.Ptr, Ctx.BinPath.Size);
        } else if (strcmp(Arg, "-h") == 0 || strcmp(Arg, "--help") == 0) {
            Build__Usage();
            exit(0);
        } else {
            Build_Str_List_Push(&Ctx, &Ctx.UserArgs, Build_Str_From_Cstr(Arg));
        }
    }

    if (Ctx.Asan && Ctx.Tsan) {
        Build_Die(&Ctx, "address_sanitizer and thread_sanitizer cannot both be enabled");
    }
    if (Ctx.Tsan && Build_Os() == BUILD_OS_WIN32) {
        Build_Die(&Ctx, "thread_sanitizer (-tsan) is not supported on Windows");
    }

    Build_Mkdir_All(Ctx.BinPath);
    Build_Log("root=%.*s bin=%.*s", (int)Ctx.Root.Size, Ctx.Root.Ptr, (int)Ctx.BinPath.Size, Ctx.BinPath.Ptr);
    return Ctx;
}

void Build_Quit(build_context* Ctx) {
    Build__Mem_Release(Ctx->Arena, Ctx->ArenaReserved);
    memset(Ctx, 0, sizeof(*Ctx));
}

static const char* Build__Path_Cstr_Stack(build_string Path, char* Stack, size_t StackSize) {
    if (Build_Str_Empty(Path)) return "";
    if (Path.Ptr[Path.Size] == 0) return Path.Ptr;
    if (Path.Size + 1 > StackSize) return NULL;
    memcpy(Stack, Path.Ptr, Path.Size);
    Stack[Path.Size] = 0;
    return Stack;
}

build_b32 Build_Path_Exists(build_string Path) {
    char Stack[4096];
    const char* C = Build__Path_Cstr_Stack(Path, Stack, sizeof(Stack));
    if (!C) return 0;
#ifdef _WIN32
    return GetFileAttributesA(C) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat St;
    return stat(C, &St) == 0;
#endif
}

build_b32 Build_Is_File(build_string Path) {
    char Stack[4096];
    const char* C = Build__Path_Cstr_Stack(Path, Stack, sizeof(Stack));
    if (!C) return 0;
#ifdef _WIN32
    DWORD Attr = GetFileAttributesA(C);
    return Attr != INVALID_FILE_ATTRIBUTES && !(Attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat St;
    return stat(C, &St) == 0 && S_ISREG(St.st_mode);
#endif
}

build_b32 Build_Is_Dir(build_string Path) {
    char Stack[4096];
    const char* C = Build__Path_Cstr_Stack(Path, Stack, sizeof(Stack));
    if (!C) return 0;
#ifdef _WIN32
    DWORD Attr = GetFileAttributesA(C);
    return Attr != INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat St;
    return stat(C, &St) == 0 && S_ISDIR(St.st_mode);
#endif
}

build_b32 Build_Mkdir_All(build_string Path) {
    if (Build_Str_Empty(Path)) return 0;
    if (Build_Is_Dir(Path)) return 1;
    char Tmp[4096];
    if (Path.Size >= sizeof(Tmp)) return 0;
    memcpy(Tmp, Path.Ptr, Path.Size);
    Tmp[Path.Size] = 0;
    for (size_t Index = 0; Index < Path.Size; Index++) {
        if (Tmp[Index] == '/' || Tmp[Index] == '\\') {
            if (Index == 0) continue;
#ifdef _WIN32
            if (Index == 2 && Tmp[1] == ':') continue;
#endif
            char Save = Tmp[Index];
            Tmp[Index] = 0;
            if (!Build_Is_Dir(Build_Str_From_Cstr(Tmp))) {
#ifdef _WIN32
                if (_mkdir(Tmp) != 0 && !Build_Is_Dir(Build_Str_From_Cstr(Tmp))) return 0;
#else
                if (mkdir(Tmp, 0755) != 0 && !Build_Is_Dir(Build_Str_From_Cstr(Tmp))) return 0;
#endif
            }
            Tmp[Index] = Save;
        }
    }
#ifdef _WIN32
    if (_mkdir(Tmp) != 0 && !Build_Is_Dir(Build_Str_From_Cstr(Tmp))) return 0;
#else
    if (mkdir(Tmp, 0755) != 0 && !Build_Is_Dir(Build_Str_From_Cstr(Tmp))) return 0;
#endif
    return 1;
}

build_b32 Build_Copy_File(build_string Src, build_string Dst) {
    char SrcStack[4096], DstStack[4096];
    const char* S = Build__Path_Cstr_Stack(Src, SrcStack, sizeof(SrcStack));
    const char* D = Build__Path_Cstr_Stack(Dst, DstStack, sizeof(DstStack));
    if (!S || !D) return 0;
#ifdef _WIN32
    return CopyFileA(S, D, FALSE) != 0;
#else
    FILE* In = fopen(S, "rb");
    if (!In) return 0;
    FILE* Out = fopen(D, "wb");
    if (!Out) { fclose(In); return 0; }
    char Buf[64 * 1024];
    size_t N;
    while ((N = fread(Buf, 1, sizeof(Buf), In)) > 0) {
        if (fwrite(Buf, 1, N, Out) != N) { fclose(In); fclose(Out); return 0; }
    }
    fclose(In);
    fclose(Out);
    return 1;
#endif
}

build_b32 Build_Remove_File(build_string Path) {
    char Stack[4096];
    const char* C = Build__Path_Cstr_Stack(Path, Stack, sizeof(Stack));
    if (!C) return 0;
#ifdef _WIN32
    return DeleteFileA(C) != 0 || GetLastError() == ERROR_FILE_NOT_FOUND;
#else
    return unlink(C) == 0 || errno == ENOENT;
#endif
}

uint64_t Build_File_Mtime(build_string Path) {
    char Stack[4096];
    const char* C = Build__Path_Cstr_Stack(Path, Stack, sizeof(Stack));
    if (!C) return 0;
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (!GetFileAttributesExA(C, GetFileExInfoStandard, &Data)) return 0;
    ULARGE_INTEGER U;
    U.LowPart = Data.ftLastWriteTime.dwLowDateTime;
    U.HighPart = Data.ftLastWriteTime.dwHighDateTime;
    return U.QuadPart;
#else
    struct stat St;
    if (stat(C, &St) != 0) return 0;
    return (uint64_t)St.st_mtime;
#endif
}

build_b32 Build_Needs_Rebuild(build_context* Ctx, build_string Output, const build_string* Inputs, int InputCount) {
    if (Ctx && Ctx->Force) return 1;
    uint64_t OutTime = Build_File_Mtime(Output);
    if (OutTime == 0) return 1;
    for (int InputIndex = 0; InputIndex < InputCount; InputIndex++) {
        uint64_t InTime = Build_File_Mtime(Inputs[InputIndex]);
        if (InTime == 0 || InTime > OutTime) return 1;
    }
    return 0;
}

build_b32 Build_Needs_Rebuild_List(build_context* Ctx, build_string Output, build_str_list* Inputs) {
    return Build_Needs_Rebuild(Ctx, Output, Inputs->Items, Inputs->Count);
}

void Build_Cmd_Reset(build_cmd* Cmd) { Cmd->Args.Count = 0; }

void Build_Cmd_Push(build_context* Ctx, build_cmd* Cmd, build_string Arg) {
    Build_Str_List_Push(Ctx, &Cmd->Args, Arg);
}

void Build_Cmd_Push_Lit(build_context* Ctx, build_cmd* Cmd, const char* Lit) {
    Build_Cmd_Push(Ctx, Cmd, Build_Str_From_Cstr(Lit));
}

void Build_Cmd_PushV(build_context* Ctx, build_cmd* Cmd, ...) {
    va_list Args;
    va_start(Args, Cmd);
    for (;;) {
        build_string Arg = va_arg(Args, build_string);
        if (!Arg.Ptr) break;
        Build_Cmd_Push(Ctx, Cmd, Arg);
    }
    va_end(Args);
}

void Build_Cmd_Push_Lits(build_context* Ctx, build_cmd* Cmd, ...) {
    va_list Args;
    va_start(Args, Cmd);
    for (;;) {
        const char* Lit = va_arg(Args, const char*);
        if (!Lit) break;
        Build_Cmd_Push_Lit(Ctx, Cmd, Lit);
    }
    va_end(Args);
}

void Build_Cmd_Push_Fmt(build_context* Ctx, build_cmd* Cmd, const char* Fmt, ...) {
    va_list Args;
    va_start(Args, Fmt);
    char Tmp[4096];
    vsnprintf(Tmp, sizeof(Tmp), Fmt, Args);
    va_end(Args);
    Build_Cmd_Push(Ctx, Cmd, Build__Arena_Str(Ctx, Tmp, strlen(Tmp)));
}

#ifdef _WIN32
static char* Build__Quote_Win_Arg(build_context* Ctx, build_string Arg) {
    build_b32 NeedQuotes = 0;
    for (size_t Index = 0; Index < Arg.Size; Index++) {
        char Ch = Arg.Ptr[Index];
        if (Ch == ' ' || Ch == '\t' || Ch == '"') { NeedQuotes = 1; break; }
    }
    if (!NeedQuotes) return (char*)Build_Cstr(Ctx, Arg);
    char* Out = Build__Arena_Push(Ctx, Arg.Size * 2 + 3);
    size_t O = 0;
    Out[O++] = '"';
    for (size_t Index = 0; Index < Arg.Size; Index++) {
        if (Arg.Ptr[Index] == '"') { Out[O++] = '\\'; Out[O++] = '"'; }
        else Out[O++] = Arg.Ptr[Index];
    }
    Out[O++] = '"';
    Out[O] = 0;
    return Out;
}

static char* Build__Join_Cmd_Line(build_context* Ctx, build_cmd* Cmd) {
    size_t Cap = 1;
    for (int ArgIndex = 0; ArgIndex < Cmd->Args.Count; ArgIndex++) {
        Cap += Cmd->Args.Items[ArgIndex].Size * 2 + 4;
    }
    char* Out = Build__Arena_Push(Ctx, Cap);
    size_t O = 0;
    for (int ArgIndex = 0; ArgIndex < Cmd->Args.Count; ArgIndex++) {
        if (ArgIndex) Out[O++] = ' ';
        char* Q = Build__Quote_Win_Arg(Ctx, Cmd->Args.Items[ArgIndex]);
        size_t N = strlen(Q);
        memcpy(Out + O, Q, N);
        O += N;
    }
    Out[O] = 0;
    return Out;
}
#endif

static void Build__Log_Cmd(build_cmd* Cmd) {
    printf("[build] >");
    for (int ArgIndex = 0; ArgIndex < Cmd->Args.Count; ArgIndex++) {
        printf(" %.*s", (int)Cmd->Args.Items[ArgIndex].Size, Cmd->Args.Items[ArgIndex].Ptr);
    }
    printf("\n");
    fflush(stdout);
}

int Build_Cmd_Run_Cwd(build_context* Ctx, build_cmd* Cmd, build_string Cwd) {
    if (Cmd->Args.Count == 0) {
        Build_Log_Error(Ctx, "empty command");
        return 1;
    }
    Build__Log_Cmd(Cmd);
#ifdef _WIN32
    char* CmdLine = Build__Join_Cmd_Line(Ctx, Cmd);
    STARTUPINFOA Si;
    PROCESS_INFORMATION Pi;
    ZeroMemory(&Si, sizeof(Si));
    Si.cb = sizeof(Si);
    ZeroMemory(&Pi, sizeof(Pi));
    char* Mutable = (char*)Build__Arena_Str(Ctx, CmdLine, strlen(CmdLine)).Ptr;
    const char* CwdC = Build_Str_Empty(Cwd) ? NULL : Build_Cstr(Ctx, Cwd);
    BOOL Ok = CreateProcessA(NULL, Mutable, NULL, NULL, TRUE, 0, NULL, CwdC, &Si, &Pi);
    if (!Ok) {
        Build_Log_Error(Ctx, "CreateProcess failed (%lu): %s", GetLastError(), CmdLine);
        return 1;
    }
    WaitForSingleObject(Pi.hProcess, INFINITE);
    DWORD Code = 1;
    GetExitCodeProcess(Pi.hProcess, &Code);
    CloseHandle(Pi.hProcess);
    CloseHandle(Pi.hThread);
    return (int)Code;
#else
    pid_t Pid = fork();
    if (Pid < 0) {
        Build_Log_Error(Ctx, "fork failed: %s", strerror(errno));
        return 1;
    }
    if (Pid == 0) {
        if (!Build_Str_Empty(Cwd) && chdir(Build_Cstr(Ctx, Cwd)) != 0) {
            fprintf(stderr, "[build] chdir failed: %s\n", strerror(errno));
            _exit(127);
        }
        size_t N = (size_t)Cmd->Args.Count;
        char** Argv = (char**)Build__Arena_Push(Ctx, sizeof(char*) * (N + 1));
        for (size_t ArgIndex = 0; ArgIndex < N; ArgIndex++) {
            Argv[ArgIndex] = (char*)Build_Cstr(Ctx, Cmd->Args.Items[ArgIndex]);
        }
        Argv[N] = NULL;
        execvp(Argv[0], Argv);
        fprintf(stderr, "[build] execvp(%s) failed: %s\n", Argv[0], strerror(errno));
        _exit(127);
    }
    int Status = 0;
    if (waitpid(Pid, &Status, 0) < 0) {
        Build_Log_Error(Ctx, "waitpid failed: %s", strerror(errno));
        return 1;
    }
    if (WIFEXITED(Status)) return WEXITSTATUS(Status);
    return 1;
#endif
}

int Build_Cmd_Run(build_context* Ctx, build_cmd* Cmd) {
    return Build_Cmd_Run_Cwd(Ctx, Cmd, BUILD_STR_END);
}

void Build_Cmd_Run_Or_Die(build_context* Ctx, build_cmd* Cmd) {
    int Code = Build_Cmd_Run(Ctx, Cmd);
    if (Code != 0) Build_Die(Ctx, "command failed with exit code %d", Code);
}

void Build_Cmd_Run_Cwd_Or_Die(build_context* Ctx, build_cmd* Cmd, build_string Cwd) {
    int Code = Build_Cmd_Run_Cwd(Ctx, Cmd, Cwd);
    if (Code != 0) Build_Die(Ctx, "command failed with exit code %d", Code);
}

build_toolchain Build_Toolchain_MSVC(void) {
    build_toolchain Tc;
    memset(&Tc, 0, sizeof(Tc));
    Tc.Compiler = BUILD_COMPILER_MSVC;
    Tc.CC = Build_Str_Lit("cl");
    Tc.CXX = Build_Str_Lit("cl");
    Tc.AR = Build_Str_Lit("lib");
    Tc.ObjExt = Build_Str_Lit(".obj");
    Tc.LibExt = Build_Str_Lit(".lib");
    Tc.LibPrefix = BUILD_STR_END;
    Tc.DllExt = Build_Str_Lit(".dll");
    Tc.ExeExt = Build_Str_Lit(".exe");
    return Tc;
}

build_toolchain Build_Toolchain_Clang(void) {
    build_toolchain Tc;
    memset(&Tc, 0, sizeof(Tc));
    Tc.Compiler = BUILD_COMPILER_CLANG;
    Tc.CC = Build_Str_Lit("clang");
    Tc.CXX = Build_Str_Lit("clang++");
#ifdef _WIN32
    Tc.AR = Build_Str_Lit("lib");
    Tc.ObjExt = Build_Str_Lit(".obj");
    Tc.LibExt = Build_Str_Lit(".lib");
    Tc.LibPrefix = BUILD_STR_END;
    Tc.DllExt = Build_Str_Lit(".dll");
    Tc.ExeExt = Build_Str_Lit(".exe");
#else
    Tc.AR = Build_Str_Lit("ar");
    Tc.ObjExt = Build_Str_Lit(".o");
    Tc.LibExt = Build_Str_Lit(".a");
    Tc.LibPrefix = Build_Str_Lit("lib");
#if defined(__APPLE__)
    Tc.DllExt = Build_Str_Lit(".dylib");
#else
    Tc.DllExt = Build_Str_Lit(".so");
#endif
    Tc.ExeExt = BUILD_STR_END;
#endif
    return Tc;
}

build_toolchain Build_Toolchain_Gcc(void) {
    build_toolchain Tc = Build_Toolchain_Clang();
    Tc.Compiler = BUILD_COMPILER_GCC;
    Tc.CC = Build_Str_Lit("gcc");
    Tc.CXX = Build_Str_Lit("g++");
    return Tc;
}

static build_b32 Build__Command_Exists(const char* Name) {
#ifdef _WIN32
    char Buf[1024];
    snprintf(Buf, sizeof(Buf), "where %s >nul 2>&1", Name);
    return system(Buf) == 0;
#else
    char Buf[1024];
    snprintf(Buf, sizeof(Buf), "command -v %s >/dev/null 2>&1", Name);
    return system(Buf) == 0;
#endif
}

build_toolchain Build_Toolchain_Detect(build_context* Ctx, build_b32 PreferClang) {
    (void)Ctx;
#ifdef _WIN32
    if (PreferClang) return Build_Toolchain_Clang();
    if (Build__Command_Exists("cl")) return Build_Toolchain_MSVC();
    return Build_Toolchain_Clang();
#else
    (void)PreferClang;
    if (Build__Command_Exists("clang++")) return Build_Toolchain_Clang();
    if (Build__Command_Exists("g++")) return Build_Toolchain_Gcc();
    return Build_Toolchain_Clang();
#endif
}

build_string Build_Obj_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem) {
    return Build_Fmt(Ctx, "%.*s/%.*s%.*s", (int)Dir.Size, Dir.Ptr, (int)Stem.Size, Stem.Ptr,
                     (int)Tc->ObjExt.Size, Tc->ObjExt.Ptr);
}

build_string Build_Lib_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem) {
    return Build_Fmt(Ctx, "%.*s/%.*s%.*s%.*s", (int)Dir.Size, Dir.Ptr,
                     (int)Tc->LibPrefix.Size, Tc->LibPrefix.Ptr ? Tc->LibPrefix.Ptr : "",
                     (int)Stem.Size, Stem.Ptr, (int)Tc->LibExt.Size, Tc->LibExt.Ptr);
}

build_string Build_Exe_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem) {
    return Build_Fmt(Ctx, "%.*s/%.*s%.*s", (int)Dir.Size, Dir.Ptr, (int)Stem.Size, Stem.Ptr,
                     (int)Tc->ExeExt.Size, Tc->ExeExt.Ptr ? Tc->ExeExt.Ptr : "");
}

build_string Build_Dll_Path(build_context* Ctx, const build_toolchain* Tc, build_string Dir, build_string Stem) {
#if !defined(_WIN32)
    if (Tc->Compiler != BUILD_COMPILER_MSVC) {
        return Build_Fmt(Ctx, "%.*s/lib%.*s%.*s", (int)Dir.Size, Dir.Ptr, (int)Stem.Size, Stem.Ptr,
                         (int)Tc->DllExt.Size, Tc->DllExt.Ptr);
    }
#endif
    return Build_Fmt(Ctx, "%.*s/%.*s%.*s", (int)Dir.Size, Dir.Ptr, (int)Stem.Size, Stem.Ptr,
                     (int)Tc->DllExt.Size, Tc->DllExt.Ptr);
}

void Build_Compile_Opts_Apply_Defaults(build_compile_opts* Opts) {
    if (Opts->Standard == 0) Opts->Standard = Opts->Cpp ? 20 : 11;
    /* -fPIC is meaningless / rejected on Windows clang-msvc targets. */
    if (Opts->Toolchain.Compiler != BUILD_COMPILER_MSVC && Build_Os() != BUILD_OS_WIN32) Opts->PIC = 1;
}

static build_string Build__Driver(build_compile_opts* Opts) {
    if (Opts->ObjC) {
#ifdef __APPLE__
        return Opts->Cpp ? Build_Str_Lit("clang++") : Build_Str_Lit("clang");
#else
        return Opts->Cpp ? Opts->Toolchain.CXX : Opts->Toolchain.CC;
#endif
    }
    return Opts->Cpp ? Opts->Toolchain.CXX : Opts->Toolchain.CC;
}

static void Build__Push_Compile_Flags(build_context* Ctx, build_cmd* Cmd, build_compile_opts* Opts) {
    build_toolchain* Tc = &Opts->Toolchain;
    if (Tc->Compiler == BUILD_COMPILER_MSVC) {
        Build_Cmd_PushV(Ctx, Cmd, Build_Str_Lit("/nologo"), Build_Str_Lit("/FC"), Build_Str_Lit("/Z7"), Build_Str_Lit("/c"), BUILD_STR_END);
        Build_Cmd_Push_Lit(Ctx, Cmd, Opts->Debug ? "/Od" : "/O2");
        Build_Cmd_PushV(Ctx, Cmd, Build_Str_Lit("/GF"), Build_Str_Lit("/experimental:c11atomics"), BUILD_STR_END);
        if (Opts->Cpp) {
            Build_Cmd_Push_Fmt(Ctx, Cmd, "/std:c++%d", Opts->Standard);
            if (Opts->NoRTTI) Build_Cmd_Push_Lit(Ctx, Cmd, "/GR-");
        } else {
            Build_Cmd_Push_Fmt(Ctx, Cmd, "/std:c%d", Opts->Standard);
        }
        if (Opts->MsvcWall) {
            Build_Cmd_PushV(Ctx, Cmd,
                Build_Str_Lit("/Wall"), Build_Str_Lit("/WX"),
                Build_Str_Lit("/wd4061"), Build_Str_Lit("/wd4062"), Build_Str_Lit("/wd4063"),
                Build_Str_Lit("/wd4100"), Build_Str_Lit("/wd4127"), Build_Str_Lit("/wd4189"),
                Build_Str_Lit("/wd4191"), Build_Str_Lit("/wd4201"), Build_Str_Lit("/wd4255"),
                Build_Str_Lit("/wd4324"), Build_Str_Lit("/wd4355"), Build_Str_Lit("/wd4365"),
                Build_Str_Lit("/wd4456"), Build_Str_Lit("/wd4464"), Build_Str_Lit("/wd4505"),
                Build_Str_Lit("/wd4530"), Build_Str_Lit("/wd4577"), Build_Str_Lit("/wd4625"),
                Build_Str_Lit("/wd4626"), Build_Str_Lit("/wd4668"), Build_Str_Lit("/wd4710"),
                Build_Str_Lit("/wd4711"), Build_Str_Lit("/wd4820"), Build_Str_Lit("/wd4996"),
                Build_Str_Lit("/wd5204"), Build_Str_Lit("/wd5026"), Build_Str_Lit("/wd5027"),
                Build_Str_Lit("/wd5045"), Build_Str_Lit("/wd5246"), Build_Str_Lit("/wd5250"),
                Build_Str_Lit("/wd5262"), BUILD_STR_END);
        }
        if (Opts->Asan) Build_Cmd_Push_Lit(Ctx, Cmd, "/fsanitize=address");
    } else {
        Build_Cmd_PushV(Ctx, Cmd, Build_Str_Lit("-c"), Build_Str_Lit("-g"), BUILD_STR_END);
        Build_Cmd_Push_Lit(Ctx, Cmd, Opts->Debug ? "-O0" : "-O3");
        if (Opts->PIC) Build_Cmd_Push_Lit(Ctx, Cmd, "-fPIC");
        if (Opts->Cpp) {
            Build_Cmd_Push_Fmt(Ctx, Cmd, "-std=c++%d", Opts->Standard);
            if (Opts->NoRTTI) Build_Cmd_Push_Lit(Ctx, Cmd, "-fno-rtti");
            if (Opts->NoExceptions) Build_Cmd_Push_Lit(Ctx, Cmd, "-fno-exceptions");
        } else if (!Opts->ObjC) {
            Build_Cmd_Push_Fmt(Ctx, Cmd, "-std=c%d", Opts->Standard);
        }
        if (Opts->WarningsAsErrors || Opts->MsvcWall) {
            Build_Cmd_PushV(Ctx, Cmd,
                Build_Str_Lit("-Wall"), Build_Str_Lit("-Werror"),
                Build_Str_Lit("-Wno-missing-braces"), Build_Str_Lit("-Wno-switch"),
                Build_Str_Lit("-Wno-unused-function"), Build_Str_Lit("-Wno-unused-const-variable"),
                Build_Str_Lit("-Wno-nullability-completeness"), Build_Str_Lit("-Wno-unused-variable"),
                Build_Str_Lit("-Wno-undefined-internal"), BUILD_STR_END);
#ifdef _WIN32
            Build_Cmd_PushV(Ctx, Cmd, Build_Str_Lit("-Wno-microsoft-cast"), Build_Str_Lit("-gcodeview"), BUILD_STR_END);
#endif
        }
        if (Opts->Asan) Build_Cmd_Push_Lit(Ctx, Cmd, "-fsanitize=address");
        if (Opts->Tsan) Build_Cmd_Push_Lit(Ctx, Cmd, "-fsanitize=thread");
        if (Opts->Ubsan) Build_Cmd_Push_Lit(Ctx, Cmd, "-fsanitize=undefined");
        if (Build_Os() == BUILD_OS_LINUX) {
            Build_Cmd_PushV(Ctx, Cmd, Build_Str_Lit("-D_GNU_SOURCE=1"), Build_Str_Lit("-gdwarf-4"), BUILD_STR_END);
        }
    }
    for (int IncludeIndex = 0; IncludeIndex < Opts->Includes.Count; IncludeIndex++) {
        Build_Cmd_Push_Fmt(Ctx, Cmd, "-I%.*s", (int)Opts->Includes.Items[IncludeIndex].Size,
                           Opts->Includes.Items[IncludeIndex].Ptr);
    }
    for (int DefineIndex = 0; DefineIndex < Opts->Defines.Count; DefineIndex++) {
        Build_Cmd_Push_Fmt(Ctx, Cmd, "-D%.*s", (int)Opts->Defines.Items[DefineIndex].Size,
                           Opts->Defines.Items[DefineIndex].Ptr);
    }
    for (int FlagIndex = 0; FlagIndex < Opts->ExtraFlags.Count; FlagIndex++) {
        Build_Cmd_Push(Ctx, Cmd, Opts->ExtraFlags.Items[FlagIndex]);
    }
}

build_b32 Build_Compile(build_context* Ctx, build_compile_opts* Opts, build_string SrcPath, build_string ObjPath) {
    build_string Inputs[] = {SrcPath};
    if (!Build_Needs_Rebuild(Ctx, ObjPath, Inputs, 1)) {
        Build_Log("skip compile %.*s", (int)ObjPath.Size, ObjPath.Ptr);
        return 1;
    }
    build_string ObjDir = Build__Arena_Str(Ctx, ObjPath.Ptr, ObjPath.Size);
    for (size_t Index = ObjDir.Size; Index > 0; Index--) {
        if (ObjDir.Ptr[Index - 1] == '\\' || ObjDir.Ptr[Index - 1] == '/') {
            ((char*)ObjDir.Ptr)[Index - 1] = 0;
            ObjDir.Size = Index - 1;
            Build_Mkdir_All(ObjDir);
            break;
        }
    }
    build_cmd Cmd = {0};
    Build_Cmd_Push(Ctx, &Cmd, Build__Driver(Opts));
    Build__Push_Compile_Flags(Ctx, &Cmd, Opts);
    if (Opts->Toolchain.Compiler == BUILD_COMPILER_MSVC) {
        Build_Cmd_Push_Fmt(Ctx, &Cmd, "-Fo%.*s", (int)ObjPath.Size, ObjPath.Ptr);
        Build_Cmd_Push(Ctx, &Cmd, SrcPath);
    } else {
        if (Opts->ObjC && !Opts->Cpp) {
            Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-x"), Build_Str_Lit("objective-c"), BUILD_STR_END);
        }
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-o"), ObjPath, SrcPath, BUILD_STR_END);
    }
    int Code = Build_Cmd_Run(Ctx, &Cmd);
    if (Code != 0) {
        Build_Log_Error(Ctx, "compile failed: %.*s", (int)SrcPath.Size, SrcPath.Ptr);
        return 0;
    }
    return 1;
}

build_b32 Build_Compile_Many(build_context* Ctx, build_compile_opts* Opts, build_str_list* Sources, build_string ObjDir, build_str_list* OutObjs) {
    Build_Mkdir_All(ObjDir);
    Build_Compile_Opts_Apply_Defaults(Opts);

    build_str_list DirtySrcs = {0};
    build_str_list DirtyObjs = {0};
    for (int SourceIndex = 0; SourceIndex < Sources->Count; SourceIndex++) {
        build_string Src = Sources->Items[SourceIndex];
        build_string Stem = Build__Stem(Ctx, Src);
        build_string Obj = Build_Obj_Path(Ctx, &Opts->Toolchain, ObjDir, Stem);
        Build_Str_List_Push(Ctx, OutObjs, Obj);
        build_string Inputs[] = {Src};
        if (Build_Needs_Rebuild(Ctx, Obj, Inputs, 1)) {
            Build_Str_List_Push(Ctx, &DirtySrcs, Src);
            Build_Str_List_Push(Ctx, &DirtyObjs, Obj);
        } else {
            Build_Log("skip compile %.*s", (int)Obj.Size, Obj.Ptr);
        }
    }
    if (DirtySrcs.Count == 0) return 1;

    /* One cl/clang++ invocation for all dirty sources when the driver supports it.
       MSVC: /FoDir\ writes basename.obj next to each source name.
       Clang/GCC: -c a.c b.c writes a.o/b.o in CWD — run with Cwd=ObjDir and pass
       absolute sources so outputs land as <stem>.o in ObjDir. */
    if (DirtySrcs.Count == 1) {
        return Build_Compile(Ctx, Opts, DirtySrcs.Items[0], DirtyObjs.Items[0]);
    }

    build_cmd Cmd = {0};
    Build_Cmd_Push(Ctx, &Cmd, Build__Driver(Opts));
    Build__Push_Compile_Flags(Ctx, &Cmd, Opts);
    if (Opts->Toolchain.Compiler == BUILD_COMPILER_MSVC) {
        /* Trailing slash => /Fo is an output directory (required for multi-source /c). */
        char FoDir[4096];
        if (ObjDir.Size + 2 >= sizeof(FoDir)) {
            Build_Log_Error(Ctx, "obj dir path too long for batched compile");
            return 0;
        }
        memcpy(FoDir, ObjDir.Ptr, ObjDir.Size);
        FoDir[ObjDir.Size] = '\\';
        FoDir[ObjDir.Size + 1] = 0;
        Build_Cmd_Push_Fmt(Ctx, &Cmd, "-Fo%s", FoDir);
        for (int Index = 0; Index < DirtySrcs.Count; Index++) Build_Cmd_Push(Ctx, &Cmd, DirtySrcs.Items[Index]);
        if (Build_Cmd_Run(Ctx, &Cmd) != 0) {
            Build_Log_Error(Ctx, "batched compile failed (%d sources)", DirtySrcs.Count);
            return 0;
        }
    } else {
        if (Opts->ObjC && !Opts->Cpp) {
            Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-x"), Build_Str_Lit("objective-c"), BUILD_STR_END);
        }
        for (int Index = 0; Index < DirtySrcs.Count; Index++) Build_Cmd_Push(Ctx, &Cmd, DirtySrcs.Items[Index]);
        /* clang/gcc place <stem>.o in CWD when -c has no per-file -o */
        if (Build_Cmd_Run_Cwd(Ctx, &Cmd, ObjDir) != 0) {
            Build_Log_Error(Ctx, "batched compile failed (%d sources)", DirtySrcs.Count);
            return 0;
        }
        /* Confirm expected objs exist (stem from source basename). */
        for (int Index = 0; Index < DirtyObjs.Count; Index++) {
            if (!Build_Is_File(DirtyObjs.Items[Index])) {
                Build_Log_Error(Ctx, "batched compile missing output: %.*s",
                                (int)DirtyObjs.Items[Index].Size, DirtyObjs.Items[Index].Ptr);
                return 0;
            }
        }
    }
    return 1;
}

build_b32 Build_Archive(build_context* Ctx, const build_toolchain* Tc, build_str_list* Objs, build_string LibPath) {
    if (!Build_Needs_Rebuild_List(Ctx, LibPath, Objs)) {
        Build_Log("skip archive %.*s", (int)LibPath.Size, LibPath.Ptr);
        return 1;
    }
    build_cmd Cmd = {0};
    if (Tc->Compiler == BUILD_COMPILER_MSVC || (Build_Os() == BUILD_OS_WIN32 && Build_Str_Eq_Lit(Tc->AR, "lib"))) {
        Build_Cmd_Push(Ctx, &Cmd, Tc->AR);
        Build_Cmd_Push_Lit(Ctx, &Cmd, "/nologo");
        Build_Cmd_Push_Fmt(Ctx, &Cmd, "/out:%.*s", (int)LibPath.Size, LibPath.Ptr);
        for (int ObjIndex = 0; ObjIndex < Objs->Count; ObjIndex++) Build_Cmd_Push(Ctx, &Cmd, Objs->Items[ObjIndex]);
    } else {
        Build_Remove_File(LibPath);
        Build_Cmd_PushV(Ctx, &Cmd, Tc->AR, Build_Str_Lit("rcs"), LibPath, BUILD_STR_END);
        for (int ObjIndex = 0; ObjIndex < Objs->Count; ObjIndex++) Build_Cmd_Push(Ctx, &Cmd, Objs->Items[ObjIndex]);
    }
    int Code = Build_Cmd_Run(Ctx, &Cmd);
    if (Code != 0) {
        Build_Log_Error(Ctx, "archive failed: %.*s", (int)LibPath.Size, LibPath.Ptr);
        return 0;
    }
    return 1;
}

build_b32 Build_Link(build_context* Ctx, build_link_opts* Opts) {
    if (!Ctx->Force && !Build_Needs_Rebuild_List(Ctx, Opts->OutPath, &Opts->Objects)) {
        build_b32 Needs = 0;
        uint64_t OutTime = Build_File_Mtime(Opts->OutPath);
        if (OutTime == 0) Needs = 1;
        for (int LibIndex = 0; LibIndex < Opts->Libs.Count && !Needs; LibIndex++) {
            build_string Lib = Opts->Libs.Items[LibIndex];
            if (Build_Is_File(Lib) && Build_File_Mtime(Lib) > OutTime) Needs = 1;
        }
        if (!Needs) {
            Build_Log("skip link %.*s", (int)Opts->OutPath.Size, Opts->OutPath.Ptr);
            return 1;
        }
    }
    build_toolchain* Tc = &Opts->Toolchain;
    build_string Driver = Opts->Cpp ? Tc->CXX : Tc->CC;
    build_cmd Cmd = {0};
    Build_Cmd_Push(Ctx, &Cmd, Driver);
    if (Tc->Compiler == BUILD_COMPILER_MSVC) {
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("/nologo"), Build_Str_Lit("/FC"), Build_Str_Lit("/Z7"), BUILD_STR_END);
        Build_Cmd_Push_Lit(Ctx, &Cmd, Opts->Debug ? "/Od" : "/O2");
        if (Opts->Shared) Build_Cmd_Push_Lit(Ctx, &Cmd, "/LD");
        for (int ObjIndex = 0; ObjIndex < Opts->Objects.Count; ObjIndex++) Build_Cmd_Push(Ctx, &Cmd, Opts->Objects.Items[ObjIndex]);
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("/link"), Build_Str_Lit("/opt:ref"), Build_Str_Lit("/incremental:no"), BUILD_STR_END);
        for (int DirIndex = 0; DirIndex < Opts->LibDirs.Count; DirIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "/LIBPATH:%.*s", (int)Opts->LibDirs.Items[DirIndex].Size, Opts->LibDirs.Items[DirIndex].Ptr);
        }
        for (int LibIndex = 0; LibIndex < Opts->Libs.Count; LibIndex++) Build_Cmd_Push(Ctx, &Cmd, Opts->Libs.Items[LibIndex]);
        for (int FlagIndex = 0; FlagIndex < Opts->ExtraFlags.Count; FlagIndex++) Build_Cmd_Push(Ctx, &Cmd, Opts->ExtraFlags.Items[FlagIndex]);
        if (!Build_Str_Empty(Opts->PdbPath)) Build_Cmd_Push_Fmt(Ctx, &Cmd, "/PDB:%.*s", (int)Opts->PdbPath.Size, Opts->PdbPath.Ptr);
        Build_Cmd_Push_Fmt(Ctx, &Cmd, "/out:%.*s", (int)Opts->OutPath.Size, Opts->OutPath.Ptr);
    } else {
        Build_Cmd_Push_Lit(Ctx, &Cmd, "-g");
        Build_Cmd_Push_Lit(Ctx, &Cmd, Opts->Debug ? "-O0" : "-O3");
        if (Opts->Shared) {
            Build_Cmd_Push_Lit(Ctx, &Cmd, "-shared");
            if (!Build_Str_Empty(Opts->InstallName)) {
                Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-install_name"), Opts->InstallName, BUILD_STR_END);
            }
        }
        for (int ObjIndex = 0; ObjIndex < Opts->Objects.Count; ObjIndex++) Build_Cmd_Push(Ctx, &Cmd, Opts->Objects.Items[ObjIndex]);
        for (int DirIndex = 0; DirIndex < Opts->LibDirs.Count; DirIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "-L%.*s", (int)Opts->LibDirs.Items[DirIndex].Size, Opts->LibDirs.Items[DirIndex].Ptr);
        }
        for (int LibIndex = 0; LibIndex < Opts->Libs.Count; LibIndex++) {
            build_string Lib = Opts->Libs.Items[LibIndex];
            if (Build_Is_File(Lib) || Build_Str_Ends_With(Lib, Build_Str_Lit("/")) ||
                (Lib.Size && (Lib.Ptr[0] == '/' || Lib.Ptr[0] == '-'))) {
                Build_Cmd_Push(Ctx, &Cmd, Lib);
            } else {
                /* clang-msvc + link.exe: -lfoo.lib resolves via -L as foo.lib */
                Build_Cmd_Push_Fmt(Ctx, &Cmd, "-l%.*s", (int)Lib.Size, Lib.Ptr);
            }
        }
        for (int FrameworkIndex = 0; FrameworkIndex < Opts->Frameworks.Count; FrameworkIndex++) {
            Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-framework"), Opts->Frameworks.Items[FrameworkIndex], BUILD_STR_END);
        }
        for (int FlagIndex = 0; FlagIndex < Opts->ExtraFlags.Count; FlagIndex++) Build_Cmd_Push(Ctx, &Cmd, Opts->ExtraFlags.Items[FlagIndex]);
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-o"), Opts->OutPath, BUILD_STR_END);
    }
    int Code = Build_Cmd_Run(Ctx, &Cmd);
    if (Code != 0) {
        Build_Log_Error(Ctx, "link failed: %.*s", (int)Opts->OutPath.Size, Opts->OutPath.Ptr);
        return 0;
    }
    return 1;
}

build_b32 Build_Compile_And_Link(build_context* Ctx, build_compile_opts* CompileOpts, build_link_opts* LinkOpts, build_string SrcPath) {
    build_str_list Inputs = {0};
    Build_Str_List_Push(Ctx, &Inputs, SrcPath);
    for (int LibIndex = 0; LibIndex < LinkOpts->Libs.Count; LibIndex++) Build_Str_List_Push(Ctx, &Inputs, LinkOpts->Libs.Items[LibIndex]);
    if (!Build_Needs_Rebuild_List(Ctx, LinkOpts->OutPath, &Inputs) && !Ctx->Force) {
        Build_Log("skip compile+link %.*s", (int)LinkOpts->OutPath.Size, LinkOpts->OutPath.Ptr);
        return 1;
    }

    /* Keep intermediates next to /out (not process CWD — often the repo root). */
    build_string OutDir = Build_Str_Lit(".");
    for (size_t Index = LinkOpts->OutPath.Size; Index > 0; Index--) {
        char Ch = LinkOpts->OutPath.Ptr[Index - 1];
        if (Ch == '\\' || Ch == '/') {
            OutDir = Build__Arena_Str(Ctx, LinkOpts->OutPath.Ptr, Index - 1);
            break;
        }
    }
    Build_Mkdir_All(OutDir);
    build_string Stem = Build__Stem(Ctx, SrcPath);
    build_toolchain* Tc = &CompileOpts->Toolchain;
    build_string ObjPath = Build_Obj_Path(Ctx, Tc, OutDir, Stem);
    build_string OutStem = Build__Stem(Ctx, LinkOpts->OutPath);
    build_string ImplibPath = Build_Lib_Path(Ctx, Tc, OutDir, OutStem);

    build_string Driver = CompileOpts->Cpp ? Tc->CXX : Tc->CC;
    build_cmd Cmd = {0};
    Build_Cmd_Push(Ctx, &Cmd, Driver);
    build_compile_opts Co = *CompileOpts;
    if (Tc->Compiler == BUILD_COMPILER_MSVC) {
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("/nologo"), Build_Str_Lit("/FC"), Build_Str_Lit("/Z7"), BUILD_STR_END);
        Build_Cmd_Push_Lit(Ctx, &Cmd, Co.Debug ? "/Od" : "/O2");
        if (Co.Cpp) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "/std:c++%d", Co.Standard ? Co.Standard : 20);
            if (Co.NoRTTI) Build_Cmd_Push_Lit(Ctx, &Cmd, "/GR-");
        }
        if (Co.Asan) Build_Cmd_Push_Lit(Ctx, &Cmd, "/fsanitize=address");
        for (int IncludeIndex = 0; IncludeIndex < Co.Includes.Count; IncludeIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "-I%.*s", (int)Co.Includes.Items[IncludeIndex].Size, Co.Includes.Items[IncludeIndex].Ptr);
        }
        for (int DefineIndex = 0; DefineIndex < Co.Defines.Count; DefineIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "-D%.*s", (int)Co.Defines.Items[DefineIndex].Size, Co.Defines.Items[DefineIndex].Ptr);
        }
        for (int FlagIndex = 0; FlagIndex < Co.ExtraFlags.Count; FlagIndex++) Build_Cmd_Push(Ctx, &Cmd, Co.ExtraFlags.Items[FlagIndex]);
        if (LinkOpts->Shared) Build_Cmd_Push_Lit(Ctx, &Cmd, "/LD");
        Build_Cmd_Push_Fmt(Ctx, &Cmd, "-Fo%.*s", (int)ObjPath.Size, ObjPath.Ptr);
        Build_Cmd_Push(Ctx, &Cmd, SrcPath);
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("/link"), Build_Str_Lit("/opt:ref"), Build_Str_Lit("/incremental:no"), BUILD_STR_END);
        for (int DirIndex = 0; DirIndex < LinkOpts->LibDirs.Count; DirIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "/LIBPATH:%.*s", (int)LinkOpts->LibDirs.Items[DirIndex].Size, LinkOpts->LibDirs.Items[DirIndex].Ptr);
        }
        for (int LibIndex = 0; LibIndex < LinkOpts->Libs.Count; LibIndex++) Build_Cmd_Push(Ctx, &Cmd, LinkOpts->Libs.Items[LibIndex]);
        for (int FlagIndex = 0; FlagIndex < LinkOpts->ExtraFlags.Count; FlagIndex++) Build_Cmd_Push(Ctx, &Cmd, LinkOpts->ExtraFlags.Items[FlagIndex]);
        if (!Build_Str_Empty(LinkOpts->PdbPath)) Build_Cmd_Push_Fmt(Ctx, &Cmd, "/PDB:%.*s", (int)LinkOpts->PdbPath.Size, LinkOpts->PdbPath.Ptr);
        if (LinkOpts->Shared) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "/IMPLIB:%.*s", (int)ImplibPath.Size, ImplibPath.Ptr);
        }
        Build_Cmd_Push_Fmt(Ctx, &Cmd, "/out:%.*s", (int)LinkOpts->OutPath.Size, LinkOpts->OutPath.Ptr);
    } else {
        Build_Cmd_Push_Lit(Ctx, &Cmd, "-g");
        Build_Cmd_Push_Lit(Ctx, &Cmd, Co.Debug ? "-O0" : "-O3");
        if (Co.PIC) Build_Cmd_Push_Lit(Ctx, &Cmd, "-fPIC");
        if (Co.Cpp) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "-std=c++%d", Co.Standard ? Co.Standard : 20);
            if (Co.NoRTTI) Build_Cmd_Push_Lit(Ctx, &Cmd, "-fno-rtti");
            if (Co.NoExceptions) Build_Cmd_Push_Lit(Ctx, &Cmd, "-fno-exceptions");
        }
        if (Co.Asan) Build_Cmd_Push_Lit(Ctx, &Cmd, "-fsanitize=address");
        if (Co.Tsan) Build_Cmd_Push_Lit(Ctx, &Cmd, "-fsanitize=thread");
        if (Co.Ubsan) Build_Cmd_Push_Lit(Ctx, &Cmd, "-fsanitize=undefined");
        for (int IncludeIndex = 0; IncludeIndex < Co.Includes.Count; IncludeIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "-I%.*s", (int)Co.Includes.Items[IncludeIndex].Size, Co.Includes.Items[IncludeIndex].Ptr);
        }
        for (int DefineIndex = 0; DefineIndex < Co.Defines.Count; DefineIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "-D%.*s", (int)Co.Defines.Items[DefineIndex].Size, Co.Defines.Items[DefineIndex].Ptr);
        }
        for (int FlagIndex = 0; FlagIndex < Co.ExtraFlags.Count; FlagIndex++) Build_Cmd_Push(Ctx, &Cmd, Co.ExtraFlags.Items[FlagIndex]);
        if (LinkOpts->Shared) {
            Build_Cmd_Push_Lit(Ctx, &Cmd, "-shared");
            if (!Build_Str_Empty(LinkOpts->InstallName)) {
                Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-install_name"), LinkOpts->InstallName, BUILD_STR_END);
            }
        }
        Build_Cmd_Push(Ctx, &Cmd, SrcPath);
        for (int DirIndex = 0; DirIndex < LinkOpts->LibDirs.Count; DirIndex++) {
            Build_Cmd_Push_Fmt(Ctx, &Cmd, "-L%.*s", (int)LinkOpts->LibDirs.Items[DirIndex].Size, LinkOpts->LibDirs.Items[DirIndex].Ptr);
        }
        for (int LibIndex = 0; LibIndex < LinkOpts->Libs.Count; LibIndex++) {
            build_string Lib = LinkOpts->Libs.Items[LibIndex];
            if (Build_Is_File(Lib) || (Lib.Size && Lib.Ptr[0] == '-')) {
                Build_Cmd_Push(Ctx, &Cmd, Lib);
            } else {
                Build_Cmd_Push_Fmt(Ctx, &Cmd, "-l%.*s", (int)Lib.Size, Lib.Ptr);
            }
        }
        for (int FrameworkIndex = 0; FrameworkIndex < LinkOpts->Frameworks.Count; FrameworkIndex++) {
            Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-framework"), LinkOpts->Frameworks.Items[FrameworkIndex], BUILD_STR_END);
        }
        for (int FlagIndex = 0; FlagIndex < LinkOpts->ExtraFlags.Count; FlagIndex++) Build_Cmd_Push(Ctx, &Cmd, LinkOpts->ExtraFlags.Items[FlagIndex]);
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("-o"), LinkOpts->OutPath, BUILD_STR_END);
    }
    int Code = Build_Cmd_Run(Ctx, &Cmd);
    if (Code != 0) {
        Build_Log_Error(Ctx, "compile+link failed: %.*s", (int)SrcPath.Size, SrcPath.Ptr);
        return 0;
    }
    /* One-shot compile+link: drop the intermediate object (output already linked). */
    Build_Remove_File(ObjPath);
    return 1;
}

build_b32 Build_Target(build_context* Ctx, build_target* Target) {
    Build_Log("target %.*s", (int)Target->Name.Size, Target->Name.Ptr ? Target->Name.Ptr : "(unnamed)");
    Build_Compile_Opts_Apply_Defaults(&Target->Compile);
    build_str_list Objs = {0};
    if (!Build_Compile_Many(Ctx, &Target->Compile, &Target->Sources, Ctx->BinPath, &Objs)) {
        return 0;
    }
    build_b32 Result = 1;
    build_string Out = !Build_Str_Empty(Target->OutPath) ? Target->OutPath : Target->Link.OutPath;
    switch (Target->Kind) {
        case BUILD_ARTIFACT_OBJECTS: Result = 1; break;
        case BUILD_ARTIFACT_STATIC: Result = Build_Archive(Ctx, &Target->Compile.Toolchain, &Objs, Out); break;
        case BUILD_ARTIFACT_SHARED:
        case BUILD_ARTIFACT_EXE: {
            Target->Link.Toolchain = Target->Compile.Toolchain;
            Target->Link.Cpp = Target->Compile.Cpp;
            Target->Link.Debug = Target->Compile.Debug;
            Target->Link.Shared = Target->Kind == BUILD_ARTIFACT_SHARED;
            Target->Link.Objects = Objs;
            if (Build_Str_Empty(Target->Link.OutPath)) Target->Link.OutPath = Out;
            Result = Build_Link(Ctx, &Target->Link);
        } break;
    }
    if (Result) Build_Log("target %.*s ok", (int)Target->Name.Size, Target->Name.Ptr ? Target->Name.Ptr : "(unnamed)");
    return Result;
}

build_string Build_Script_Path(build_context* Ctx, build_string ScriptStem) {
    return Build_Fmt(Ctx, "%.*s%s", (int)ScriptStem.Size, ScriptStem.Ptr,
                     Build_Os() == BUILD_OS_WIN32 ? ".bat" : ".sh");
}

static void Build__Cmd_Push_Script(build_context* Ctx, build_cmd* Cmd, build_string ScriptStem) {
    build_string Script = Build_Script_Path(Ctx, ScriptStem);
    if (Build_Os() == BUILD_OS_WIN32) {
        Build_Cmd_Push_Lit(Ctx, Cmd, "cmd");
        Build_Cmd_Push_Lit(Ctx, Cmd, "/c");
    } else {
        Build_Cmd_Push_Lit(Ctx, Cmd, "bash");
    }
    Build_Cmd_Push(Ctx, Cmd, Script);
}

build_b32 Build_Run(build_context* Ctx, build_string Exe, ...) {
    build_cmd Cmd = {0};
    Build_Cmd_Push(Ctx, &Cmd, Exe);
    va_list Args;
    va_start(Args, Exe);
    for (;;) {
        build_string Arg = va_arg(Args, build_string);
        if (!Arg.Ptr) break;
        Build_Cmd_Push(Ctx, &Cmd, Arg);
    }
    va_end(Args);
    int Code = Build_Cmd_Run(Ctx, &Cmd);
    if (Code != 0) {
        Build_Log_Error(Ctx, "run failed: %.*s", (int)Exe.Size, Exe.Ptr);
        return 0;
    }
    return 1;
}

build_b32 Build_Run_Script(build_context* Ctx, build_string ScriptStem, ...) {
    build_cmd Cmd = {0};
    Build__Cmd_Push_Script(Ctx, &Cmd, ScriptStem);
    va_list Args;
    va_start(Args, ScriptStem);
    for (;;) {
        build_string Arg = va_arg(Args, build_string);
        if (!Arg.Ptr) break;
        Build_Cmd_Push(Ctx, &Cmd, Arg);
    }
    va_end(Args);
    int Code = Build_Cmd_Run(Ctx, &Cmd);
    if (Code != 0) {
        build_string Script = Build_Script_Path(Ctx, ScriptStem);
        Build_Log_Error(Ctx, "script failed: %.*s", (int)Script.Size, Script.Ptr);
        return 0;
    }
    return 1;
}

build_b32 Build_Run_Script_Cwd(build_context* Ctx, build_string Cwd, build_string ScriptStem, ...) {
    build_cmd Cmd = {0};
    Build__Cmd_Push_Script(Ctx, &Cmd, ScriptStem);
    va_list Args;
    va_start(Args, ScriptStem);
    for (;;) {
        build_string Arg = va_arg(Args, build_string);
        if (!Arg.Ptr) break;
        Build_Cmd_Push(Ctx, &Cmd, Arg);
    }
    va_end(Args);
    int Code = Build_Cmd_Run_Cwd(Ctx, &Cmd, Cwd);
    if (Code != 0) {
        build_string Script = Build_Script_Path(Ctx, ScriptStem);
        Build_Log_Error(Ctx, "script failed: %.*s", (int)Script.Size, Script.Ptr);
        return 0;
    }
    return 1;
}

build_b32 Build_Run_Cwd(build_context* Ctx, build_string Cwd, build_string Exe, ...) {
    build_cmd Cmd = {0};
    Build_Cmd_Push(Ctx, &Cmd, Exe);
    va_list Args;
    va_start(Args, Exe);
    for (;;) {
        build_string Arg = va_arg(Args, build_string);
        if (!Arg.Ptr) break;
        Build_Cmd_Push(Ctx, &Cmd, Arg);
    }
    va_end(Args);
    int Code = Build_Cmd_Run_Cwd(Ctx, &Cmd, Cwd);
    if (Code != 0) {
        Build_Log_Error(Ctx, "run failed: %.*s", (int)Exe.Size, Exe.Ptr);
        return 0;
    }
    return 1;
}

build_b32 Build_Stage_File(build_context* Ctx, build_string Src, build_string DstDir) {
    if (!Build_Is_File(Src)) {
        Build_Log_Error(Ctx, "stage: missing %.*s", (int)Src.Size, Src.Ptr);
        return 0;
    }
    Build_Mkdir_All(DstDir);
    build_string Name = Build__Filename(Src);
    build_string Dst = Build_Path_Join(Ctx, DstDir, Name);
    if (!Ctx->Force && Build_Is_File(Dst) && Build_File_Mtime(Dst) >= Build_File_Mtime(Src)) return 1;
    Build_Log("stage %.*s -> %.*s", (int)Src.Size, Src.Ptr, (int)Dst.Size, Dst.Ptr);
    if (!Build_Copy_File(Src, Dst)) {
        Build_Log_Error(Ctx, "copy failed");
        return 0;
    }
    return 1;
}

build_b32 Build_Git_Is_Available(void) {
#ifdef _WIN32
    return system("where git >nul 2>&1") == 0;
#else
    return system("command -v git >/dev/null 2>&1") == 0;
#endif
}

static build_b32 Build__Git_Stdin_Is_Tty(void) {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

build_b32 Build_Git_Require(build_context* Ctx) {
    if (Build_Git_Is_Available()) return 1;

    Build_Log_Error(Ctx, "git is not installed (or not on PATH)");
    fprintf(stderr, "[build] Git is required to fetch submodules / dependencies.\n");
#ifdef _WIN32
    fprintf(stderr, "[build] Install options:\n");
    fprintf(stderr, "[build]   winget install --id Git.Git -e --source winget\n");
    fprintf(stderr, "[build]   https://git-scm.com/download/win\n");
#else
    fprintf(stderr, "[build] Install options:\n");
    fprintf(stderr, "[build]   macOS:  brew install git\n");
    fprintf(stderr, "[build]   Debian: sudo apt-get install git\n");
    fprintf(stderr, "[build]   https://git-scm.com/downloads\n");
#endif

    if (!Build__Git_Stdin_Is_Tty()) return 0;

    fprintf(stderr, "[build] Attempt package-manager install now? [y/N] ");
    fflush(stderr);
    char Line[32];
    if (!fgets(Line, sizeof(Line), stdin)) return 0;
    if (Line[0] != 'y' && Line[0] != 'Y') return 0;

#ifdef _WIN32
    Build_Log("running: winget install --id Git.Git -e --source winget");
    if (system("winget install --id Git.Git -e --source winget") != 0) {
        Build_Log_Error(Ctx, "winget install failed — install Git manually, then re-run the build");
        return 0;
    }
#elif defined(__APPLE__)
    Build_Log("running: brew install git");
    if (system("brew install git") != 0) {
        Build_Log_Error(Ctx, "brew install failed — install Git manually, then re-run the build");
        return 0;
    }
#else
    Build_Log("running: sudo apt-get install -y git");
    if (system("sudo apt-get install -y git") != 0) {
        Build_Log_Error(Ctx, "apt-get install failed — install Git manually, then re-run the build");
        return 0;
    }
#endif

    if (!Build_Git_Is_Available()) {
        Build_Log_Error(Ctx, "git still not on PATH — open a new terminal (or restart the shell) and re-run the build");
        return 0;
    }
    Build_Log("git is available");
    return 1;
}

build_b32 Build_Git_Submodule_Is_Checked_Out(build_context* Ctx, build_string RelPath) {
    build_string Abs = Build_Path_Join(Ctx, Ctx->RepoRoot, RelPath);
    if (!Build_Is_Dir(Abs)) return 0;
    if (Build_Path_Exists(Build_Path_Join(Ctx, Abs, Build_Str_Lit(".git")))) return 1;
    if (Build_Path_Exists(Build_Path_Join(Ctx, Abs, Build_Str_Lit("CMakeLists.txt")))) return 1;
    if (Build_Path_Exists(Build_Path_Join(Ctx, Abs, Build_Str_Lit("build.bat")))) return 1;
    if (Build_Path_Exists(Build_Path_Join(Ctx, Abs, Build_Str_Lit("build.sh")))) return 1;
    if (Build_Path_Exists(Build_Path_Join(Ctx, Abs, Build_Str_Lit("code")))) return 1;
    return 0;
}

static build_b32 Build__Git_Submodule_Ensure_Ex(build_context* Ctx, build_string RelPath, build_b32 Recursive) {
    if (Build_Git_Submodule_Is_Checked_Out(Ctx, RelPath)) {
        Build_Log("submodule ok %.*s", (int)RelPath.Size, RelPath.Ptr);
        return 1;
    }
    if (!Build_Git_Require(Ctx)) return 0;
    Build_Log("submodule ensure %.*s%s", (int)RelPath.Size, RelPath.Ptr, Recursive ? " (recursive)" : "");
    build_cmd Cmd = {0};
    Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("git"), Build_Str_Lit("-C"), Ctx->RepoRoot,
                    Build_Str_Lit("submodule"), Build_Str_Lit("update"), Build_Str_Lit("--init"), BUILD_STR_END);
    if (Recursive) Build_Cmd_Push_Lit(Ctx, &Cmd, "--recursive");
    Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--depth"), Build_Str_Lit("1"), Build_Str_Lit("--"), RelPath, BUILD_STR_END);
    int Code = Build_Cmd_Run(Ctx, &Cmd);
    if (Code != 0) {
        Build_Log_Error(Ctx, "git submodule update failed");
        return 0;
    }
    return Build_Git_Submodule_Is_Checked_Out(Ctx, RelPath);
}

build_b32 Build_Git_Submodule_Ensure(build_context* Ctx, build_string RelPath) {
    return Build__Git_Submodule_Ensure_Ex(Ctx, RelPath, 0);
}

build_b32 Build_Git_Submodule_Ensure_Recursive(build_context* Ctx, build_string RelPath) {
    return Build__Git_Submodule_Ensure_Ex(Ctx, RelPath, 1);
}

build_b32 Build_Git_Submodule_Update(build_context* Ctx, build_string RelPath, build_b32 Recursive) {
    if (!Build_Git_Require(Ctx)) return 0;
    Build_Log("submodule update %.*s%s", (int)RelPath.Size, RelPath.Ptr, Recursive ? " (recursive)" : "");
    build_cmd Cmd = {0};
    Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("git"), Build_Str_Lit("-C"), Ctx->RepoRoot,
                    Build_Str_Lit("submodule"), Build_Str_Lit("update"), Build_Str_Lit("--init"), BUILD_STR_END);
    if (Recursive) Build_Cmd_Push_Lit(Ctx, &Cmd, "--recursive");
    Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--"), RelPath, BUILD_STR_END);
    if (Build_Cmd_Run(Ctx, &Cmd) != 0) {
        Build_Log_Error(Ctx, "git submodule update failed");
        return 0;
    }
    return Build_Git_Submodule_Is_Checked_Out(Ctx, RelPath);
}

build_b32 Build_Cmake(build_context* Ctx, build_cmake* Cmake) {
    if (!Build_Str_Empty(Cmake->StampFile) && !Ctx->Force && Build_Is_File(Cmake->StampFile)) {
        Build_Log("skip cmake (stamp)");
        return 1;
    }
    Build_Mkdir_All(Cmake->BuildDir);
    build_string Cache = Build_Path_Join(Ctx, Cmake->BuildDir, Build_Str_Lit("CMakeCache.txt"));
    if (Ctx->Force || !Build_Is_File(Cache)) {
        build_cmd Cmd = {0};
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("cmake"), Build_Str_Lit("-S"), Cmake->SourceDir,
                        Build_Str_Lit("-B"), Cmake->BuildDir, BUILD_STR_END);
        for (int ArgIndex = 0; ArgIndex < Cmake->ConfigureArgs.Count; ArgIndex++) {
            Build_Cmd_Push(Ctx, &Cmd, Cmake->ConfigureArgs.Items[ArgIndex]);
        }
        int Code = Build_Cmd_Run(Ctx, &Cmd);
        if (Code != 0) { Build_Log_Error(Ctx, "cmake configure failed"); return 0; }
    }
    {
        build_cmd Cmd = {0};
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("cmake"), Build_Str_Lit("--build"), Cmake->BuildDir, BUILD_STR_END);
        if (!Build_Str_Empty(Cmake->Config)) {
            Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--config"), Cmake->Config, BUILD_STR_END);
        }
        Build_Cmd_Push_Lit(Ctx, &Cmd, "--parallel");
        int Code = Build_Cmd_Run(Ctx, &Cmd);
        if (Code != 0) { Build_Log_Error(Ctx, "cmake build failed"); return 0; }
    }
    if (!Build_Str_Empty(Cmake->ExtraBuildTarget)) {
        build_cmd Cmd = {0};
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("cmake"), Build_Str_Lit("--build"), Cmake->BuildDir, BUILD_STR_END);
        if (!Build_Str_Empty(Cmake->Config)) Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--config"), Cmake->Config, BUILD_STR_END);
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--parallel"), Build_Str_Lit("--target"), Cmake->ExtraBuildTarget, BUILD_STR_END);
        int Code = Build_Cmd_Run(Ctx, &Cmd);
        if (Code != 0) { Build_Log_Error(Ctx, "cmake target failed"); return 0; }
    }
    if (!Build_Str_Empty(Cmake->InstallPrefix)) {
        build_cmd Cmd = {0};
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("cmake"), Build_Str_Lit("--install"), Cmake->BuildDir, BUILD_STR_END);
        if (!Build_Str_Empty(Cmake->Config)) Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--config"), Cmake->Config, BUILD_STR_END);
        Build_Cmd_PushV(Ctx, &Cmd, Build_Str_Lit("--prefix"), Cmake->InstallPrefix, BUILD_STR_END);
        int Code = Build_Cmd_Run(Ctx, &Cmd);
        if (Code != 0) { Build_Log_Error(Ctx, "cmake install failed"); return 0; }
    }
    return 1;
}

#endif /* BUILD_FRAMEWORK_IMPLEMENTATION_DONE */
#endif /* BUILD_FRAMEWORK_IMPLEMENTATION */
