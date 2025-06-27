@echo off

setlocal enabledelayedexpansion

set tracy_path=
set build_debug=0
set build_clang=0
set build_asan=0
set build_cpp=0
set build_gdi=1

set base_path=%~dp0
set code_path=%base_path%\code
set bin_path=%base_path%\bin

:cmd_line
if "%~1"=="" goto end_cmd_line

if /i "%~1"=="-tracy_path" (
	set tracy_path="%~2"
	shift
)

if /i "%~1"=="-debug" (
	set build_debug=1
)

if /i "%~1"=="-clang" (
	set build_clang=1
)

if /i "%~1"=="-asan" (
	set build_asan=1
)

if /i "%~1"=="-bin_path" (
	set bin_path="%~2"
	shift
)

if /i "%~1"=="-cpp" (
	set build_cpp=1
)

if /i "%~1"=="-no_gdi" (
	set build_gdi=0
)

shift
goto cmd_line
:end_cmd_line

set build_meta=1
set build_tracy=0

if exist "%tracy_path%" (
	set build_tracy=1
)

if not exist "%bin_path%" (
	mkdir "%bin_path%"
)

for /f "delims=" %%i in ('where clang') do (
	set "clang_path=%%~dpi"
)
set clang_path=%clang_path%..\lib\clang\21\lib\windows\

if %build_asan% == 1 (
	if %build_clang% == 1 (
		xcopy "%clang_path%*" "%bin_path%\" /E /I /H /Y >nul 2>&1
	)

	if %build_clang% == 0 (
		del /q "%bin_path%\clang_rt*" >nul 2>&1
	)
)

set app_includes=
set app_defines=-DENABLE_OVERRIDE=0 -DRPMALLOC_FIRST_CLASS_HEAPS=1

if %build_debug% == 1 (
	set app_defines=%app_defines% -DDEBUG_BUILD
)

if %build_tracy% == 1 (
	set app_defines=%app_defines% -DTRACY_ENABLE -DTRACY_IMPORTS
	set app_includes=%app_includes% -I"%tracy_path%\public\tracy"
)

set msvc_optimized_flag=/Od
set clang_optimized_flag=-O0
if %build_debug% == 0 (
	set msvc_optimized_flag=/O2
	set clang_optimized_flag=-O3
)

set clang_dll=-shared
set clang_compile_only=-c
set clang_warnings=-Wall -Werror -Wno-missing-braces -Wno-switch -Wno-unused-function -Wno-unused-const-variable -Wno-nullability-completeness -Wno-unused-variable -Wno-microsoft-cast
set clang_flags=-g -gcodeview %clang_optimized_flag%
set clang_out=-o 
set clang_link=

set msvc_dll=-LD
set msvc_compile_only=/c
set msvc_warnings=/Wall /WX /wd4061 /wd4062 /wd4063 /wd4100 /wd4127 /wd4189 /wd4191 /wd4201 /wd4255 /wd4324 /wd4355 /wd4365 /wd4456 /wd4464 /wd4505 /wd4530 /wd4577 /wd4625 /wd4626 /wd4668 /wd4710 /wd4711 /wd4820 /wd5026 /wd5027 /wd5045 /wd5246 /wd5250 /wd5262
set msvc_flags=/nologo /FC /Z7 /experimental:c11atomics %msvc_optimized_flag%
set msvc_out=/out:
set msvc_link=/link /opt:ref /incremental:no

REM enable string memory pools for MSVC
set msvc_flags=%msvc_flags% /GF

if %build_asan% == 1 (
	set clang_flags=%clang_flags% -fsanitize=address
	set msvc_flags=%msvc_flags% -fsanitize=address
)

if %build_clang% == 1 (
	set obj_out=-o
	set compile_only=%clang_compile_only%
	set compile_warnings=%clang_warnings%
	set compile_flags=%clang_flags% 
	set compile_out=%clang_out%
	set compile_dll=%clang_dll%
	set compile_link=%clang_link%
	set compiler_c=clang -std=c11
	set compiler_cpp=clang++ -std=c++20
)

if %build_clang% == 0 (
	set obj_out=-Fo:
	set compile_only=%msvc_compile_only%
	set compile_warnings=%msvc_warnings%
	set compile_flags=%msvc_flags%
	set compile_out=%msvc_out%
	set compile_dll=%msvc_dll%
	set compile_link=%msvc_link%
	set compiler_c=cl /std:c11
	set compiler_cpp=cl /std:c++20
)

set compiler=%compiler_c%
set c_ext=c
if %build_cpp% == 1 (
	set compiler=%compiler_cpp%
	set c_ext=cpp
)

REM build tracy if enabled
if %build_tracy% == 0 (
	goto skip_tracy
)

pushd "%bin_path%"
	%compiler_cpp% %compile_flags% %compile_warnings% %compile_dll% -DTRACY_ENABLE -DTRACY_EXPORTS "%tracy_path%\public\TracyClient.cpp" %compile_link% %compile_out%TracyClient.dll
popd

:skip_tracy

REM build
pushd "%bin_path%"
	%compiler_c% %compile_flags% %compile_warnings% %compile_only% %app_defines% %app_includes% %obj_out% rpmalloc.obj "%code_path%\third_party\rpmalloc\rpmalloc.c" 
	%compiler% %compile_flags% %compile_warnings% %compile_only% %app_defines% %app_includes% %obj_out% base.obj "%code_path%\base.%c_ext%"
	%compiler% %compile_flags% %compile_warnings% %compile_only% %app_defines% %app_includes% %obj_out% win32_base.obj "%code_path%\os\win32\win32_base.%c_ext%"
popd

if %build_meta% == 0 (
	goto skip_meta
)

REM build and run meta program
pushd "%bin_path%"
	%compiler% %compile_flags% %compile_warnings% %app_defines% %app_includes% "%code_path%\meta_program\meta_program.%c_ext%" base.obj win32_base.obj rpmalloc.obj %compile_link% %compile_out%meta_program.exe	
popd

:skip_meta

REM build gdi
if %build_gdi% == 0 (
	goto skip_gdi
)

pushd "%bin_path%"
	meta_program.exe -d "%code_path%\gdi"
	%compiler_cpp% %compile_flags% %compile_warnings% %compile_only% %app_defines% -DVK_USE_PLATFORM_WIN32_KHR %app_includes% -I"%code_path%\third_party\Vulkan-Headers" -I"%code_path%\third_party\VulkanMemoryAllocator" %obj_out% vk_mem_alloc.obj "%code_path%\gdi\backend\vk\vk_vma_usage.cpp"
	%compiler% %compile_flags% %compile_warnings% %compile_only% %app_defines% -DVK_USE_PLATFORM_WIN32_KHR %app_includes% -I"%code_path%\third_party\Vulkan-Headers" -I"%code_path%\third_party\VulkanMemoryAllocator" -I"%code_path%" %obj_out% gdi.obj "%code_path%\gdi\backend\vk\vk_gdi.%c_ext%"
popd

:skip_gdi

set obj_files=win32_base.obj base.obj rpmalloc.obj
if %build_gdi% == 1 (
	set obj_files=%obj_files% gdi.obj vk_mem_alloc.obj
)

pushd "%bin_path%"
	lib /nologo /out:base.lib %obj_files%
popd