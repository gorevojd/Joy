@echo off

set DISABLED_WARNS=/wd4530 /wd4577 /wd4005
SET COMP_OPTS=/FC /Oi /EHa- /Gm- /Zi /GR- /FeJoy /nologo %DISABLED_WARNS%
SET INCLUDE_PATH=/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"


IF NOT EXIST ..\Build MKDIR ..\Build
PUSHD ..\Build

cl %COMP_OPTS% %INCLUDE_PATH% ..\Code\win32_joy.cpp ..\Code\joy_software_renderer.cpp ..\Code\joy_assets.cpp ..\Code\joy_gui.cpp ..\Code\joy_platform.cpp ..\Code\joy_render_blur.cpp ..\COde\joy_asset_util.cpp ..\COde\joy_colors.cpp ..\Code\joy_opengl.cpp ..\Code\joy_dirx.cpp /link /NOLOGO /INCREMENTAL:no /LIBPATH:"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64" /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib opengl32.lib

POPD