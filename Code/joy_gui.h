#ifndef JOY_GUI_H
#define JOY_GUI_H

#include "joy_input.h"
#include "joy_assets.h"
#include "joy_math.h"
#include "joy_render_stack.h"
#include "joy_strings.h"
#include "joy_colors.h"

inline float GetBaseline(Font_Info* fontInfo, float scale = 1.0f){
    float res = (fontInfo->ascenderHeight + fontInfo->lineGap) * scale;
    
    return(res);
}

inline float GetLineAdvance(Font_Info* fontInfo, float scale = 1.0f){
    float res = (fontInfo->ascenderHeight + fontInfo->lineGap - fontInfo->descenderHeight) * scale;
    
    return(res);
}

inline float GetScaledAscender(Font_Info* fontInfo, float scale = 1.0f){
    float res = fontInfo->ascenderHeight * scale;
    
    return(res);
}

enum GuiColorType{
    GuiColor_Text,
    GuiColor_HotText,
    GuiColor_Borders,
    
    GuiColor_ButtonBackground,
    GuiColor_ButtonBackgroundHot,
    GuiColor_ButtonForeground,
    GuiColor_ButtonForegroundDisabled,
    GuiColor_ButtonForegroundHot,
    GuiColor_ButtonGrad1,
    GuiColor_ButtonGrad2,
    
    GuiColor_WindowBackground,
    GuiColor_WindowBorder,
    GuiColor_WindowBorderHot,
    GuiColor_WindowBorderActive,
    
    GuiColor_Count,
};

#define GUI_GETCOLOR(color) gui->colors[color]
#define GUI_GETCOLOR_COLSYS(index) gui->colorState.colorTable[index].color
#define GUI_COLORHEX(str) ColorFromHex(str)

struct Gui_Page{
    char name[128];
    u32 id;
    
    Gui_Page* next;
    Gui_Page* prev;
};

struct Gui_Window{
    Gui_Window* nextAlloc;
    Gui_Window* prevAlloc;
    
    Gui_Window* next;
    Gui_Window* prev;
    
    rc2 rect;
};

enum GuiWindowSnapType{
    GuiWindowSnap_Left,
    GuiWindowSnap_Right,
    GuiWindowSnap_Top,
    GuiWindowSnap_Bottom,
    GuiWindowSnap_Whole,
};


enum GuiAdvanceType{
    GuiAdvanceType_Column,
    GuiAdvanceType_Row,
};

struct GuiAdvanceCtx{
    u32 type;
    float rememberValue;
    float baseline;
    float maximum;
    
    float maxHorz;
    float maxVert;
};

struct Gui_Layout{
    v2 start;
    v2 at;
    
    GuiAdvanceCtx advanceRememberStack[16];
    int stackCurrentIndex;
};

#define GUI_TOOLTIP_MAX_SIZE 256
struct Gui_Tooltip{
    char text[GUI_TOOLTIP_MAX_SIZE];
    v2 at;
};

struct Gui_Element{
    char name[64];
    
    u32 id;
    
    Gui_Element* mext;
    Gui_Element* prev;
    
    Gui_Element* nextAlloc;
    Gui_Element* prevAlloc;
};

struct Gui_State{
    Font_Info* mainFont;
    float fontScale;
    
    Render_Stack* stack;
    Input* input;
    Memory_Region* mem;
    
    Gui_Layout layout;
    Gui_Layout* currentLayout;
    Gui_Page rootPage;
    Gui_Page* currentPage;
    
    Gui_Window windowUseSentinel;
    Gui_Window windowFreeSentinel;
    Gui_Window windowSentinel4Returning;
    Gui_Window windowLeafSentinel;
    
    Gui_Window* tempWindow1;
    Gui_Window* tempWindow2;
    
    Gui_Element freeSentinel;
    Gui_Element useSentinel;
    
    int width;
    int height;
    
#define GUI_MAX_TOOLTIPS 256
    Gui_Tooltip tooltips[GUI_MAX_TOOLTIPS];
    int tooltipIndex;
    
    Bmp_Info* checkboxMark;
    
    Color_State colorState;
    v4 colors[GuiColor_Count];
    float windowAlpha;
};

inline float GuiGetBaseline(Gui_State* gui, float scale = 1.0f){
    float res = GetBaseline(gui->mainFont, gui->fontScale * scale);
    
    return(res);
}

inline float GuiGetLineAdvance(Gui_State* gui, float scale = 1.0f){
    float res = GetLineAdvance(gui->mainFont, gui->fontScale * scale);
    
    return(res);
}

//TODO(Dima): Delete this
inline Gui_Layout* GetFirstLayout(Gui_State* gui){
    Gui_Layout* res = &gui->layout;
    
    return(res);
}

enum print_text_operation{
    PrintTextOp_Print,
    PrintTextOp_GetSize,
};

rc2 PrintTextInternal(Font_Info* fontInfo, Render_Stack* stack, char* text, v2 P, u32 textOp, float scale = 1.0f, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f));
v2 GetTextSizeInternal(Font_Info* fontInfo, char* text, float scale);
rc2 PrintTextCenteredInRectInternal(Font_Info* fontInfo, Render_Stack* stack, char* text, rc2 rect, float scale = 1.0f, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f));

v2 GetTextSize(Gui_State* gui, char* Text, float scale = 1.0f);
rc2 GetTextRect(Gui_State* gui, char* Text, v2 p, float scale = 1.0f);
rc2 PrintText(Gui_State* gui, char* text, v2 P, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), float scale = 1.0f);
rc2 PrintTextCenteredInRect(Gui_State* gui, char* text, rc2 tect, float scale = 1.0f, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f));

void GuiUpdateWindows(Gui_State* gui);

void InitGui(
Gui_State* gui, 
Input* input, 
Assets* assets, 
Memory_Region* mem, 
Render_Stack* stack,
int width,
int height);
void GuiBeginLayout(Gui_State* gui, Gui_Layout* layout);
void GuiEndLayout(Gui_State* gui, Gui_Layout* layout);

void GuiBeginPage(Gui_State* gui, char* name);
void GuiEndPage(Gui_State* gui);

void GuiBeginRow(Gui_State* gui);
void GuiEndRow(Gui_State* gui);
void GuiBeginColumn(Gui_State* gui);
void GuiEndColumn(Gui_State* gui);

void GuiPreRender(Gui_State* gui);

void GuiTooltip(Gui_State* gui, char* tooltipText, v2 at);
void GuiText(Gui_State* gui, char* text);
b32 GuiButton(Gui_State* gui, char* buttonName);
void GuiBoolButton(Gui_State* gui, char* buttonName, b32* value);
void GuiBoolButtonOnOff(Gui_State* gui, char* buttonName, b32* value);
void GuiCheckbox(Gui_State* gui, char* name, b32* value);

#endif