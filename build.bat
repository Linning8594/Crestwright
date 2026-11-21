@echo off
rem SPDX-License-Identifier: AGPL-3.0-or-later
rem Copyright (c) 2026 Linning
call "D:\VisualStudio\VC\Auxiliary\Build\vcvars64.bat" >nul
set CMAKE="D:\VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set NINJA="D:\VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
%CMAKE% -S "D:\CheStuWorSpace_1\Crestwright" -B "D:\CheStuWorSpace_1\Crestwright\build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=%NINJA% -DJUCE_BUILD_EXAMPLES=OFF -DJUCE_BUILD_EXTRAS=OFF
if errorlevel 1 exit /b 1
%CMAKE% --build "D:\CheStuWorSpace_1\Crestwright\build" --config Release
