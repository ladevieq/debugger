@ECHO OFF

:: Unpack Arguments
for %%a in (%*) do set "%%a=1"

set BUILD_DIR=build

:: Clean directory
IF EXIST %BUILD_DIR% rmdir %BUILD_DIR%\ /S /Q

mkdir build

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set C_FLAGS=/nologo /W4 /WX /Zi /GS- /GR- /Gs1000000000 /Fo:%BUILD_DIR%\ /std:c17 /c /Tc
set L_FLAGS=/WX /SUBSYSTEM:CONSOLE /NODEFAULTLIB /stack:0x100000,100000 /DEBUG

if "%release%"=="0" set C_FLAGS=%C_FLAGS% /Od

set SRC_FILE=

:: DBG
set SRC_FILE=main
:: Compile
cl /Fd:%BUILD_DIR%\%SRC_FILE%.pdb %C_FLAGS% src/%SRC_FILE%.c

:: Link
link %L_FLAGS% %BUILD_DIR%\%SRC_FILE%.obj /OUT:%BUILD_DIR%\%SRC_FILE%.exe kernel32.lib

:: Test program
set SRC_FILE=test
:: Compile
cl /Fd:%BUILD_DIR%\test.pdb %C_FLAGS% src/test.c

:: Link
link %L_FLAGS% %BUILD_DIR%\%SRC_FILE%.obj /OUT:%BUILD_DIR%\%SRC_FILE%.exe kernel32.lib
