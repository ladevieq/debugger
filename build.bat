@ECHO OFF

:: Unpack Arguments
set release=0
for %%a in (%*) do set "%%a=1"

set BUILD_DIR=build

:: Clean directory
IF EXIST %BUILD_DIR% rmdir %BUILD_DIR%\ /S /Q

mkdir build

set VS2022_COM="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set VS2022_PRO="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

if EXIST %VS2022_COM% (
    call %VS2022_COM%
    goto compilation
)
if EXIST %VS2022_PRO% (
    call %VS2022_PRO%
    goto compilation
)


:compilation
set C_FLAGS=/DHASHMAP /nologo /W4 /WX /Zi /GS- /GR- /Gs1000000000 /Fo:%BUILD_DIR%\ /Iinclude /Isrc /std:c17 /c /Tc
REM set C_FLAGS=/nologo /W4 /WX /Zi /GS- /GR- /Gs1000000000 /Fo:%BUILD_DIR%\ /Iinclude /Isrc /std:c17 /c /Tc
set L_FLAGS=/WX /SUBSYSTEM:CONSOLE /NODEFAULTLIB /stack:0x100000,100000

if "%release%"=="0" (
    set C_FLAGS=/Od %C_FLAGS%
    set L_FLAGS=/DEBUG %L_FLAGS%
)

set src=main utils events module parser pdb hashmap stack_walk

:: DBG
:: Compile
(for %%s in (%src%) do (
    call set OBJS=%%OBJS%% %BUILD_DIR%\%%s.obj
    cl /Fd:%BUILD_DIR%\%%s.pdb %C_FLAGS% src\%%s.c
))

:: Link
link %L_FLAGS% /OUT:%BUILD_DIR%\main.exe %OBJS% kernel32.lib

:: Test program
set SRC_FILE=test
:: Compile
cl /Fd:%BUILD_DIR%\%SRC_FILE%.pdb %C_FLAGS% src/%SRC_FILE%.c

:: Link
link %L_FLAGS% %BUILD_DIR%\%SRC_FILE%.obj /OUT:%BUILD_DIR%\%SRC_FILE%.exe kernel32.lib
