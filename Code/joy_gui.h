#ifndef JOY_GUI_H
#define JOY_GUI_H

#include "joy_input.h"
#include "joy_assets.h"
#include "joy_math.h"
#include "joy_render.h"
#include "joy_strings.h"
#include "joy_colors.h"

#define JOY_GUI_TRUE 1
#define JOY_GUI_FALSE 0

/*
NOTE(dima):

1) Gui layouts structured next way. Each element is Gui_Element structure.
It can have some children and parents elements. The tree of elements is a
Gui_Page. It's done this way because of comfortability to group the elements
*/

inline float GetBaseline(Font_Info* fontInfo, float scale = 1.0f){
    float res = (fontInfo->AscenderHeight + fontInfo->LineGap) * scale;
    
    return(res);
}

inline float GetLineAdvance(Font_Info* fontInfo, float scale = 1.0f){
    float res = (fontInfo->AscenderHeight + fontInfo->LineGap - fontInfo->DescenderHeight) * scale;
    
    return(res);
}

inline float GetScaledAscender(Font_Info* fontInfo, float scale = 1.0f){
    float res = fontInfo->AscenderHeight * scale;
    
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

enum Gui_Layout_Type{
    GuiLayout_Layout,
    GuiLayout_Window,
};

struct Gui_Layout{
    v2 Start;
    v2 At;
    v2 Dim;
    
    char Name[128];
    u32 ID;
    u32 Type;
    
    struct Gui_Element* Elem;
    
    Gui_Layout* Next;
    Gui_Layout* Prev;
    
    GuiAdvanceCtx AdvanceRememberStack[16];
    int StackCurrentIndex;
};

struct Gui_Window{
    Gui_Window* NextAlloc;
    Gui_Window* PrevAlloc;
    
    Gui_Window* Next;
    Gui_Window* Prev;
    
    rc2 rect;
    
    Gui_Layout layout;
    
    b32 visible;
};

enum GuiWindowSnapType{
    GuiWindowSnap_Left,
    GuiWindowSnap_Right,
    GuiWindowSnap_Top,
    GuiWindowSnap_Bottom,
    GuiWindowSnap_Whole,
};

#define GUI_TOOLTIP_MAX_SIZE 256
struct Gui_Tooltip{
    char text[GUI_TOOLTIP_MAX_SIZE];
    v2 at;
};

struct Gui_Page{
    char name[128];
    u32 id;
    
    Gui_Element* elem;
    
    Gui_Page* Next;
    Gui_Page* Prev;
};

enum Gui_Element_Type{
    GuiElement_None,
    GuiElement_Root,
    GuiElement_Page,
    GuiElement_ChildrenSentinel,
    GuiElement_TreeLink,
    GuiElement_Item,
    GuiElement_RowColumn,
    GuiElement_RadioGroup,
    GuiElement_Layout,
};

struct Gui_Element{
    char name[64];
    
    struct {
        union{
            struct{
                u32* ref;
                u32 activeId;
            } radioGroup;
            
            struct {
                Gui_Layout* ref;
            } layout;
            
            struct {
                Gui_Page* ref;
            } page;
            
            struct {
                int CaretPos;
            } inputText;
        };
        
        // NOTE(Dima): is initialized (b32)
        b32 isInit;
    }data;
    
    b32 opened;
    
    Gui_Element* parentInTree;
    
    u32 id;
    u32 type;
    
    Gui_Element* Next;
    Gui_Element* Prev;
    
    Gui_Element* NextAlloc;
    Gui_Element* PrevAlloc;
    
    Gui_Element* parent;
    Gui_Element* childSentinel;
    int childCount;
};

struct Gui_State{
    Font_Info* mainFont;
    float fontScale;
    
    Render_Stack* stack;
    
    Input* input;
    Memory_Region* mem;
    
    Gui_Page rootPage;
    Gui_Page* currentPage;
    int pageCount;
    
    Gui_Layout rootLayout;
    int layoutCount;
    
    Gui_Element* rootElement;
    Gui_Element* curElement;
    
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
    
    Asset_Atlas* atlas;
    Bmp_Info* CheckboxMark;
    
    Color_State colorState;
    v4 colors[GuiColor_Count];
    float windowAlpha;
    
    v2 dimStack[128];
    int dimStackIndex;
    b32 inPushBlock;
};

enum Gui_Interaction_Type{
    GuiInteraction_None,
    GuiInteraction_Variable,
    GuiInteraction_Move,
    GuiInteraction_Resize,
};

struct Gui_Interaction{
    u32 ID;
    u32 InteractionType;
    
    virtual void Interact(Gui_State* gui) = 0;
};

template<typename  t> struct Gui_Variable_Interaction : public Gui_Interaction{
    t* Variable;
};

enum class Gui_Resize_Interaction_Type{
    None,
    Default,
    Proportional,
    Horizontal,
    Vertical,
};

struct Gui_Resize_Interaction : public Gui_Interaction{
    v2* DimensionPtr;
	v2 Position;
	v2 MinDim;
	v2 OffsetInAnchor;
    Gui_Resize_Interaction_Type Type;
    
    Gui_Resize_Interaction(v2 Position, v2* Dimension, Gui_Resize_Interaction_Type Type){
        this->InteractionType = GuiInteraction_Resize;
        
        this->DimensionPtr = DimensionPtr;
        this->Type = Type;
        this->MinDim = V2(0.0f, 0.0f);
        this->OffsetInAnchor = V2(0.0f, 0.0f);
        this->Position = Position;
    }
    
    virtual void Interact(Gui_State* gui) override {
        v2 WorkRectP = this->Position;
        v2 MouseP = gui->input->MouseP - this->OffsetInAnchor;
        
        v2* WorkDim = this->DimensionPtr;
        
        switch (this->Type) {
            case Gui_Resize_Interaction_Type::Default: {
                *WorkDim = MouseP - WorkRectP;
                
                if (MouseP.x - WorkRectP.x < this->MinDim.x) {
                    WorkDim->x = this->MinDim.x;
                }
                
                if (MouseP.y - WorkRectP.y < this->MinDim.y) {
                    WorkDim->y = this->MinDim.y;
                }
            }break;
            
            case Gui_Resize_Interaction_Type::Horizontal: {
                if (MouseP.x - WorkRectP.x < this->MinDim.x) {
                    WorkDim->x = this->MinDim.x;
                }
            }break;
            
            case Gui_Resize_Interaction_Type::Vertical: {
                if (MouseP.y - WorkRectP.y < this->MinDim.y) {
                    WorkDim->y = this->MinDim.y;
                }
            }break;
            
            case Gui_Resize_Interaction_Type::Proportional: {
                float WidthToHeight = WorkDim->x / WorkDim->y;
                WorkDim->y = MouseP.y - WorkRectP.y;
                WorkDim->x = WorkDim->y * WidthToHeight;
                
                if (WorkDim->y < this->MinDim.y) {
                    WorkDim->y = this->MinDim.y;
                    WorkDim->x = WorkDim->y * WidthToHeight;
                }
            }break;
        }
    }
};

enum class Gui_Move_Interaction_Type{
    None,
    Move,
};

struct Gui_Move_Interaction : public Gui_Interaction{
    v2* MovePosition;
    Gui_Move_Interaction_Type Type;
    v2 OffsetInAnchor;
    
    Gui_Move_Interaction(v2* MovePosition, Gui_Move_Interaction_Type Type){
        this->InteractionType = GuiInteraction_Move;
        
        this->Type = Type;
        this->OffsetInAnchor = V2(0.0f, 0.0f);
        this->MovePosition = MovePosition;
    }
    
    virtual void Interact(Gui_State* gui) override {
        v2* WorkP = this->MovePosition;
        v2 MouseP = gui->input->MouseP - this->OffsetInAnchor;
        
        switch (this->Type) {
            case Gui_Move_Interaction_Type::Move: {
                *WorkP = MouseP;
            }break;
        }
    }
};


inline Gui_Element* 
GuiFindElementOfTypeUpInTree(Gui_Element* curElement, u32 elementType) {
    Gui_Element* result = 0;
    
    Gui_Element* at = curElement;
    while (at != 0) {
        if (at->type == elementType) {
            result = at;
            break;
        }
        
        at = at->parent;
    }
    
    return(result);
}


inline Gui_Layout* GetParentLayout(Gui_State* gui){
    Gui_Layout* res = 0;
    
    Gui_Element* layoutElem = GuiFindElementOfTypeUpInTree(
        gui->curElement, 
        GuiElement_Layout);
    
    if(!layoutElem){
        res = &gui->rootLayout;
    }
    else{
        res = layoutElem->data.layout.ref;
    }
    
    return(res);
}

inline void GuiPushDim(Gui_State* gui, v2 dim){
    ASSERT(gui->dimStackIndex < ARRAY_COUNT(gui->dimStack));
    ASSERT(!gui->inPushBlock);
    
    // NOTE(Dima): can't push if inside push block
    if(gui->inPushBlock){
        
    }
    else{
        gui->dimStack[gui->dimStackIndex++] = dim;
    }
}

inline b32 GuiPopDim(Gui_State* gui, v2* outDim){
    b32 result = 0;
    
    int decrementValue = gui->inPushBlock ? 0 : 1;
    
    if(gui->dimStackIndex > 0){
        result = 1;
        
        if(outDim){
            int viewIndex = gui->dimStackIndex - 1;
            *outDim = gui->dimStack[viewIndex];
        }
        gui->dimStackIndex -= decrementValue;
    }
    
    return(result);
}


inline void GuiPushDimBegin(Gui_State* gui, v2 dim){
    gui->dimStack[gui->dimStackIndex++] = dim;
    
    gui->inPushBlock = 1;
}

inline void GuiPushDimEnd(Gui_State* gui){
    gui->inPushBlock = 0;
    
    GuiPopDim(gui, 0);
}

inline rc2 GetTxtElemRect(Gui_State* gui, Gui_Layout* lay, rc2 txtRc, v2 growScale){
    v2 popDim;
    if(GuiPopDim(gui, &popDim)){
        txtRc.max = txtRc.min + popDim;
    }
    else{
        rc2 tempTextRc = GrowRectByScaledValue(txtRc, growScale, gui->fontScale);
        txtRc.max = txtRc.min + GetRectDim(tempTextRc);
    }
    
    return(txtRc);
}

inline float GuiGetBaseline(Gui_State* gui, float scale = 1.0f){
    float res = GetBaseline(gui->mainFont, gui->fontScale * scale);
    
    return(res);
}

inline float GuiGetLineAdvance(Gui_State* gui, float scale = 1.0f){
    float res = GetLineAdvance(gui->mainFont, gui->fontScale * scale);
    
    return(res);
}

enum print_text_operation{
    PrintTextOp_Print,
    PrintTextOp_GetSize,
};

rc2 PrintTextInternal(Font_Info* fontInfo, 
                      Render_Stack* stack, 
                      char* text, 
                      v2 P, 
                      u32 textOp, 
                      float scale = 1.0f, 
                      v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), 
                      int CaretP = 0, 
                      v2* CaretPrintPOut = 0);
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

void GuiFrameBegin(Gui_State* gui);
void GuiFrameEnd(Gui_State* gui);

void GuiBeginLayout(Gui_State* gui, char* name, u32 layoutType);
void GuiEndLayout(Gui_State* gui);

void GuiBeginPage(Gui_State* gui, char* name);
void GuiEndPage(Gui_State* gui);

void GuiBeginRow(Gui_State* gui);
void GuiEndRow(Gui_State* gui);
void GuiBeginColumn(Gui_State* gui);
void GuiEndColumn(Gui_State* gui);

void GuiFramePrepare4Render(Gui_State* gui);

void GuiTooltip(Gui_State* gui, char* tooltipText, v2 at);
void GuiText(Gui_State* gui, char* text);
b32 GuiButton(Gui_State* gui, char* buttonName);
void GuiBoolButton(Gui_State* gui, char* buttonName, b32* value);
void GuiBoolButtonOnOff(Gui_State* gui, char* buttonName, b32* value);
void GuiCheckbox(Gui_State* gui, char* name, b32* value);

void GuiBeginTree(Gui_State* gui, char* name);
void GuiEndTree(Gui_State* gui);

void GuiBeginRadioGroup(
Gui_State* gui, 
char* name, 
u32* ref, 
u32 defaultId);
void GuiRadioButton(Gui_State* gui, char* name, u32 uniqueId);
void GuiEndRadioGroup(Gui_State* gui);

void GuiInputText(Gui_State* gui, char* name, char* Buf, int BufSize);
void GuiSliderFloat(Gui_State* gui, float* Value, float Min, float Max, char* Name);

void GuiTest(Gui_State* gui, float deltaTime);

#endif