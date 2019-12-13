#include "win32_joy.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

/*
TODO(Dima):
Assets:
Bake blur

Renderer:
Software 3d renderer

dima privet , kak dela? i tebia lybly
*/

GLOBAL_VARIABLE win32_state Win32;
GLOBAL_VARIABLE b32 GlobalRunning;
GLOBAL_VARIABLE input GlobalInput;
GLOBAL_VARIABLE assets GlobalAssets;
GLOBAL_VARIABLE gui_state GlobalGui;

platform_api PlatformAPI;

PLATFORM_FREE_FILE_MEMORY(Win32FreeFileMemory){
    if (ReadFileResult->Data != 0){
        VirtualFree(ReadFileResult->Data, 0, MEM_RELEASE);
    }
    
    ReadFileResult->Data = 0;
    ReadFileResult->DataSize = 0;
}

PLATFORM_READ_FILE(Win32ReadFile){
    platform_read_file_result Res = {};
    
    HANDLE FileHandle = CreateFileA(
        FilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0, 0);
    
    if (FileHandle != INVALID_HANDLE_VALUE){
        LARGE_INTEGER FileSizeLI;
        if (GetFileSizeEx(FileHandle, &FileSizeLI)){
            u32 FileSize = (FileSizeLI.QuadPart & 0xFFFFFFFF);
            Res.Data = VirtualAlloc(0, FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Res.Data){
                DWORD BytesRead;
                if (ReadFile(FileHandle, Res.Data, FileSize, &BytesRead, 0) && (FileSize == BytesRead)){
                    Res.DataSize = FileSize;
                }
                else{
                    Win32FreeFileMemory(&Res);
                }
            }
            else{
                //TODO(Dima): Logging. Can not allocate memory
            }
        }
        else{
            //TODO(Dima): Logging. Can't get file size
        }
        
        CloseHandle(FileHandle);
    }
    else{
        //TODO(Dima): Logging. Can't open file
    }
    
    return(Res);
}

PLATFORM_WRITE_FILE(Win32WriteFile){
    b32 Result = 0;
    
    HANDLE FileHandle = CreateFileA(FilePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE){
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Data, Size, &BytesWritten, 0)){
            Result = (BytesWritten == Size);
        }
        else{
            //TODO(Dima): Logging
        }
        
        CloseHandle(FileHandle);
    }
    else{
        //TODO(Dima): Logging
    }
    
    
    return(Result);
}

PLATFORM_SHOW_ERROR(Win32ShowError){
    char* CaptionText = "Error";
    u32 MessageBoxType = MB_OK;
    
    switch(Type){
        case PlatformError_Error:{
            CaptionText = "Error";
            MessageBoxType |= MB_ICONERROR;
        }break;
        
        case PlatformError_Warning:{
            CaptionText = "Warning";
            MessageBoxType |= MB_ICONWARNING;
        }break;
        
        case PlatformError_Information:{
            CaptionText = "Information";
            MessageBoxType |= MB_ICONINFORMATION;
        }break;
    }
    
    MessageBoxA(0, Text, CaptionText, MessageBoxType);
}

PLATFORM_DEBUG_OUTPUT_STRING(Win32DebugOutputString){
    OutputDebugString(Text);
}

INTERNAL_FUNCTION void
Win32ToggleFullscreen(win32_state* Win32)
{
    DWORD Style = GetWindowLong(Win32->Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetWindowPlacement(Win32->Window, &Win32->WindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(Win32->Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Win32->Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Win32->Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Win32->Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Win32->Window, &Win32->WindowPlacement);
        SetWindowPos(Win32->Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

inline void Win32ProcessKey(key_state* Key, b32 IsDown){
    if(Key->EndedDown != IsDown){
        Key->EndedDown = IsDown;
        
        Key->TransitionHappened = 1;
    }
}

INTERNAL_FUNCTION void 
Win32ProcessMessages(input* Input){
    MSG Msg;
    while(PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE)){
        switch(Msg.message){
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKey = (u32)Msg.wParam;
                b32 WasDown = ((Msg.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Msg.lParam & (1 << 31)) == 0);
                b32 AltKeyWasDown = ((Msg.lParam & (1 << 29)) != 0);
                
                //NOTE(dima): If state of key was changed
                if(WasDown != IsDown){
                    u32 KeyType;
                    switch(VKey){
                        case VK_LBUTTON: { KeyType = KeyMouse_Left; }break;
                        case VK_RBUTTON: { KeyType = KeyMouse_Right; }break;
                        case VK_MBUTTON: { KeyType = KeyMouse_Middle; }break;
                        case VK_XBUTTON1: { KeyType = KeyMouse_X1; }break;
                        case VK_XBUTTON2: { KeyType = KeyMouse_X2; }break;
                        case VK_LEFT: { KeyType = Key_Left; }break;
                        case VK_RIGHT: { KeyType = Key_Right; }break;
                        case VK_UP: { KeyType = Key_Up; }break;
                        case VK_DOWN: { KeyType = Key_Down; }break;
                        case VK_BACK: { KeyType = Key_Backspace; }break;
                        case VK_TAB: { KeyType = Key_Tab; }break;
                        case VK_RETURN: { KeyType = Key_Return; }break;
                        case VK_SHIFT: { KeyType = Key_Shift; }break;
                        case VK_CONTROL: { KeyType = Key_Control; }break;
                        case VK_ESCAPE: { KeyType= Key_Escape; }break;
                        case VK_SPACE: { KeyType = Key_Space; }break;
                        case VK_HOME: { KeyType = Key_Home; }break;
                        case VK_END: { KeyType = Key_End; }break;
                        case VK_INSERT: { KeyType = Key_Insert; }break;
                        case VK_DELETE: { KeyType = Key_Delete; }break;
                        case VK_HELP: { KeyType = Key_Help; }break;
                        
                        case 0x30:{ KeyType = Key_0; }break;
                        case 0x31:{ KeyType = Key_1; }break;
                        case 0x32:{ KeyType = Key_2; }break;
                        case 0x33:{ KeyType = Key_3; }break;
                        case 0x34:{ KeyType = Key_4; }break;
                        case 0x35:{ KeyType = Key_5; }break;
                        case 0x36:{ KeyType = Key_6; }break;
                        case 0x37:{ KeyType = Key_7; }break;
                        case 0x38:{ KeyType = Key_8; }break;
                        case 0x39:{ KeyType = Key_9; }
                        
                        case 'A':{ KeyType = Key_A; }break;
                        case 'B':{ KeyType = Key_B; }break;
                        case 'C':{ KeyType = Key_C; }break;
                        case 'D':{ KeyType = Key_D; }break;
                        case 'E':{ KeyType = Key_E; }break;
                        case 'F':{ KeyType = Key_F; }break;
                        case 'G':{ KeyType = Key_G; }break;
                        case 'H':{ KeyType = Key_H; }break;
                        case 'I':{ KeyType = Key_I; }break;
                        case 'J':{ KeyType = Key_J; }break;
                        case 'K':{ KeyType = Key_K; }break;
                        case 'L':{ KeyType = Key_L; }break;
                        case 'M':{ KeyType = Key_M; }break;
                        case 'N':{ KeyType = Key_N; }break;
                        case 'O':{ KeyType = Key_O; }break;
                        case 'P':{ KeyType = Key_P; }break;
                        case 'Q':{ KeyType = Key_Q; }break;
                        case 'R':{ KeyType = Key_R; }break;
                        case 'S':{ KeyType = Key_S; }break;
                        case 'T':{ KeyType = Key_T; }break;
                        case 'U':{ KeyType = Key_U; }break;
                        case 'V':{ KeyType = Key_V; }break;
                        case 'W':{ KeyType = Key_W; }break;
                        case 'X':{ KeyType = Key_X; }break;
                        case 'Y':{ KeyType = Key_Y; }break;
                        case 'Z':{ KeyType = Key_Z; }break;
                        
                        case VK_NUMPAD0: { KeyType = Key_Num0; }break;
                        case VK_NUMPAD1: { KeyType = Key_Num1; }break;
                        case VK_NUMPAD2: { KeyType = Key_Num2; }break;
                        case VK_NUMPAD3: { KeyType = Key_Num3; }break;
                        case VK_NUMPAD4: { KeyType = Key_Num4; }break;
                        case VK_NUMPAD5: { KeyType = Key_Num5; }break;
                        case VK_NUMPAD6: { KeyType = Key_Num6; }break;
                        case VK_NUMPAD7: { KeyType = Key_Num7; }break;
                        case VK_NUMPAD8: { KeyType = Key_Num8; }break;
                        case VK_NUMPAD9: { KeyType = Key_Num9; }break;
                        case VK_MULTIPLY: { KeyType = Key_Multiply; }break;
                        case VK_ADD: { KeyType = Key_Add; }break;
                        case VK_DIVIDE: { KeyType = Key_Divide; }break;
                        case VK_SUBTRACT: { KeyType = Key_Subtract; }break;
                        case VK_SEPARATOR: { KeyType = Key_Separator; }break;
                        case VK_DECIMAL: { KeyType = Key_Decimal; }break;
                        case VK_F1: {  KeyType = Key_F1; }break;
                        case VK_F2: {  KeyType = Key_F2; }break;
                        case VK_F3: {  KeyType = Key_F3; }break;
                        case VK_F4: {  KeyType = Key_F4; }break;
                        case VK_F5: {  KeyType = Key_F5; }break;
                        case VK_F6: {  KeyType = Key_F6; }break;
                        case VK_F7: {  KeyType = Key_F7; }break;
                        case VK_F8: {  KeyType = Key_F8; }break;
                        case VK_F9: {  KeyType = Key_F9; }break;
                        case VK_F10: {  KeyType = Key_F10; }break;
                        case VK_F11: {  KeyType = Key_F11; }break;
                        case VK_F12: {  KeyType = Key_F12; }break;
                        case VK_VOLUME_MUTE: { KeyType = Key_VolumeMute; }break;
                        case VK_VOLUME_UP: { KeyType = Key_VolumeUp; }break;
                        case VK_VOLUME_DOWN: { KeyType = Key_VolumeDown; }break;
                        
                        Win32ProcessKey(&Input->KeyStates[KeyType], IsDown);
                    }
                    
                    if(IsDown){
                        
                        if(AltKeyWasDown && VKey == VK_F4){
                            GlobalRunning = 0;
                        }
                        
                        if(AltKeyWasDown && VKey == VK_RETURN){
                            Win32ToggleFullscreen(&Win32);
                        }
                    }
                }
            }break;
            
            case WM_LBUTTONDOWN:{
                
            }break;
            
            case WM_QUIT:{
                GlobalRunning = 0;
            }break;
            
            case WM_CLOSE:{
                PostQuitMessage(0);
                GlobalRunning = 0;
            }break;
            
            case WM_DESTROY:{
                PostQuitMessage(0);
                GlobalRunning = 0;
            }break;
            
            default:{
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }break;
        } //END SWITCH
    } //END_WHILE
}

INTERNAL_FUNCTION void
Win32ProcessInput(input* Input)
{
    POINT Point;
    BOOL GetCursorPosRes = GetCursorPos(&Point);
    BOOL ScreenToClientRes = ScreenToClient(Win32.Window, &Point);
    
    v2 MouseP = V2(Point.x, Point.y);
    Input->LastMouseP = Input->MouseP;
    Input->MouseP = MouseP;
    
    //NOTE(Dima): Processing mouse buttons
    DWORD Win32MouseKeyID[] = {
        VK_LBUTTON,
        VK_MBUTTON,
        VK_RBUTTON,
        VK_XBUTTON1,
        VK_XBUTTON2,
    };
    
    for(u32 MouseKeyIndex = 0;
        MouseKeyIndex < ARRAY_COUNT(Win32MouseKeyID);
        MouseKeyIndex++)
    {
        Input->KeyStates[MouseKey_Left + MouseKeyIndex].TransitionHappened = 0;
        SHORT WinMouseKeyState = GetKeyState(Win32MouseKeyID[MouseKeyIndex]);
        
        Win32ProcessKey(&Input->KeyStates[MouseKey_Left + MouseKeyIndex], WinMouseKeyState & (1 << 15));
    }
}

INTERNAL_FUNCTION void
Win32DisplayBitmapInWindow(
win32_state* Win,
int WindowWidth,
int WindowHeight)
{
    HDC DC = GetDC(Win->Window);
    
    DWORD Style = GetWindowLong(Win->Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        StretchDIBits(
            DC,
            0, 0, WindowWidth, WindowHeight,
            0, 0, Win->Bitmap.Width, Win->Bitmap.Height,
            Win->Bitmap.Pixels, &Win->BMI,
            DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        GetMonitorInfo(MonitorFromWindow(Win->Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo);;
        StretchDIBits(
            DC,
            MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
            MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
            MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
            0, 0, Win->Bitmap.Width, Win->Bitmap.Height,
            Win->Bitmap.Pixels, &Win->BMI,
            DIB_RGB_COLORS, SRCCOPY);
    }
    
    ReleaseDC(Win->Window, DC);
}

LRESULT CALLBACK
Win32WindowProcessing(
HWND Window,
UINT Message,
WPARAM WParam,
LPARAM LParam)
{
    switch (Message){
        
        case WM_SIZE:{
            
        }break;
        
        case WM_DESTROY:{
            GlobalRunning = 0;
        }break;
        
        case WM_QUIT:{
            GlobalRunning = 0;
        }break;
        
        case WM_CLOSE:{
            GlobalRunning = 0;
        }break;
        
        default:{
            return DefWindowProc(Window, Message, WParam, LParam);
        }break;
    }
    
    return(0);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    QueryPerformanceFrequency(&Win32.PerformanceFreqLI);
    Win32.OneOverPerformanceFreq = 1.0f / (float)Win32.PerformanceFreqLI.QuadPart;
    
    u32 MemoryBlockSize = Gibibytes(1);
    void* MemoryBlock = VirtualAlloc(
        0, 
        MemoryBlockSize,
        MEM_COMMIT | MEM_RESERVE, 
        PAGE_READWRITE);
    memory_region GlobalMem = InitMemoryRegion(MemoryBlock, MemoryBlockSize);
    
    WNDCLASSEXA WindowClass = {};
    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.lpszClassName = "MainWindowClassName";
    WindowClass.hInstance = hInstance;
    WindowClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32WindowProcessing;
    
    RegisterClassExA(&WindowClass);
    
    int WindowWidth = 1366;
    WindowWidth = (WindowWidth + 3) & (~3);
    int WindowHeight = 768;
    
    int WindowCreateW;
    int WindowCreateH;
    
    RECT ClientRect;
    ClientRect.left = 0;
    ClientRect.top = 0;
    ClientRect.right = WindowWidth;
    ClientRect.bottom = WindowHeight;
    BOOL WindowRectAdjusted = AdjustWindowRect(
        &ClientRect, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & (~WS_OVERLAPPED), 0);
    
    if(WindowRectAdjusted){
        WindowCreateW = ClientRect.right - ClientRect.left;
        WindowCreateH = ClientRect.bottom - ClientRect.top;
    }
    else{
        WindowCreateW = WindowWidth;
        WindowCreateH = WindowHeight;
    }
    
    Win32.Window = CreateWindowA(
        WindowClass.lpszClassName,
        "Joy",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WindowCreateW,
        WindowCreateH,
        0, 0, 
        hInstance, 
        0);
    
    Win32.WindowWidth = WindowWidth;
    Win32.WindowHeight = WindowHeight;
    
    void* Win32BitmapMemory = PushSomeMem(&GlobalMem, WindowWidth * WindowHeight * 4, 64);
    Win32.Bitmap = AssetAllocateBitmapInternal(WindowWidth, WindowHeight, Win32BitmapMemory);
    Win32.BMI = {};
    BITMAPINFO* BMI = &Win32.BMI;
    BITMAPINFOHEADER* BMIHeader = &BMI->bmiHeader;
    BMIHeader->biSize=  sizeof(BITMAPINFOHEADER);
    BMIHeader->biWidth = WindowWidth;
    BMIHeader->biHeight = -WindowHeight;
    BMIHeader->biPlanes = 1;
    BMIHeader->biBitCount = 32;
    BMIHeader->biCompression = BI_RGB;
    BMIHeader->biSizeImage = 0;
    
#if USE_VULKAN
    Win32InitVulkan(&Win32.Vulkan, &GlobalMem);
#endif
    
    // NOTE(Dima): Initializing platform API
    InitDefaultPlatformAPI(&PlatformAPI);
    
    PlatformAPI.ReadFile = Win32ReadFile;
    PlatformAPI.WriteFile = Win32WriteFile;
    PlatformAPI.FreeFileMemory = Win32FreeFileMemory;
    PlatformAPI.ShowError = Win32ShowError;
    PlatformAPI.OutputString = Win32DebugOutputString;
    
    // NOTE(Dima): Initializing engine systems
    InitAssets(&GlobalAssets);
    render_stack RenderStack_ = InitRenderStack(&GlobalMem, Megabytes(1));
    render_stack* RenderStack = &RenderStack_;
    InitGui(
        &GlobalGui, 
        &GlobalInput, 
        &GlobalAssets, 
        &GlobalMem, 
        RenderStack, 
        WindowWidth, 
        WindowHeight);
    
    float Time = 0.0f;
    float DeltaTime = 0.016f;
    
    gui_layout* Lay = GetFirstLayout(&GlobalGui);
    
    //ShellExecuteA(NULL, "open", "http://www.microsoft.com", NULL, NULL, SW_SHOWNORMAL);
    
    GlobalRunning = 1;
    while(GlobalRunning){
        LARGE_INTEGER BeginClockLI;
        QueryPerformanceCounter(&BeginClockLI);
        
        Win32ProcessMessages(&GlobalInput);
        Win32ProcessInput(&GlobalInput);
        
        rc2 ClipRect = RcMinMax(V2(0.0f, 0.0f), V2(WindowWidth, WindowHeight));
        
        RenderStackBeginFrame(RenderStack);
        
        PushClearColor(RenderStack, V3(1.0f, 0.5f, 0.0f));
        
        PushBitmap(RenderStack, &GlobalAssets.Sunset, V2(100.0f, Sin(Time * 1.0f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.SunsetOrange, V2(200.0f, Sin(Time * 1.1f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.SunsetField, V2(300.0f, Sin(Time * 1.2f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.SunsetMountains, V2(400.0f, Sin(Time * 1.3f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.MountainsFuji, V2(500.0f, Sin(Time * 1.4f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.RoadClouds, V2(600.0f, Sin(Time * 1.5f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.Sunrise, V2(700.0f, Sin(Time * 1.6f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        
        char FPSBuf[64];
        stbsp_sprintf(FPSBuf, "FPS %.2f, ms %.3f", 1.0f / DeltaTime, DeltaTime);
        
        static int LastFrameEntryCount = 0;
        static int LastFrameBytesUsed = 0;
        char StackInfo[256];
        stbsp_sprintf(StackInfo, "EntryCount: %d; BytesUsed: %d;", 
                      LastFrameEntryCount, 
                      LastFrameBytesUsed);
        
#if 1
        gui_state* Gui = &GlobalGui;
        
        GuiBeginPage(Gui, "Page1");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "Page2");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "Animation");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "Platform");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "GUI");
        GuiEndPage(Gui);
        
        //GuiUpdateWindows(Gui);
        
        GuiBeginLayout(Gui, Lay);
        GuiText(Gui, "Hello world");
        GuiText(Gui, FPSBuf);
        GuiText(Gui, StackInfo);
        GuiText(Gui, "I love Kate");
        GuiText(Gui, "I wish joy and happiness for everyone");
        
        LOCAL_AS_GLOBAL int RectCount = 0;
        GuiBeginRow(Gui);
        if(GuiButton(Gui, "Add")){
            RectCount++;
        }
        if(GuiButton(Gui, "Clear")){
            RectCount--;
            if(RectCount < 0){
                RectCount = 0;
            }
        }
        GuiEndRow(Gui);
        for(int i = 0; i < RectCount; i++){
            PushRect(RenderStack, RcMinDim(V2(100 + i * 50, 100), V2(40, 40)));
        }
        
        static b32 BoolButtonValue;
        GuiBeginRow(Gui);
        GuiBoolButton(Gui, "BoolButton", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButton123", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButton1234", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButtonasdfga", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButtonzxcvzxcb", &BoolButtonValue);
        GuiEndRow(Gui);
        
        static b32 BoolButtonOnOffValue;
        GuiBeginRow(Gui);
        GuiBeginColumn(Gui);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &BoolButtonValue);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff1", &BoolButtonValue);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff2", &BoolButtonValue);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff3", &BoolButtonValue);
        GuiEndColumn(Gui);
        
        GuiBeginColumn(Gui);
        static b32 CheckboxValue1;
        static b32 CheckboxValue2;
        static b32 CheckboxValue3;
        static b32 CheckboxValue4;
        GuiCheckbox(Gui, "Checkbox", &CheckboxValue1);
        GuiCheckbox(Gui, "Checkbox1", &CheckboxValue2);
        GuiCheckbox(Gui, "Checkbox2", &CheckboxValue3);
        GuiCheckbox(Gui, "Checkbox3", &CheckboxValue4);
        GuiEndColumn(Gui);
        GuiEndRow(Gui);
        
        GuiTooltip(Gui, "Hello world!", GlobalInput.MouseP);
        
        GuiPreRender(Gui);
        
        GuiEndLayout(&GlobalGui, Lay);
#endif
        
        LastFrameBytesUsed = RenderStack->Data.Used;
        LastFrameEntryCount = RenderStack->EntryCount;
        
        RenderMultithreaded(&PlatformAPI.HighPriorityQueue, RenderStack, &Win32.Bitmap);
        RenderMultithreadedRGBA2BGRA(&PlatformAPI.HighPriorityQueue, &Win32.Bitmap);
        
        Win32DisplayBitmapInWindow(&Win32, WindowWidth, WindowHeight);
        
        LARGE_INTEGER EndClockLI;
        QueryPerformanceCounter(&EndClockLI);
        u64 ClocksElapsed = EndClockLI.QuadPart - BeginClockLI.QuadPart;
        DeltaTime = (float)ClocksElapsed * Win32.OneOverPerformanceFreq;
        Time += DeltaTime;
    }
    
    //NOTE(dima): Cleanup
#if USE_VULKAN
    Win32CleanupVulkan(&Win32.Vulkan);
#endif
    
    FreePlatformAPI(&PlatformAPI);
    DestroyWindow(Win32.Window);
    VirtualFree(MemoryBlock, 0, MEM_RELEASE);
    
    return (0);
}