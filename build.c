#define BUILD_FRAMEWORK_IMPLEMENTATION
#include "code/build_framework.h"

typedef struct base_config {
    build_b32 PreferClang;
    build_b32 Cpp;
    build_b32 BuildGdi;
    build_b32 BuildGeometry;
    build_b32 BuildMeta;
    build_b32 Tests;
    build_string TracyPath;
} base_config;

static void Base_Usage(void) {
    fprintf(stderr,
            "base build options:\n"
            "  -clang              Prefer clang (Windows)\n"
            "  -cpp                Build as C++\n"
            "  -no_gdi             Skip GDI / Vulkan backend\n"
            "  -no_geometry        Skip geometry module\n"
            "  -tests              Build unit tests\n"
            "  -tracy_path <dir>   Enable Tracy client\n");
}

static base_config Base_Parse_Config(build_context* Ctx) {
    base_config Config = {0};
    Config.BuildGdi = 1;
    Config.BuildGeometry = 1;
    Config.BuildMeta = 1;
#ifndef _WIN32
    Config.PreferClang = 1;
#endif

    for (int ArgIndex = 0; ArgIndex < Ctx->UserArgs.Count; ArgIndex++) {
        build_string Arg = Ctx->UserArgs.Items[ArgIndex];
        if (Build_Str_Eq_Lit(Arg, "-clang")) {
            Config.PreferClang = 1;
        } else if (Build_Str_Eq_Lit(Arg, "-cpp")) {
            Config.Cpp = 1;
        } else if (Build_Str_Eq_Lit(Arg, "-no_gdi")) {
            Config.BuildGdi = 0;
        } else if (Build_Str_Eq_Lit(Arg, "-no_geometry")) {
            Config.BuildGeometry = 0;
        } else if (Build_Str_Eq_Lit(Arg, "-tests")) {
            Config.Tests = 1;
        } else if (Build_Str_Eq_Lit(Arg, "-tracy_path")) {
            if (ArgIndex + 1 >= Ctx->UserArgs.Count) {
                Base_Usage();
                Build_Die(Ctx, "-tracy_path requires a directory");
            }
            Config.TracyPath = Ctx->UserArgs.Items[++ArgIndex];
        } else if (Build_Str_Eq_Lit(Arg, "-h") || Build_Str_Eq_Lit(Arg, "--help")) {
            Base_Usage();
            exit(0);
        } else {
            Base_Usage();
            Build_Die(Ctx, "unknown flag '%.*s'", (int)Arg.Size, Arg.Ptr);
        }
    }
    return Config;
}

static build_compile_opts Base_Make_Compile_Opts(build_context* Ctx, base_config* Config, build_toolchain* Tc) {
    build_compile_opts Opts = {0};
    Opts.Toolchain = *Tc;
    Opts.Cpp = Config->Cpp;
    Opts.Debug = Ctx->Debug;
    Opts.Asan = Ctx->Asan;
    Opts.Tsan = Ctx->Tsan;
    Opts.Ubsan = Ctx->Ubsan;
    Opts.Standard = Config->Cpp ? 20 : 11;
    Opts.MsvcWall = 1;
    Opts.WarningsAsErrors = 1;
    Build_Compile_Opts_Apply_Defaults(&Opts);

    Build_Str_List_Push_Many(Ctx, &Opts.Defines, "ENABLE_OVERRIDE=0", "RPMALLOC_FIRST_CLASS_HEAPS=1");
    if (Ctx->Debug) Build_Str_List_Push_Lit(Ctx, &Opts.Defines, "DEBUG_BUILD");

    if (!Build_Str_Empty(Config->TracyPath) && Build_Is_Dir(Config->TracyPath)) {
        Build_Str_List_Push_Many(Ctx, &Opts.Defines, "TRACY_ENABLE", "TRACY_IMPORTS");
        Build_Str_List_Push(Ctx, &Opts.Includes, Build_Path_Join(Ctx, Config->TracyPath, Build_Str_Lit("public/tracy")));
    }

    return Opts;
}

static build_string Base_Source_Ext(base_config* Config) {
    return Config->Cpp ? Build_Str_Lit("cpp") : Build_Str_Lit("c");
}

int main(int ArgCount, char** Args) {
    build_context Ctx = Build_Init(ArgCount, Args);
    base_config Config = Base_Parse_Config(&Ctx);

    if (!Build_Is_File(Build_Path_Join(&Ctx, Ctx.Root, Build_Str_Lit("code/build_framework.h")))) {
        Build_Die(&Ctx, "cannot find code/build_framework.h under %.*s", (int)Ctx.Root.Size, Ctx.Root.Ptr);
    }

    build_string CodePath = Build_Path_Join(&Ctx, Ctx.Root, Build_Str_Lit("code"));
    build_toolchain Tc = Build_Toolchain_Detect(&Ctx, Config.PreferClang);
    build_string Ext = Base_Source_Ext(&Config);

    if (!Build_Str_Empty(Config.TracyPath) && Build_Is_Dir(Config.TracyPath)) {
        build_compile_opts Co = Base_Make_Compile_Opts(&Ctx, &Config, &Tc);
        Co.Cpp = 1;
        Co.Standard = 20;
        build_link_opts Lo = {0};
        Lo.Toolchain = Tc;
        Lo.Cpp = 1;
        Lo.Debug = Ctx.Debug;
        Lo.Shared = 1;
        Lo.OutPath = Build_Dll_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("TracyClient"));
        Build_Str_List_Push_Many(&Ctx, &Co.Defines, "TRACY_ENABLE", "TRACY_EXPORTS");
        if (!Build_Compile_And_Link(&Ctx, &Co, &Lo,
                                    Build_Path_Join(&Ctx, Config.TracyPath, Build_Str_Lit("public/TracyClient.cpp")))) {
            return 1;
        }
    }

    build_compile_opts Co = Base_Make_Compile_Opts(&Ctx, &Config, &Tc);
    build_str_list Objs = {0};

    build_string RpmallocSrc = Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("third_party/rpmalloc/rpmalloc.c"));
    build_string RpmallocObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("rpmalloc"));
    {
        build_compile_opts C = Co;
        C.Cpp = 0;
        C.Standard = 11;
        if (!Build_Compile(&Ctx, &C, RpmallocSrc, RpmallocObj)) return 1;
        Build_Str_List_Push(&Ctx, &Objs, RpmallocObj);
    }

    build_string BaseSrc = Build_Fmt(&Ctx, "%.*s/base.%.*s", (int)CodePath.Size, CodePath.Ptr, (int)Ext.Size, Ext.Ptr);
    build_string BaseObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("base"));
    if (!Build_Compile(&Ctx, &Co, BaseSrc, BaseObj)) return 1;
    Build_Str_List_Push(&Ctx, &Objs, BaseObj);

    build_string OsObj = BUILD_STR_END;
    if (Build_Os() == BUILD_OS_WIN32) {
        build_string OsSrc = Build_Fmt(&Ctx, "%.*s/os/win32/win32_base.%.*s",
                                       (int)CodePath.Size, CodePath.Ptr, (int)Ext.Size, Ext.Ptr);
        OsObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("win32_base"));
        if (!Build_Compile(&Ctx, &Co, OsSrc, OsObj)) return 1;
    } else {
        build_string OsSrc = Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("os/posix/posix_base.c"));
        build_compile_opts C = Co;
        C.Cpp = 0;
        C.Standard = 11;
        OsObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("posix_base"));
        if (!Build_Compile(&Ctx, &C, OsSrc, OsObj)) return 1;
    }
    Build_Str_List_Push(&Ctx, &Objs, OsObj);

    if (Config.BuildMeta) {
        build_string MetaSrc = Build_Fmt(&Ctx, "%.*s/meta_program/meta_program.%.*s",
                                         (int)CodePath.Size, CodePath.Ptr, (int)Ext.Size, Ext.Ptr);
        build_string MetaObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("meta_program"));
        if (!Build_Compile(&Ctx, &Co, MetaSrc, MetaObj)) return 1;

        build_string MetaOut = Build_Exe_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("meta_program"));
        build_str_list MetaObjs = {0};
        for (int ObjIndex = 0; ObjIndex < Objs.Count; ObjIndex++) {
            Build_Str_List_Push(&Ctx, &MetaObjs, Objs.Items[ObjIndex]);
        }
        Build_Str_List_Push(&Ctx, &MetaObjs, MetaObj);
        build_link_opts Lo = {0};
        Lo.Toolchain = Tc;
        Lo.Cpp = Co.Cpp;
        Lo.Debug = Ctx.Debug;
        Lo.OutPath = MetaOut;
        Lo.Objects = MetaObjs;
        if (Build_Os() != BUILD_OS_WIN32) Build_Str_List_Push_Lit(&Ctx, &Lo.Libs, "m");
        if (!Build_Link(&Ctx, &Lo)) return 1;

        if (Config.BuildGdi) {
            if (!Build_Run(&Ctx, MetaOut, Build_Str_Lit("-d"),
                           Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("gdi")), BUILD_STR_END)) {
                return 1;
            }
        }
    }

    if (Config.BuildGdi) {
        const char* VulkanDefine = "VK_USE_PLATFORM_WIN32_KHR";
        if (Build_Os() == BUILD_OS_OSX) VulkanDefine = "VK_USE_PLATFORM_METAL_EXT";
        else if (Build_Os() == BUILD_OS_LINUX) VulkanDefine = "VK_USE_PLATFORM_XLIB_KHR";

        build_compile_opts GdiCo = Co;
        Build_Str_List_Push_Lit(&Ctx, &GdiCo.Defines, VulkanDefine);
        Build_Str_List_Push(&Ctx, &GdiCo.Includes, Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("third_party/Vulkan-Headers")));
        Build_Str_List_Push(&Ctx, &GdiCo.Includes, Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("third_party/VulkanMemoryAllocator")));
        Build_Str_List_Push(&Ctx, &GdiCo.Includes, CodePath);

        build_string VmaSrc = Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("gdi/backend/vk/vk_vma_usage.cpp"));
        build_string VmaObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("vk_mem_alloc"));
        {
            build_compile_opts C = GdiCo;
            C.Cpp = 1;
            C.Standard = 20;
            if (!Build_Compile(&Ctx, &C, VmaSrc, VmaObj)) return 1;
            Build_Str_List_Push(&Ctx, &Objs, VmaObj);
        }

        if (Build_Os() == BUILD_OS_OSX) {
            build_string GdiExt = Config.Cpp ? Build_Str_Lit("mm") : Build_Str_Lit("m");
            build_string GdiSrc = Build_Fmt(&Ctx, "%.*s/gdi/backend/vk/vk_gdi.%.*s",
                                            (int)CodePath.Size, CodePath.Ptr, (int)GdiExt.Size, GdiExt.Ptr);
            build_string GdiObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("gdi"));
            build_compile_opts C = GdiCo;
            C.ObjC = 1;
            C.Cpp = Config.Cpp;
            if (!Build_Compile(&Ctx, &C, GdiSrc, GdiObj)) return 1;
            Build_Str_List_Push(&Ctx, &Objs, GdiObj);
        } else {
            build_string GdiSrc = Build_Fmt(&Ctx, "%.*s/gdi/backend/vk/vk_gdi.%.*s",
                                            (int)CodePath.Size, CodePath.Ptr, (int)Ext.Size, Ext.Ptr);
            build_string GdiObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("gdi"));
            if (!Build_Compile(&Ctx, &GdiCo, GdiSrc, GdiObj)) return 1;
            Build_Str_List_Push(&Ctx, &Objs, GdiObj);
        }
    }

    if (Config.BuildGeometry) {
        build_string GeoSrc = Build_Fmt(&Ctx, "%.*s/geometry/geometry.%.*s",
                                        (int)CodePath.Size, CodePath.Ptr, (int)Ext.Size, Ext.Ptr);
        build_compile_opts GeoCo = Co;
        if (Build_Os() != BUILD_OS_WIN32) {
            GeoSrc = Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("geometry/geometry.c"));
            GeoCo.Cpp = 1;
            GeoCo.Standard = 20;
            Build_Str_List_Push_Many(&Ctx, &GeoCo.ExtraFlags, "-Wno-deprecated", "-x", "c++");
        }
        Build_Str_List_Push(&Ctx, &GeoCo.Includes, CodePath);
        build_string GeoObj = Build_Obj_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("geometry"));
        if (!Build_Compile(&Ctx, &GeoCo, GeoSrc, GeoObj)) return 1;
        Build_Str_List_Push(&Ctx, &Objs, GeoObj);
    }

    build_string LibOut = Build_Lib_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("base"));
    if (!Build_Archive(&Ctx, &Tc, &Objs, LibOut)) return 1;

    if (Config.Tests) {
        build_string DxcBase = Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("third_party/DirectXShaderCompiler"));
#ifdef _WIN32
        build_string DxcLib = Build_Path_Join(&Ctx, DxcBase, Build_Str_Lit("bin/Release/lib/dxcompiler.lib"));
        if (!Build_Is_File(DxcLib)) {
            build_cmake Cmake = {0};
            Cmake.SourceDir = DxcBase;
            Cmake.BuildDir = Build_Path_Join(&Ctx, DxcBase, Build_Str_Lit("bin"));
            Cmake.Config = Build_Str_Lit("Release");
            Build_Str_List_Push_Many(&Ctx, &Cmake.ConfigureArgs,
                "-G", "Visual Studio 17 2022",
                "-C", "cmake/caches/PredefinedParams.cmake",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DLLVM_TARGETS_TO_BUILD=None",
                "-DLLVM_INCLUDE_TESTS=OFF",
                "-DENABLE_SPIRV_CODEGEN=ON",
                "-DHLSL_INCLUDE_TESTS=OFF");
            if (!Build_Cmake(&Ctx, &Cmake)) return 1;
        }
        if (!Build_Stage_File(&Ctx, DxcLib, Ctx.BinPath)) return 1;
        if (!Build_Stage_File(&Ctx, Build_Path_Join(&Ctx, DxcBase, Build_Str_Lit("bin/Release/bin/dxcompiler.dll")), Ctx.BinPath)) return 1;

        build_compile_opts TCo = Co;
        TCo.Cpp = 1;
        TCo.Standard = 20;
        Build_Str_List_PushV(&Ctx, &TCo.Includes, CodePath,
            Build_Path_Join(&Ctx, DxcBase, Build_Str_Lit("include")), BUILD_STR_END);
        build_link_opts TLo = {0};
        TLo.Toolchain = Tc;
        TLo.Cpp = 1;
        TLo.Debug = Ctx.Debug;
        TLo.OutPath = Build_Exe_Path(&Ctx, &Tc, Ctx.BinPath, Build_Str_Lit("unit_tests"));
        Build_Str_List_Push_Many(&Ctx, &TLo.Libs, "base.lib", "dxcompiler.lib");
        Build_Str_List_Push(&Ctx, &TLo.LibDirs, Ctx.BinPath);
        if (!Build_Compile_And_Link(&Ctx, &TCo, &TLo,
                                    Build_Path_Join(&Ctx, CodePath, Build_Str_Lit("tests/unit_tests/unit_tests.cpp")))) {
            return 1;
        }
#else
        Build_Log("tests: non-Windows test wiring left as in build.sh (dxc/sdl)");
#endif
    }

    Build_Quit(&Ctx);
    return Ctx.Failed ? 1 : 0;
}
