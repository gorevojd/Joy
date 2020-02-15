@echo off

set DISABLED_WARNS=/wd4530 /wd4577 /wd4005
SET COMP_OPTS=/FC /Oi /EHa- /Gm- /Zi /GR- /FeJoy /nologo %DISABLED_WARNS%
SET INCLUDE_PATH=/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"

IF NOT EXIST ..\Build MKDIR ..\Build
PUSHD ..\Build

set COMPILATION_FILES=..\Code\win32_joy.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_sort.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_memory.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_render.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_assets.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_gui.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_platform.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_render_blur.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_software_renderer.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_asset_util.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_colors.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_opengl.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_audio.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_camera.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_world.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_game.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_modes.cpp
set COMPILATION_FILES=%COMPILATION_FILES% ..\Code\joy_input.cpp

cl %COMP_OPTS% %INCLUDE_PATH% %COMPILATION_FILES% /link /NOLOGO /INCREMENTAL:no /LIBPATH:"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64" /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib opengl32.lib dsound.lib Xinput.lib

POPD