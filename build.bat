@echo off
setlocal

set base_path=%~dp0
if "%base_path:~-1%"=="\" set "base_path=%base_path:~0,-1%"

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1

if not exist "%base_path%\bin" mkdir "%base_path%\bin"

pushd "%base_path%"
echo [build] compiling bin\build.exe
cl /nologo /std:c11 /W3 /O2 /Fo:bin\build.obj /Fe:bin\build.exe build.c
if errorlevel 1 (
    popd
    exit /b 1
)
del /q bin\build.obj 2>nul

bin\build.exe %*
set exit_code=%errorlevel%
popd
exit /b %exit_code%
