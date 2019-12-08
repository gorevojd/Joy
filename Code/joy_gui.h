#ifndef JOY_GUI_H
#define JOY_GUI_H

#include "joy_input.h"
#include "joy_assets.h"
#include "joy_math.h"
#include "joy_render_stack.h"
#include "joy_strings.h"


inline float GetBaseline(font_info* FontInfo, float Scale = 1.0f){
    float Result = (FontInfo->AscenderHeight + FontInfo->LineGap) * Scale;
    
    return(Result);
}

inline float GetLineAdvance(font_info* FontInfo, float Scale = 1.0f){
    float Result = (FontInfo->AscenderHeight + FontInfo->LineGap - FontInfo->DescenderHeight) * Scale;
    
    return(Result);
}

inline float GetScaledAscender(font_info* FontInfo, float Scale = 1.0f){
    float Result = FontInfo->AscenderHeight * Scale;
    
    return(Result);
}

enum gui_color_type{
    GuiColor_Text,
    GuiColor_HotText,
    GuiColor_Borders,
    
    GuiColor_ButtonBackground,
    GuiColor_ButtonBackgroundHot,
    GuiColor_ButtonForeground,
    GuiColor_ButtonForegroundDisabled,
    GuiColor_ButtonForegroundHot,
    
    GuiColor_WindowBackground,
    GuiColor_WindowBorder,
    GuiColor_WindowBorderHot,
    GuiColor_WindowBorderActive,
    
    GuiColor_Count,
};

#define GUI_GETCOLOR(color) Gui->Colors[color]

struct gui_page{
    char Name[128];
    u32 ID;
    
    gui_page* Next;
    gui_page* Prev;
};

struct gui_window{
    gui_window* Parent;
    gui_window* ChildrenSentinel;
    
    gui_window* NextAlloc;
    gui_window* PrevAlloc;
    
    gui_window* NextLeaf;
    gui_window* PrevLeaf;
    
    gui_window* NextBro;
    gui_window* PrevBro;
    
    rc2 Rect;
};

enum gui_window_snap_type{
    GuiWindowSnap_Left,
    GuiWindowSnap_Right,
    GuiWindowSnap_Top,
    GuiWindowSnap_Bottom,
    GuiWindowSnap_Whole,
};

struct gui_element{
    char Name[128];
    u32 ID;
    u32 Type;
    
    gui_element* Next;
    gui_element* Prev;
};

struct gui_layout{
    v2 Start;
    v2 At;
};

struct gui_state{
    font_info* MainFont;
    float FontScale;
    
    render_stack* Stack;
    input* Input;
    memory_region* Mem;
    
    gui_layout Layout;
    gui_layout* CurrentLayout;
    gui_page RootPage;
    gui_page* CurrentPage;
    
    gui_window WindowUseSentinel;
    gui_window WindowFreeSentinel;
    gui_window WindowSentinel4Returning;
    gui_window WindowLeafSentinel;
    
    gui_window* TempWindow1;
    gui_window* TempWindow2;
    
    int Width;
    int Height;
    
    bmp_info* CheckboxMark;
    
    v4 Colors[GuiColor_Count];
    float WindowAlpha;
};

inline float GuiGetBaseline(gui_state* Gui, float Scale = 1.0f){
    float Result = GetBaseline(Gui->MainFont, Gui->FontScale * Scale);
    
    return(Result);
}

inline float GuiGetLineAdvance(gui_state* Gui, float Scale = 1.0f){
    float Result = GetLineAdvance(Gui->MainFont, Gui->FontScale * Scale);
    
    return(Result);
}

//TODO(Dima): Delete this
inline gui_layout* GetFirstLayout(gui_state* Gui){
    gui_layout* Result = &Gui->Layout;
    
    return(Result);
}

enum print_text_operation{
    PrintTextOp_Print,
    PrintTextOp_GetSize,
};

rc2 PrintTextInternal(font_info* FontInfo, render_stack* Stack, char* Text, v2 P, u32 TextOp, float Scale = 1.0f, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f));
v2 GetTextSizeInternal(font_info* FontInfo, char* Text, float Scale);
rc2 PrintTextCenteredInRectInternal(font_info* FontInfo, render_stack* Stack, char* Text, rc2 Rect, float Scale = 1.0f, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f));

v2 GetTextSize(gui_state* Gui, char* Text, float Scale = 1.0f);
rc2 GetTextRect(gui_state* Gui, char* Text, v2 P, float Scale = 1.0f);
rc2 PrintText(gui_state* Gui, char* Text, v2 P, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f), float Scale = 1.0f);
rc2 PrintTextCenteredInRect(gui_state* Gui, char* Text, rc2 Rect, float Scale = 1.0f, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f));

void GuiUpdateWindows(gui_state* Gui);

void InitGui(
gui_state* Gui, 
input* Input, 
assets* Assets, 
memory_region* Mem, 
render_stack* Stack,
int Width,
int Height);
void GuiBeginLayout(gui_state* Gui, gui_layout* Layout);
void GuiEndLayout(gui_state* Gui, gui_layout* Layout);

void GuiBeginPage(gui_state* Gui, char* Name);
void GuiEndPage(gui_state* Gui);

void GuiText(gui_state* Gui, char* Text);
b32 GuiButton(gui_state* Gui, char* ButtonName);
void GuiBoolButton(gui_state* Gui, char* ButtonName, b32* Value);
void GuiBoolButtonOnOff(gui_state* Gui, char* ButtonName, b32* Value);
void GuiCheckbox(gui_state* Gui, char* Name, b32* Value);

#endif