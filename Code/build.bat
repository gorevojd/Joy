@echo off

set DISABLED_WARNS=/wd4530 /wd4577 /wd4005
SET COMP_OPTS=/Od /Zi /Oi /arch:AVX2 /FC /MP /EHa- /Gm- /GR- /nologo %DISABLED_WARNS%
SET INCLUDE_PATH=/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"

IF NOT EXIST ..\Build MKDIR ..\Build
PUSHD ..\Build

ECHO ___________
ECHO ***********
ECHO *   JOY   *
ECHO ***********

REM set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy.cpp
set COMPILATION_FILES=..\Code\win32_joy.cpp

SET THIS_PROJECT_NAME=Joy
ECHO Compiling Joy
cl /Fe%THIS_PROJECT_NAME% %COMP_OPTS% %INCLUDE_PATH% %COMPILATION_FILES% /link /NOLOGO /INCREMENTAL:no /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib opengl32.lib dsound.lib Xinput.lib

ECHO ____________________
ECHO ********************
ECHO *   AssetBuilder   *
ECHO ********************

set ASSBUILD_COMPFILES=..\Code\tool_asset_build.cpp

SET THIS_PROJECT_NAME=AssetBuilder
cl /Fe%THIS_PROJECT_NAME% %COMP_OPTS% %ASSBUILD_COMPFILES% /link /NOLOGO /INCREMENTAL:no /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib

ECHO ____________________
ECHO ********************
ECHO *   AssimpBuilder  *
ECHO ********************

set ASSIMP_INCLUDE="../../ExternalPrograms/3dParty/assimp-master/Include"
set ASSIMP_LIBPATH="../../ExternalPrograms/3dParty/assimp-master/AssimpBuild/code/Release/"
set ASSBUILD_COMPFILES=..\Code\assimp_assets.cpp

SET THIS_PROJECT_NAME=AssimpBuilder
cl /Fe%THIS_PROJECT_NAME% %COMP_OPTS% /I%ASSIMP_INCLUDE% %ASSBUILD_COMPFILES% /link /LIBPATH:%ASSIMP_LIBPATH% /NOLOGO /INCREMENTAL:no /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib assimp-vc140-mt.lib


POPD