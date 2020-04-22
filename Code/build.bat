@echo off

set DISABLED_WARNS=/wd4530 /wd4577 /wd4005
SET COMP_OPTS=/FC /Oi /MP /EHa- /Gm- /Zi /GR- /nologo %DISABLED_WARNS%
SET INCLUDE_PATH=/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"

IF NOT EXIST ..\Build MKDIR ..\Build
PUSHD ..\Build

ECHO ____________________
ECHO ********************
ECHO *   AssetBuilder   *
ECHO ********************

set ASSBUILD_COMPFILES=..\Code\tool_asset_build.cpp
set ASSBUILD_COMPFILES=%ASSBUILD_COMPFILES% ..\Code\tool_asset_build_loading.cpp
set ASSBUILD_COMPFILES=%ASSBUILD_COMPFILES% ..\Code\tool_asset_build_commands.cpp

SET THIS_PROJECT_NAME=AssetBuilder
cl /Fe%THIS_PROJECT_NAME% %COMP_OPTS% %ASSBUILD_COMPFILES% /link /NOLOGO /INCREMENTAL:no /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib


ECHO ____________________
ECHO ********************
ECHO *   AssimpBuilder  *
ECHO ********************

set ASSIMP_INCLUDE="../../ExternalPrograms/3dParty/assimp-master/Include"
set ASSIMP_LIBPATH="../../ExternalPrograms/3dParty/assimp-master/AssimpBuild/code/Debug"
set ASSBUILD_COMPFILES=..\Code\tool_asset_build_assimp.cpp
set ASSBUILD_COMPFILES=%ASSBUILD_COMPFILES% ..\Code\tool_asset_build_loading.cpp
set ASSBUILD_COMPFILES=%ASSBUILD_COMPFILES% ..\Code\tool_asset_build_commands.cpp

SET THIS_PROJECT_NAME=AssimpBuilder
cl /Fe%THIS_PROJECT_NAME% %COMP_OPTS% /I%ASSIMP_INCLUDE% %ASSBUILD_COMPFILES% /link /LIBPATH:%ASSIMP_LIBPATH% /NOLOGO /INCREMENTAL:no /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib assimp-vc140-mtd.lib

ECHO ___________
ECHO ***********
ECHO *   JOY   *
ECHO ***********

set COMPILATION_FILES=..\Code\win32_joy.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_debug.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_animation.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_sort.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_memory.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_render.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_assets.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_gui.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_platform.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_render_blur.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_software_renderer.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_software_renderer_functions.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_colors.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_opengl.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_audio.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_camera.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_world.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_game_mode.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_modes.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_input.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_engine.cpp

SET THIS_PROJECT_NAME=Joy
ECHO Compiling Joy
cl /Fe%THIS_PROJECT_NAME% %COMP_OPTS% %INCLUDE_PATH% %COMPILATION_FILES% /link /NOLOGO /INCREMENTAL:no /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib opengl32.lib dsound.lib Xinput.lib

POPD