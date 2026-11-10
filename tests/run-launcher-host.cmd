@echo off
setlocal
set "ROOT=%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 2
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VS=%%i\Common7\Tools\VsDevCmd.bat"
if not exist "%VS%" exit /b 2
call "%VS%" -arch=x64 >nul
if errorlevel 1 exit /b 2
if not exist "%ROOT%\local\m013" mkdir "%ROOT%\local\m013"
cl /nologo /std:c++17 /EHsc /utf-8 /I"%ROOT%\mkxp-z-nx\nx" /I"%ROOT%\src\mkxp-z\src" "%ROOT%\tests\launcher_host.cpp" "%ROOT%\mkxp-z-nx\nx\launcher_library.cpp" /Fe:"%ROOT%\local\m013\launcher_host.exe"
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 2
"%ROOT%\local\m013\launcher_host.exe" "%ROOT%\local\m013" "%~1\switch\FangameNX"
