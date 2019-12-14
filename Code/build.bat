@echo off

set DISABLED_WARNS=/wd4530 /wd4577
SET COMP_OPTS=/FC /Oi /EHa- /Gm- /Zi /GR- /FeJoy /nologo %DISABLED_WARNS%
SET INCLUDE_PATH=/I"C:\VulkanSDK\1.1.114.0\Include"

IF NOT EXIST ..\Build MKDIR ..\Build
PUSHD ..\Build

cl %COMP_OPTS% %INCLUDE_PATH% ..\Code\win32_joy.cpp ..\Code\joy_software_renderer.cpp ..\Code\joy_assets.cpp ..\Code\joy_gui.cpp ..\Code\joy_platform.cpp ..\Code\joy_render_blur.cpp ..\COde\joy_asset_util.cpp ..\COde\joy_colors.cpp ..\Code\joy_opengl.cpp  /link /NOLOGO /INCREMENTAL:no /OPT:ref kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib opengl32.lib

POPD