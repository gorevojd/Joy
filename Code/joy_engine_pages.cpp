#include "joy_engine_pages.h"

void InitEnginePages(engine_pages* Pages){
    DescribeEnginePage(Pages, EnginePage_Render, "Render", RenderGUIElementCallback);
    DescribeEnginePage(Pages, EnginePage_Audio, "Audio", AudioGUIElementCallback);
    DescribeEnginePage(Pages, EnginePage_Gui, "GUI", GUIGUIElementCallback);
    DescribeEnginePage(Pages, EnginePage_Memory, "Memory", MemoryGUIElementCallback);
    DescribeEnginePage(Pages, EnginePage_Platform, "Platform", PlatformGUIElementCallback);
    DescribeEnginePage(Pages, EnginePage_Profile, "Profile", ProfileGUIElementCallback);
    DescribeEnginePage(Pages, EnginePage_Test, "Test", TestGUIElementCallback);
}