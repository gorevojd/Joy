#include "joy_gui.h"
#include "joy_defines.h"

// NOTE(Dima): Returns newly pushed window array
INTERNAL_FUNCTION gui_window* GuiGrowWindowFreePool(gui_state* Gui, memory_region* Mem,  int Count){
    gui_window* WindowFreePoolArray = PushArray(Mem, gui_window, Count);
    
    for(int Index = 0; 
        Index < Count;
        Index++)
    {
        gui_window* Window = WindowFreePoolArray + Index;
        
        *Window = {};
        
        Window->PrevAlloc = Gui->WindowFreeSentinel.PrevAlloc;
        Window->NextAlloc = &Gui->WindowFreeSentinel;
        
        Window->PrevAlloc->NextAlloc = Window;
        Window->NextAlloc->PrevAlloc = Window;
    }
    
    return(WindowFreePoolArray);
}

inline gui_window* GuiPopFromReturnList(gui_state* Gui){
    gui_window* Result = Gui->WindowSentinel4Returning.Next;
    
    // NOTE(Dima): deleting from return list
    Result->Next->Prev = Result->Prev;
    Result->Prev->Next = Result->Next;
    
    Result->Next = 0;
    Result->Prev = 0;
    
    return(Result);
}

inline void GuiDeallocateWindow(gui_state* Gui, gui_window* Todo){
    Todo->NextAlloc->PrevAlloc = Todo->PrevAlloc;
    Todo->PrevAlloc->NextAlloc = Todo->NextAlloc;
    
    Todo->NextAlloc = Gui->WindowFreeSentinel.NextAlloc;
    Todo->PrevAlloc = &Gui->WindowFreeSentinel;
    
    Todo->NextAlloc->PrevAlloc = Todo;
    Todo->PrevAlloc->NextAlloc = Todo;
}

// NOTE(Dima): This function allocates as much windows as we need and 
// NOTE(Dima): then adds them to return list. It returns first element
// NOTE(Dima): of that list
INTERNAL_FUNCTION gui_window* GuiAllocateWindows(gui_state* Gui, int Count)
{
    // NOTE(Dima): If free list is emty then allocate some more to it
    b32 CanAllocateArray = 1;
    int CanAllocateCount = Count;
    gui_window* CheckAt = Gui->WindowFreeSentinel.NextAlloc;
    for(int CheckIndex = 0; CheckIndex < Count; CheckIndex++){
        if(CheckAt == &Gui->WindowFreeSentinel){
            CanAllocateArray = 0;
            CanAllocateCount = CheckIndex;
            break;
        }
        
        CheckAt = CheckAt->NextAlloc;
    }
    
    int ToAllocateCount = Max(128, Count - CanAllocateCount);
    if(!CanAllocateArray){
        GuiGrowWindowFreePool(Gui, Gui->Mem, ToAllocateCount);
    }
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(Gui->WindowSentinel4Returning.Next == &Gui->WindowSentinel4Returning);
    
    for(int AddIndex = 0;
        AddIndex < Count;
        AddIndex++)
    {
        // NOTE(Dima): Before in this algo we ensured that we would
        // NOTE(Dima): have as mush elements as we need. But for sure
        // NOTE(Dima): I'll double check if we can grab one more element.
        Assert(Gui->WindowFreeSentinel.NextAlloc != &Gui->WindowFreeSentinel);
        
        // NOTE(Dima): Allocating from free list
        gui_window* AddWindow = Gui->WindowFreeSentinel.NextAlloc;
        
        AddWindow->PrevAlloc->NextAlloc = AddWindow->NextAlloc;
        AddWindow->NextAlloc->PrevAlloc = AddWindow->PrevAlloc;
        
        // NOTE(Dima): Inserting to use list
        AddWindow->NextAlloc = &Gui->WindowUseSentinel;
        AddWindow->PrevAlloc = Gui->WindowUseSentinel.PrevAlloc;
        
        AddWindow->NextAlloc->PrevAlloc = AddWindow;
        AddWindow->PrevAlloc->NextAlloc = AddWindow;
        
        // NOTE(Dima): Inserting to return list
        AddWindow->Next = &Gui->WindowSentinel4Returning;
        AddWindow->Prev = Gui->WindowSentinel4Returning.Prev;
        
        AddWindow->Next->Prev = AddWindow;
        AddWindow->Prev->Next = AddWindow;
    }
    
    gui_window* Result = Gui->WindowSentinel4Returning.Next;
    
    return(Result);
}

INTERNAL_FUNCTION gui_window* GuiAllocateWindow(gui_state* Gui){
    GuiAllocateWindows(Gui, 1);
    
    gui_window* Result = GuiPopFromReturnList(Gui);
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(Gui->WindowSentinel4Returning.Next == &Gui->WindowSentinel4Returning);
    
    return(Result);
}

inline void GuiAddWindowToList(gui_window* Window, 
                               gui_window* Sentinel)
{
    Window->Next = Sentinel;
    Window->Prev = Sentinel->Prev;
    
    Window->Next->Prev = Window;
    Window->Prev->Next = Window;
}


void InitGui(
gui_state* Gui, 
input* Input, 
assets* Assets, 
memory_region* Mem, 
render_stack* Stack,
int Width,
int Height)
{
    Gui->MainFont = &Assets->InconsolataBold;
    Gui->CheckboxMark = &Assets->CheckboxMark;
    Gui->FontScale = 1.0f;
    
    Gui->Stack = Stack;
    Gui->Input = Input;
    Gui->Mem = Mem;
    
    Gui->Layout = {};
    Gui->CurrentLayout = 0;
    
    Gui->Width = Width;
    Gui->Height = Height;
    
    // NOTE(Dima): Initializing root page 
    CopyStrings(Gui->RootPage.Name, "Root");
    Gui->RootPage.Next = &Gui->RootPage;
    Gui->RootPage.Prev = &Gui->RootPage;
    
    // NOTE(Dima): Initializing of window free pool and sentinels
    Gui->WindowUseSentinel.NextAlloc = &Gui->WindowUseSentinel;
    Gui->WindowUseSentinel.PrevAlloc = &Gui->WindowUseSentinel;
    Gui->WindowFreeSentinel.NextAlloc = &Gui->WindowFreeSentinel;
    Gui->WindowFreeSentinel.PrevAlloc = &Gui->WindowFreeSentinel;
    
    GuiGrowWindowFreePool(Gui, Mem, 128);
    
    // NOTE(Dima): Init window sentinel for returning windows
    // NOTE(Dima): as list when we allocate multiple of them.
    Gui->WindowSentinel4Returning.Next = &Gui->WindowSentinel4Returning;
    Gui->WindowSentinel4Returning.Prev = &Gui->WindowSentinel4Returning;
    
    // NOTE(Dima): Init window leaf sentinel
    Gui->WindowLeafSentinel.Next = &Gui->WindowLeafSentinel;
    Gui->WindowLeafSentinel.Prev = &Gui->WindowLeafSentinel;
    
    Gui->TempWindow1 = GuiAllocateWindow(Gui);
    Gui->TempWindow1->Rect = RcMinDim(V2(10, 10), V2(1000, 600));
    GuiAddWindowToList(Gui->TempWindow1, &Gui->WindowLeafSentinel);
    
    // NOTE(Dima): Initializing colors
    InitColorsState(&Gui->ColorState, Mem);
    Gui->Colors[GuiColor_Text] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->Colors[GuiColor_HotText] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->Colors[GuiColor_Borders] = GUI_GETCOLOR_COLSYS(Color_Black);
    
    Gui->Colors[GuiColor_ButtonBackground] = GUI_COLORHEX("#337733");
    Gui->Colors[GuiColor_ButtonBackgroundHot] = GUI_GETCOLOR_COLSYS(Color_Cyan);
    Gui->Colors[GuiColor_ButtonForeground] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->Colors[GuiColor_ButtonForegroundHot] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->Colors[GuiColor_ButtonForegroundDisabled] = Gui->Colors[GuiColor_ButtonForeground] * 0.75f;
    
    Gui->WindowAlpha = 0.85f;
    Gui->Colors[GuiColor_WindowBackground] = V4(0.0f, 0.0f, 0.0f, Gui->WindowAlpha);
    Gui->Colors[GuiColor_WindowBorder] = GUI_GETCOLOR_COLSYS(Color_Black);
    Gui->Colors[GuiColor_WindowBorderHot] = GUI_GETCOLOR_COLSYS(Color_Magenta);
    Gui->Colors[GuiColor_WindowBorderActive] = GUI_GETCOLOR_COLSYS(Color_Blue);
}

rc2 PrintTextInternal(font_info* FontInfo, render_stack* Stack, char* Text, v2 P, u32 TextOp, float Scale, v4 Color){
    rc2 TextRect;
    
    char* At = Text;
    
    v2 CurrentP = P;
    
    while(*At){
        
        int GlyphIndex = FontInfo->Codepoint2Glyph[*At];
        glyph_info* Glyph = &FontInfo->Glyphs[GlyphIndex];
        
        float BitmapScale = Glyph->Height * Scale;
        
        v2 BitmapDim = { Glyph->Bitmap.WidthOverHeight * BitmapScale, BitmapScale };
        
        if(TextOp == PrintTextOp_Print){
            float BitmapMinY = CurrentP.y + Glyph->YOffset * Scale;
            float BitmapMinX = CurrentP.x + Glyph->XOffset * Scale;
            
            PushBitmap(Stack, &Glyph->Bitmap, V2(BitmapMinX, BitmapMinY), BitmapDim.y, Color);
        }
        
        CurrentP.x += Glyph->Advance;
        
        *At++;
    }
    
    TextRect.Min.x = P.x;
	TextRect.Min.y = P.y - FontInfo->AscenderHeight * Scale;
	TextRect.Max.x = CurrentP.x;
	TextRect.Max.y = CurrentP.y - FontInfo->DescenderHeight * Scale;
    
	return(TextRect);
}

v2 GetTextSizeInternal(font_info* FontInfo, char* Text, float Scale){
    rc2 TextRc = PrintTextInternal(
        FontInfo, 
        0, 
        Text, V2(0.0f, 0.0f), 
        PrintTextOp_GetSize, 
        Scale);
    
    v2 Result = GetRectDim(TextRc);
    
    return(Result);
}

inline v2 GetCenteredTextOffset(font_info* FontInfo, float TextDimX, rc2 Rect, float Scale = 1.0f){
    float LineDimY = GetLineAdvance(FontInfo, Scale);
    float LineDelta = FontInfo->AscenderHeight * Scale - LineDimY * 0.5f;
    
    v2 CenterRc = Rect.Min + GetRectDim(Rect) * 0.5f;
    
    v2 TargetP = V2(CenterRc.x - TextDimX * 0.5f, CenterRc.y + LineDelta);
    return(TargetP);
}

inline float GetCenteredTextOffsetY(font_info* FontInfo, rc2 Rect, float Scale = 1.0f){
    float Result = GetCenteredTextOffset(FontInfo, 0.0f, Rect, Scale).y;
    
    return(Result);
}

inline v2 GetCenteredTextOffset(font_info* FontInfo, char* Text, rc2 Rect, float Scale = 1.0f){
    v2 TextDim = GetTextSizeInternal(FontInfo, Text, Scale);
    
    v2 Result = GetCenteredTextOffset(FontInfo, TextDim.x, Rect, Scale);
    
    return(Result);
}

rc2 PrintTextCenteredInRectInternal(
font_info* FontInfo, 
render_stack* Stack, 
char* Text, 
rc2 Rect, 
float Scale, 
v4 Color)
{
    v2 TargetP = GetCenteredTextOffset(FontInfo, Text, Rect, Scale);
    rc2 Result = PrintTextInternal(FontInfo, Stack, Text, TargetP, PrintTextOp_Print, Scale, Color);
    
    return(Result);
}

v2 GetTextSize(gui_state* Gui, char* Text, float Scale){
    v2 Result = GetTextSizeInternal(Gui->MainFont, 
                                    Text, 
                                    Gui->FontScale * Scale);
    
    return(Result);
}

rc2 GetTextRect(gui_state* Gui, char* Text, v2 P, float Scale){
    rc2 TextRc = PrintTextInternal(
        Gui->MainFont, 
        Gui->Stack, 
        Text, P, 
        PrintTextOp_GetSize, 
        Gui->FontScale * Scale);
    
    return(TextRc);
}

rc2 PrintText(gui_state* Gui, char* Text, v2 P, v4 Color, float Scale){
    rc2 TextRc = PrintTextInternal(
        Gui->MainFont, 
        Gui->Stack, 
        Text, P, 
        PrintTextOp_Print, 
        Gui->FontScale * Scale,
        Color);
    
    return(TextRc);
}

rc2 PrintTextCenteredInRect(gui_state* Gui, char* Text, rc2 Rect, float Scale, v4 Color){
    rc2 Result = PrintTextCenteredInRectInternal(Gui->MainFont,
                                                 Gui->Stack,
                                                 Text, 
                                                 Rect,
                                                 Gui->FontScale * Scale,
                                                 Color);
    
    return(Result);
}

void GuiBeginLayout(gui_state* Gui, gui_layout* Layout){
    Layout->At = V2(0.0f, 0.0f);
    
    Gui->CurrentLayout = Layout;
}

void GuiEndLayout(gui_state* Gui, gui_layout* Layout){
    Layout->At = V2(0.0f, 0.0f);
    
    Gui->CurrentLayout = 0;
}

void GuiBeginPage(gui_state* Gui, char* Name){
    u32 NameID = StringHashFNV(Name);
    
    gui_page* FoundPage = 0;
    gui_page* PageAt = Gui->RootPage.Next;
    for(PageAt; PageAt != &Gui->RootPage; PageAt = PageAt->Next){
        if(NameID == PageAt->ID){
            FoundPage = PageAt;
            break;
        }
    }
    
    if(!FoundPage){
        FoundPage = PushStruct(Gui->Mem, gui_page);
        
        CopyStrings(FoundPage->Name, sizeof(FoundPage->Name), Name);
        FoundPage->ID = NameID;
        
        FoundPage->Next = Gui->RootPage.Next;
        FoundPage->Prev = &Gui->RootPage;
        FoundPage->Next->Prev = FoundPage;
        FoundPage->Prev->Next = FoundPage;
    }
    
    // NOTE(Dima): Can't start new page inside other page
    Assert(!Gui->CurrentPage);
    Gui->CurrentPage = FoundPage;
}

void GuiEndPage(gui_state* Gui){
    Gui->CurrentPage = 0;
}

struct gui_snap_in_window_result{
    union{
        struct{
            rc2 Result;
            rc2 RestRect;
        };
        rc2 Rects[2];
    };
};

gui_snap_in_window_result GuiSnapInWindowRect(rc2 WindowRect, u32 SnapType){
    gui_snap_in_window_result Result;
    
    v2 Min = WindowRect.Min;
    v2 Max = WindowRect.Max;
    
    v2 WindowDim = GetRectDim(WindowRect);
    v2 WindowHalfDim = WindowDim * 0.5f;
    
    switch(SnapType){
        case GuiWindowSnap_Left:{
            Result.Result = RcMinDim(Min, 
                                     V2(WindowHalfDim.x, WindowDim.y));
            Result.RestRect = RcMinDim(V2(Min.x + WindowHalfDim.x, Min.y),
                                       V2(WindowHalfDim.x, WindowDim.y));
        }break;
        
        case GuiWindowSnap_Right:{
            Result.Result = RcMinDim(V2(Min.x + WindowHalfDim.x, Min.y),
                                     V2(WindowHalfDim.x, WindowDim.y));
            Result.RestRect =  RcMinDim(Min, 
                                        V2(WindowHalfDim.x, WindowDim.y));
        }break;
        
        case GuiWindowSnap_Top:{
            Result.Result = RcMinDim(Min, V2(WindowDim.x, WindowHalfDim.y));
            Result.RestRect = RcMinDim(V2(Min.x, Min.y + WindowHalfDim.y), V2(WindowDim.x, WindowHalfDim.y));
        }break;
        
        case GuiWindowSnap_Bottom:{
            Result.Result = RcMinDim(V2(Min.x, Min.y + WindowHalfDim.y), V2(WindowDim.x, WindowHalfDim.y));
            Result.RestRect = RcMinDim(Min, V2(WindowDim.x, WindowHalfDim.y));;
        }break;
        
        case GuiWindowSnap_Whole:{
            Result.Result = WindowRect;
            Result.RestRect = {};
        }break;
        
        default:{
            INVALID_CODE_PATH;
        }break;
    }
    
    return(Result);
}

INTERNAL_FUNCTION void GuiSplitWindow(gui_state* Gui, 
                                      gui_window* Window, 
                                      int PartsCount, 
                                      rc2* PartsRects)
{
    GuiAllocateWindows(Gui, PartsCount);
    
    for(int NewWindowIndex = 0;
        NewWindowIndex < PartsCount;
        NewWindowIndex++)
    {
        gui_window* NewWindow = GuiPopFromReturnList(Gui);
        
        // NOTE(Dima): Adding children to leafs
        NewWindow->Next = Gui->WindowLeafSentinel.Next;
        NewWindow->Prev = &Gui->WindowLeafSentinel;
        
        NewWindow->Next->Prev = NewWindow;
        NewWindow->Prev->Next = NewWindow;
        
        NewWindow->Rect = PartsRects[NewWindowIndex];
    }
    
    // NOTE(Dima): Deallocating parent because it is not visible
    Window->Next->Prev = Window->Prev;
    Window->Prev->Next = Window->Next;
    
    Window->Next = 0;
    Window->Prev = 0;
    
    GuiDeallocateWindow(Gui, Window);
    
    // NOTE(Dima): Return list shoud be empty after usage in this function
    Assert(Gui->WindowSentinel4Returning.Next == &Gui->WindowSentinel4Returning);
}

INTERNAL_FUNCTION void GuiUpdateWindow(gui_state* Gui, gui_window* Window){
    rc2 WindowRc = Window->Rect;
    PushRect(Gui->Stack, WindowRc, GUI_GETCOLOR(GuiColor_WindowBackground));
    
    v4 OutlineColor = GUI_GETCOLOR(GuiColor_WindowBorder);
    if(MouseInRect(Gui->Input, WindowRc)){
        OutlineColor = GUI_GETCOLOR(GuiColor_WindowBorderHot);
        
        // NOTE(Dima): Processing snapping
        v2 WindowDim = GetRectDim(WindowRc);
        v2 WindowHalfDim = WindowDim * 0.5f;
        v2 WindowCenter = WindowRc.Min + WindowHalfDim;
        
        v2 MouseP = Gui->Input->MouseP;
        MouseP = ClampInRect(MouseP, WindowRc);
        v2 DiffFromCenter = MouseP - WindowCenter;
        v2 DiffRelative;
        DiffRelative.x = DiffFromCenter.x / WindowHalfDim.x;
        DiffRelative.y = DiffFromCenter.y / WindowHalfDim.y;
        v2 AbsDiff = DiffRelative;
        AbsDiff.x = Abs(AbsDiff.x);
        AbsDiff.y = Abs(AbsDiff.y);
        
        float MaxAbsDiff = 0.35f;
        u32 SnapType = GuiWindowSnap_Whole;
        if(DiffRelative.x < 0){
            if(AbsDiff.x > MaxAbsDiff){
                MaxAbsDiff = AbsDiff.x;
                SnapType = GuiWindowSnap_Left;
            }
        }
        else{
            if(AbsDiff.x > MaxAbsDiff){
                MaxAbsDiff = AbsDiff.x;
                SnapType = GuiWindowSnap_Right;
            }
        }
        
        if(DiffRelative.y < 0){
            if(AbsDiff.y > MaxAbsDiff){
                MaxAbsDiff = AbsDiff.y;
                SnapType = GuiWindowSnap_Top;
            }
        }
        else{
            if(AbsDiff.y > MaxAbsDiff){
                MaxAbsDiff = AbsDiff.y;
                SnapType = GuiWindowSnap_Bottom;
            }
        }
        
        // NOTE(Dima): Pushing snap rectangle
        if(SnapType != GuiWindowSnap_Whole){
            gui_snap_in_window_result SnapRes = GuiSnapInWindowRect(WindowRc, SnapType);
            v4 SnapColor = V4(1.0f, 0.0f, 1.0f, 0.4f);
            PushRect(Gui->Stack, SnapRes.Result, SnapColor);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSplitWindow(Gui, Window, 2, SnapRes.Rects);
            }
        }
    }
    
    // NOTE(Dima): Pushing inner outline
    PushRectInnerOutline(Gui->Stack, WindowRc, 1, OutlineColor);
    
}

void GuiUpdateWindows(gui_state* Gui){
    gui_window* UpdateAt = Gui->WindowLeafSentinel.Next;
    while(UpdateAt != &Gui->WindowLeafSentinel){
        gui_window* TempNext = UpdateAt->Next;
        
        GuiUpdateWindow(Gui, UpdateAt);
        
        UpdateAt = TempNext;
    }
}

// NOTE(Dima): Default advance type is Column advance
inline void GuiPreAdvance(gui_state* Gui, gui_layout* Layout){
    gui_advance_ctx* Ctx = &Layout->AdvanceRememberStack[Layout->StackCurrentIndex];
    b32 RowStarted = Ctx->Type == GuiAdvanceType_Row;
    
    float RememberValue = Ctx->RememberValue;
    
    if(RowStarted){
        Layout->At.y = Ctx->Baseline;
    }
    else{
        Layout->At.x = Ctx->Baseline;
        Layout->At.y += GuiGetBaseline(Gui);
    }
}

inline void GuiPostAdvance(gui_state* Gui, gui_layout* Layout, rc2 ElementRect){
    gui_advance_ctx* Ctx = &Layout->AdvanceRememberStack[Layout->StackCurrentIndex];
    b32 RowStarted = (Ctx->Type == GuiAdvanceType_Row);
    
    float RememberValue = Ctx->RememberValue;
    
    float ToX = ElementRect.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f;
    float ToY = ElementRect.Max.y + GetLineAdvance(Gui->MainFont, Gui->FontScale) * 0.2f;
    
    if(RowStarted){
        Layout->At.x = ToX;
        Ctx->Maximum = Max(Ctx->Maximum, ToY);
    }
    else{
        Layout->At.y = ToY;
        Ctx->Maximum = Max(Ctx->Maximum, ToX);
    }
}


inline gui_advance_ctx GuiRowAdvanceCtx(float RememberX, float Baseline){
    gui_advance_ctx Ctx = {};
    
    Ctx.Type = GuiAdvanceType_Row;
    Ctx.RememberValue = RememberX;
    Ctx.Baseline = Baseline;
    
    return(Ctx);
}

inline gui_advance_ctx GuiColumnAdvanceCtx(float RememberY, float Baseline){
    gui_advance_ctx Ctx = {};
    
    Ctx.Type = GuiAdvanceType_Column;
    Ctx.RememberValue = RememberY;
    Ctx.Baseline = Baseline;
    
    return(Ctx);
}

void GuiBeginRow(gui_state* Gui){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    Assert(Layout->StackCurrentIndex < ArrayCount(Layout->AdvanceRememberStack));
    
    Layout->AdvanceRememberStack[++Layout->StackCurrentIndex] = GuiRowAdvanceCtx(
        Layout->At.x, Layout->At.y + GuiGetBaseline(Gui));
}


void GuiBeginColumn(gui_state* Gui){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    Assert(Layout->StackCurrentIndex < ArrayCount(Layout->AdvanceRememberStack));
    
    Layout->AdvanceRememberStack[++Layout->StackCurrentIndex] = GuiColumnAdvanceCtx(
        Layout->At.y,
        Layout->At.x);
}

void GuiEndRow(gui_state* Gui){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    Assert(Layout->StackCurrentIndex >= 1);
    
    gui_advance_ctx* Ctx = &Layout->AdvanceRememberStack[Layout->StackCurrentIndex--];
    Assert(Ctx->Type == GuiAdvanceType_Row);
    
    Layout->At.x = Ctx->RememberValue;
    Layout->At.y = Ctx->Maximum;
}

void GuiEndColumn(gui_state* Gui){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    Assert(Layout->StackCurrentIndex >= 1);
    
    gui_advance_ctx* Ctx = &Layout->AdvanceRememberStack[Layout->StackCurrentIndex--];
    Assert(Ctx->Type == GuiAdvanceType_Column);
    
    Layout->At.y = Ctx->RememberValue;
    Layout->At.x = Ctx->Maximum;
}

void GuiPreRender(gui_state* Gui){
    for(int TooltipIndex = 0; TooltipIndex < GUI_MAX_TOOLTIPS; TooltipIndex++){
        gui_tooltip* Tooltip = &Gui->Tooltips[TooltipIndex];
        
        PrintText(Gui, Tooltip->Text, Tooltip->At, GUI_GETCOLOR(GuiColor_Text), 1.0f);
    }
    Gui->TooltipIndex = 0;
}

void GuiTooltip(gui_state* Gui, char* TooltipText, v2 At){
    Assert(Gui->TooltipIndex < GUI_MAX_TOOLTIPS);
    gui_tooltip* Tooltip = &Gui->Tooltips[Gui->TooltipIndex++];
    
    CopyStrings(Tooltip->Text, GUI_TOOLTIP_MAX_SIZE, TooltipText);
    Tooltip->At = At;
}

void GuiText(gui_state* Gui, char* Text){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    GuiPreAdvance(Gui, Layout);
    
    rc2 TextRc = PrintText(Gui, Text, Layout->At, GUI_GETCOLOR(GuiColor_Text));
    
    GuiPostAdvance(Gui, Layout, TextRc);
}

b32 GuiButton(gui_state* Gui, char* ButtonName){
    b32 Result = 0;
    
    gui_layout* Layout = GetFirstLayout(Gui);
    
    GuiPreAdvance(Gui, Layout);
    
    // NOTE(Dima): Printing button and text
    rc2 TextRc = GetTextRect(Gui, ButtonName, Layout->At);
    rc2 TempTextRc = GrowRectByScaledValue(TextRc, V2(4.0f, 2.0f), Gui->FontScale);
    TextRc.Max = TextRc.Min + GetRectDim(TempTextRc);
    PushRect(Gui->Stack, TextRc, GUI_GETCOLOR(GuiColor_ButtonBackground));
    PushRectOutline(Gui->Stack, TextRc, 1, GUI_GETCOLOR(GuiColor_Borders));
    
    // NOTE(Dima): Event processing
    v4 TextColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
    if(MouseInRect(Gui->Input, TextRc)){
        TextColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
        
        if(KeyWentDown(Gui->Input, MouseKey_Left)){
            Result = 1;
        }
    }
    
    PrintTextCenteredInRect(Gui, ButtonName, TextRc, Gui->FontScale, TextColor);
    
    GuiPostAdvance(Gui, Layout, TextRc);
    
    return(Result);
}

void GuiBoolButton(gui_state* Gui, char* ButtonName, b32* Value){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    GuiPreAdvance(Gui, Layout);
    
    // NOTE(Dima): Printing button and text
    rc2 TextRc = GetTextRect(Gui, ButtonName, Layout->At);
    rc2 TempTextRc = GrowRectByScaledValue(TextRc, V2(4.0f, 2.0f), Gui->FontScale);
    TextRc.Max = TextRc.Min + GetRectDim(TempTextRc);
    PushRect(Gui->Stack, TextRc, GUI_GETCOLOR(GuiColor_ButtonBackground));
    PushRectOutline(Gui->Stack, TextRc, 1, GUI_GETCOLOR(GuiColor_Borders));
    
    v4 TextColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
    
    // NOTE(Dima): Event processing
    if(Value){
        if(*Value == 0){
            TextColor = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
        }
        
        if(MouseInRect(Gui->Input, TextRc)){
            TextColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                *Value = !*Value;
            }
        }
    }
    
    PrintTextCenteredInRect(Gui, ButtonName, TextRc, Gui->FontScale, TextColor);
    
    GuiPostAdvance(Gui, Layout, TextRc);
}

void GuiBoolButtonOnOff(gui_state* Gui, char* ButtonName, b32* Value){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    GuiPreAdvance(Gui, Layout);
    
    // NOTE(Dima): Button printing
    rc2 ButRc = GetTextRect(Gui, "OFF", Layout->At);
    v2 ButTextDim = GetRectDim(ButRc);
    rc2 TempButRc = GrowRectByScaledValue(ButRc, V2(4.0f, 2.0f), Gui->FontScale);
    v2 TempButRcDim = GetRectDim(TempButRc);
    ButRc.Max = ButRc.Min + TempButRcDim;
    PushRect(Gui->Stack, ButRc, GUI_GETCOLOR(GuiColor_ButtonBackground));
    PushRectOutline(Gui->Stack, ButRc, 1, GUI_GETCOLOR(GuiColor_Borders));
    
    // NOTE(Dima): Button name text printing
    float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, ButRc, Gui->FontScale);
    v2 NameStart = V2(ButRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
    rc2 NameRc = PrintText(Gui, ButtonName, NameStart, GUI_GETCOLOR(GuiColor_Text));
    
    // NOTE(Dima): Event processing
    char ButtonText[4];
    CopyStrings(ButtonText, "ERR");
    v4 ButtonTextC = GUI_GETCOLOR(GuiColor_ButtonForeground);
    if(Value){
        if(*Value){
            CopyStrings(ButtonText, "ON");
        }
        else{
            ButtonTextC = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
            
            CopyStrings(ButtonText, "OFF");
        }
        
        if(MouseInRect(Gui->Input, ButRc)){
            ButtonTextC = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                *Value = !*Value;
            }
        }
    }
    
    PrintTextCenteredInRect(Gui, ButtonText, ButRc, Gui->FontScale, ButtonTextC);
    
    rc2 AdvanceRect = GetBoundingRect(ButRc, NameRc);
    GuiPostAdvance(Gui, Layout, AdvanceRect);
}

void GuiCheckbox(gui_state* Gui, char* Name, b32* Value){
    gui_layout* Layout = GetFirstLayout(Gui);
    
    GuiPreAdvance(Gui, Layout);
    
    // NOTE(Dima): Checkbox rendering
    float CheckboxSize = GetLineAdvance(Gui->MainFont, Gui->FontScale);
    rc2 CheckRect;
    CheckRect.Min = V2(Layout->At.x, Layout->At.y - GetScaledAscender(Gui->MainFont, Gui->FontScale));
    CheckRect.Max = CheckRect.Min + V2(CheckboxSize, CheckboxSize);
    rc2 TempButRc = GrowRectByScaledValue(CheckRect, V2(2.0f, 2.0f), Gui->FontScale);
    CheckRect.Max = CheckRect.Min + GetRectDim(TempButRc);
    
    // NOTE(Dima): Event processing
    v4 BackColor = GUI_GETCOLOR(GuiColor_ButtonBackground);
    if(Value){
        if(MouseInRect(Gui->Input, CheckRect)){
            BackColor = GUI_GETCOLOR(GuiColor_ButtonBackgroundHot);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                *Value = !*Value;
            }
        }
    }
    
    PushRect(Gui->Stack, CheckRect, BackColor);
    
    if(*Value){
        PushBitmap(Gui->Stack, Gui->CheckboxMark, CheckRect.Min, GetRectHeight(CheckRect), V4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    
    PushRectOutline(Gui->Stack, CheckRect, 1, GUI_GETCOLOR(GuiColor_Borders));
    
    // NOTE(Dima): Button name text printing
    float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, CheckRect, Gui->FontScale);
    v2 NameStart = V2(CheckRect.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
    rc2 NameRc = PrintText(Gui, Name, NameStart, GUI_GETCOLOR(GuiColor_Text));
    
    rc2 AdvanceRect = GetBoundingRect(CheckRect, NameRc);
    GuiPostAdvance(Gui, Layout, AdvanceRect);
}