@echo off
rem
rem AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
rem https://ai-softphone.com
rem
rem SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
rem SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
rem SPDX-License-Identifier: GPL-2.0-or-later
rem
rem This file is part of AI Softphone. There is NO WARRANTY, to the extent
rem permitted by law. See the file LICENSE for the full terms.

rem Puts the Visual Studio compiler, the linker and Ninja on the PATH of the
rem calling script.
rem
rem   call scripts\windows-build-environment.cmd || exit /b 1
rem
rem `call`, not `cmd /c`: the whole point is that the variables survive into the
rem script that asked, which is why this is a .cmd rather than something the
rem PowerShell scripts could share.
rem
rem **No third-party action for the compiler environment.** `vswhere` ships with
rem every Visual Studio installer and answers where this machine's build tools
rem are, so the one thing that could drift is a path Microsoft owns rather than
rem an action somebody else does. Written down once here because three jobs
rem needed the same six lines and the third one is where a copy goes stale.

setlocal
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FOUND=%%i"
endlocal & set "SOFTPHONE_VSPATH=%FOUND%"

if not defined SOFTPHONE_VSPATH (
    echo No Visual Studio with the C++ build tools on this machine.
    echo Install the "Desktop development with C++" workload.
    exit /b 1
)

rem vcvars64.bat calls `vswhere` by bare name without putting it on the PATH
rem itself, and stops with "not recognized" when nothing else has.
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%SOFTPHONE_VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

rem Ninja from the same installation, when the machine has not put one on the
rem PATH already.
where ninja >nul 2>&1 || set "PATH=%SOFTPHONE_VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

exit /b 0
