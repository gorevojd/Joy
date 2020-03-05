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

inline float GetBaseline(font_info* FontInfo, float Scale = 1.0f){
    float Res = 0.0f;
    
    if(FontInfo){
        Res = (FontInfo->AscenderHeight + FontInfo->LineGap) * Scale;
    }
    
    return(Res);
}

inline float GetLineAdvance(font_info* FontInfo, float Scale = 1.0f){
    float Res = 0.0f; 
    
    if(FontInfo){
        Res = (FontInfo->AscenderHeight + 
               FontInfo->LineGap - 
               FontInfo->DescenderHeight) * Scale;
    }
    
    return(Res);
}

inline float GetScaledAscender(font_info* FontInfo, float Scale = 1.0f){
    float Res = 0.0f;
    
    if(FontInfo){
        Res = FontInfo->AscenderHeight * Scale;
    }
    
    return(Res);
}

enum GuiColorType{
    GuiColor_Text,
    GuiColor_HotText,
    GuiColor_Borders,
    
    GuiColor_Hot,
    GuiColor_Active,
    
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

#define GUI_GETCOLOR(color) Gui->colors[color]
#define GUI_GETCOLOR_COLSYS(index) Gui->colorState.colorTable[index].color
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
    rc2 Rect;
    
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
    GuiWindowSnap_CenterHalf,
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

enum gui_grid_item_type{
    GuiGridItem_GridHub,
    GuiGridItem_Grid,
    GuiGridItem_Row,
    GuiGridItem_Column,
    GuiGridItem_Item,
};

struct gui_grid_item{
    rc2 InternalRect;
    rc2 Rect;
    u32 Type;
    
    float SumWeightInChildren;
    float LastSumWeightInChildren;
    float TimeSinceNotHot;
    
    u32 ActiveID;
    u32 NextActiveID;
    
    b32 IsInit;
};

enum Gui_Element_Type{
    GuiElement_None,
    
    GuiElement_Root,
    GuiElement_Page,
    GuiElement_ChildrenSentinel,
    GuiElement_Item,
    GuiElement_TempItem,
    GuiElement_RowColumn,
    GuiElement_RadioGroup,
    GuiElement_Layout,
    
    GuiElement_GridItem,
};

struct Gui_Element{
    char name[64];
    
    struct {
        union{
            struct{
                u32* ref;
                u32 activeId;
            } RadioGroup;
            
            struct {
                Gui_Layout* ref;
            } Layout;
            
            struct {
                Gui_Page* ref;
            } Page;
            
            struct {
                int CaretPos;
            } InputText;
            
            struct {
                v2 OffsetInAnchor;
            } Anchor;
            
            gui_grid_item GridItem;
        };
        
        // NOTE(Dima): is initialized (b32)
        b32 IsInit;
    }Data;
    
    b32 opened;
    int TmpCount;
    
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



enum Gui_Interaction_Type{
    GuiInteraction_None,
    GuiInteraction_Variable,
    GuiInteraction_Move,
    GuiInteraction_Resize,
    GuiInteraction_BoolInRect,
    GuiInteraction_Empty,
    GuiInteraction_ChangeGrid,
};

enum gui_priority_type{
    GUI_PRIORITY_SUPER_SMALL,
    GUI_PRIORITY_SMALL,
    GUI_PRIORITY_AVG,
    GUI_PRIORITY_HIGH,
    GUI_PRIORITY_SUPER_HIGH,
    GUI_PRIORITY_SUPER_PUPER_HIGH,
};

struct gui_interaction_ctx{
    u32 ID;
    char* Name;
    u32 Priority;
};

struct gui_interaction{
    Gui_Element* Owner;
    u32 InteractionType;
    
    gui_interaction_ctx Context;
    
    union{
        v2 OffsetInAnchor;
    };
    
    b32 WasHotInInteraction;
    b32 WasActiveInInteraction;
    
    gui_interaction(u32 InteractionType, Gui_Element* Owner){
        this->InteractionType = InteractionType;
        this->Owner = Owner;
        this->Context.ID = Owner->id;
        this->Context.Name = Owner->name;
        this->Context.Priority = GUI_PRIORITY_SMALL;
        this->WasHotInInteraction = 0;
        this->WasActiveInInteraction = 0;
    }
    
    void SetPriority(u32 Priority){
        this->Context.Priority = Priority;
    }
    
    static gui_interaction_ctx NullInteraction(){
        gui_interaction_ctx Result = {};
        
        return(Result);
    }
    
    virtual void Interact(struct gui_state* Gui) = 0;
};

struct gui_frame_info{
    render_stack* Stack;
    input_state* Input;
    int Width;
    int Height;
    float DeltaTime;
};

struct gui_state{
    assets* Assets;
    
    ASSET_TYPED_ID(font_info) MainFontID;
    ASSET_TYPED_ID(font_info) TileFontID;
    ASSET_TYPED_ID(bmp_info) CheckboxMarkID;
    ASSET_TYPED_ID(bmp_info) ChamomileID;
    
    font_info* MainFont;
    font_info* TileFont;
    
    float fontScale;
    
    gui_frame_info FrameInfo;
    
    render_stack* Stack;
    input_state* Input;
    int Width;
    int Height;
    
    mem_region* Mem;
    
    gui_interaction_ctx HotInteraction;
    gui_interaction_ctx ActiveInteraction;
    
    Gui_Page rootPage;
    Gui_Page* currentPage;
    int pageCount;
    
    Gui_Layout rootLayout;
    int layoutCount;
    
    Gui_Element* rootElement;
    Gui_Element* curElement;
    
    Gui_Element* CurrentGridHub;
    
    Gui_Window windowUseSentinel;
    Gui_Window windowFreeSentinel;
    Gui_Window windowSentinel4Returning;
    Gui_Window windowLeafSentinel;
    
    Gui_Window* tempWindow1;
    Gui_Window* tempWindow2;
    
    int TotalAllocatedGuiElements;
    Gui_Element freeSentinel;
    Gui_Element useSentinel;
    
#define GUI_MAX_TOOLTIPS 256
    Gui_Tooltip tooltips[GUI_MAX_TOOLTIPS];
    int tooltipIndex;
    
    Asset_Atlas* atlas;
    
    Color_State colorState;
    v4 colors[GuiColor_Count];
    float windowAlpha;
    
    v2 dimStack[128];
    int dimStackIndex;
    b32 inPushBlock;
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


inline Gui_Layout* GetParentLayout(gui_state* Gui){
    Gui_Layout* res = 0;
    
    Gui_Element* layoutElem = GuiFindElementOfTypeUpInTree(
        Gui->curElement, 
        GuiElement_Layout);
    
    if(!layoutElem){
        res = &Gui->rootLayout;
    }
    else{
        res = layoutElem->Data.Layout.ref;
    }
    
    return(res);
}

inline rc2 GetParentLayoutClipRect(gui_state* Gui){
    rc2 Result = GetParentLayout(Gui)->Rect;
    
    return(Result);
}

inline b32 MouseInInteractiveArea(gui_state* Gui, rc2 InteractRect){
    b32 InRect = MouseInRect(Gui->Input, InteractRect);
    b32 InClip = MouseInRect(Gui->Input, GetParentLayoutClipRect(Gui));
    
    b32 Result = InRect && InClip;
    
    return(Result);
}


inline b32 GuiIsHot(gui_state* Gui,
                    gui_interaction* interaction)
{
    b32 Result = Gui->HotInteraction.ID == interaction->Context.ID;
    
    return(Result);
}

inline void GuiSetHot(gui_state* Gui, 
                      gui_interaction* Interaction,
                      b32 ToSet)
{
    ASSERT(Interaction);
    
    if(ToSet){
        if(Interaction->Context.Priority >= Gui->HotInteraction.Priority)
        {
            Gui->HotInteraction = Interaction->Context;
        }
    }
    else{
        if(Gui->HotInteraction.ID == Interaction->Context.ID){
            Gui->HotInteraction = gui_interaction::NullInteraction();
        }
    }
}


inline b32 GuiIsActive(gui_state* Gui, gui_interaction* interaction){
    b32 Result = Gui->ActiveInteraction.ID == interaction->Context.ID;
    
    return(Result);
}

inline void GuiSetActive(gui_state* Gui, gui_interaction* interaction){
    if(GuiIsHot(Gui, interaction) && 
       interaction->Context.Priority >= Gui->ActiveInteraction.Priority)
    {
        Gui->ActiveInteraction = interaction->Context;
        GuiSetHot(Gui, interaction, JOY_FALSE);
    }
}

inline void GuiReleaseInteraction(gui_state* Gui, gui_interaction* interaction){
    if(interaction){
        if(GuiIsActive(Gui, interaction)){
            Gui->ActiveInteraction = gui_interaction::NullInteraction();
        }
    }
}


struct Gui_Empty_Interaction : public gui_interaction{
    Gui_Empty_Interaction(Gui_Element* Owner) : gui_interaction(GuiInteraction_Empty, Owner)
    {
        
    }
    
    virtual void Interact(gui_state* Gui) override {
        this->WasHotInInteraction = 0;
        this->WasActiveInInteraction = 0;
    }
};

inline void GuiGoToGrid(gui_state* Gui, char* GridName){
    Gui_Element* Parent = Gui->CurrentGridHub;
    ASSERT(Parent);
    // NOTE(Dima): Can call only when grid hub is current
    ASSERT(Parent->Data.GridItem.Type == GuiGridItem_GridHub);
    
    u32 ID = StringHashFNV(GridName);
    
    b32 FoundWithName = JOY_FALSE;
    Gui_Element* AtGrid = Parent->childSentinel->Next;
    while(AtGrid != Parent->childSentinel){
        
        if(AtGrid->id == ID){
            FoundWithName = JOY_TRUE;
            break;
        }
        
        AtGrid = AtGrid->Next;
    }
    
    if(FoundWithName){
        Parent->Data.GridItem.NextActiveID = ID;
    }
}

struct Gui_BoolInRect_Interaction : public gui_interaction{
    b32* Value;
    rc2 Rect;
    
    Gui_BoolInRect_Interaction(b32* Value, rc2 Rect, Gui_Element* Owner) : 
    gui_interaction(GuiInteraction_BoolInRect, Owner) 
    {
        this->Value = Value;
        this->Rect = Rect;
    }
    
    virtual void Interact(gui_state* Gui) override {
        
        this->WasHotInInteraction = 0;
        this->WasActiveInInteraction = 0;
        
        if(MouseInInteractiveArea(Gui, Rect)){
            GuiSetHot(Gui, this, JOY_TRUE);
            this->WasHotInInteraction = JOY_TRUE;
            
            if(KeyWentDown(Gui->FrameInfo.Input, MouseKey_Left)){
                GuiSetActive(Gui, this);
                *Value = !*Value;
                this->WasActiveInInteraction = JOY_TRUE;
                GuiReleaseInteraction(Gui, this);
            }
        }
        else{
            GuiSetHot(Gui, this, JOY_FALSE);
        }
    }
};

enum class Gui_Resize_Interaction_Type{
    None,
    Default,
    Proportional,
    Horizontal,
    Vertical,
};

struct Gui_Resize_Interaction : public gui_interaction{
    v2* DimensionPtr;
    v2 Position;
    v2 MinDim;
    Gui_Resize_Interaction_Type Type;
    
    Gui_Resize_Interaction(v2 Position, 
                           v2* Dimension,
                           v2 MinDim,
                           Gui_Resize_Interaction_Type Type, 
                           Gui_Element* Owner) : 
    gui_interaction(GuiInteraction_Resize, Owner)
    {
        this->DimensionPtr = Dimension;
        this->Type = Type;
        this->MinDim = MinDim;
        this->OffsetInAnchor = V2(0.0f, 0.0f);
        this->Position = Position;
    }
    
    virtual void Interact(gui_state* Gui) override {
        v2 WorkRectP = this->Position;
        v2 MouseP = Gui->FrameInfo.Input->MouseP - this->OffsetInAnchor;
        
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

struct Gui_Move_Interaction : public gui_interaction{
    v2* MovePosition;
    Gui_Move_Interaction_Type Type;
    
    Gui_Move_Interaction(v2* MovePosition, 
                         Gui_Move_Interaction_Type Type, 
                         Gui_Element* Owner) : 
    gui_interaction(GuiInteraction_Move, Owner)
    {
        this->Type = Type;
        this->OffsetInAnchor = V2(0.0f, 0.0f);
        this->MovePosition = MovePosition;
    }
    
    virtual void Interact(gui_state* Gui) override {
        v2* WorkP = this->MovePosition;
        v2 MouseP = Gui->FrameInfo.Input->MouseP - this->OffsetInAnchor;
        
        switch (this->Type) {
            case Gui_Move_Interaction_Type::Move: {
                *WorkP = MouseP;
            }break;
        }
    }
};



inline void GuiPushDim(gui_state* Gui, v2 dim){
    ASSERT(Gui->dimStackIndex < ARRAY_COUNT(Gui->dimStack));
    ASSERT(!Gui->inPushBlock);
    
    // NOTE(Dima): can't push if inside push block
    if(Gui->inPushBlock){
        
    }
    else{
        Gui->dimStack[Gui->dimStackIndex++] = dim;
    }
}

inline b32 GuiPopDim(gui_state* Gui, v2* outDim){
    b32 result = 0;
    
    int decrementValue = Gui->inPushBlock ? 0 : 1;
    
    if(Gui->dimStackIndex > 0){
        result = 1;
        
        if(outDim){
            int viewIndex = Gui->dimStackIndex - 1;
            *outDim = Gui->dimStack[viewIndex];
        }
        Gui->dimStackIndex -= decrementValue;
    }
    
    return(result);
}


inline void GuiPushDimBegin(gui_state* Gui, v2 dim){
    Gui->dimStack[Gui->dimStackIndex++] = dim;
    
    Gui->inPushBlock = 1;
}

inline void GuiPushDimEnd(gui_state* Gui){
    Gui->inPushBlock = 0;
    
    GuiPopDim(Gui, 0);
}

inline rc2 GetTxtElemRect(gui_state* Gui, Gui_Layout* lay, rc2 txtRc, v2 growScale){
    v2 popDim;
    if(GuiPopDim(Gui, &popDim)){
        txtRc.max = txtRc.min + popDim;
    }
    else{
        rc2 tempTextRc = GrowRectByScaledValue(txtRc, growScale, Gui->fontScale);
        txtRc.max = txtRc.min + GetRectDim(tempTextRc);
    }
    
    return(txtRc);
}

inline float GuiGetBaseline(gui_state* Gui, float Scale = 1.0f){
    float res = GetBaseline(Gui->MainFont, Gui->fontScale * Scale);
    
    return(res);
}

inline float GuiGetLineAdvance(gui_state* Gui, float Scale = 1.0f){
    float res = GetLineAdvance(Gui->MainFont, Gui->fontScale * Scale);
    
    return(res);
}

enum print_text_operation{
    PrintTextOp_Print,
    PrintTextOp_GetSize,
};

v2 GetTextSize(gui_state* Gui, char* Text, float Scale = 1.0f);
rc2 GetTextRect(gui_state* Gui, char* Text, v2 p, float Scale = 1.0f);
rc2 PrintText(gui_state* Gui, char* text, v2 P, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), float Scale = 1.0f);
rc2 PrintTextCenteredInRect(gui_state* Gui, char* text, rc2 tect, float Scale = 1.0f, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f));

void GuiUpdateWindows(gui_state* Gui);

void InitGui(
gui_state* Gui,
assets* Assets);

void GuiFrameBegin(gui_state* Gui, gui_frame_info GuiFrameInfo);
void GuiFrameEnd(gui_state* Gui);

void GuiBeginLayout(gui_state* Gui, char* name, u32 layoutType);
void GuiEndLayout(gui_state* Gui);

void GuiBeginPage(gui_state* Gui, char* name);
void GuiEndPage(gui_state* Gui);

void GuiBeginRow(gui_state* Gui);
void GuiEndRow(gui_state* Gui);
void GuiBeginColumn(gui_state* Gui);
void GuiEndColumn(gui_state* Gui);

void GuiFramePrepare4Render(gui_state* Gui);

void GuiTooltip(gui_state* Gui, char* tooltipText, v2 at);
void GuiText(gui_state* Gui, char* text);
b32 GuiButton(gui_state* Gui, char* buttonName);
void GuiBoolButton(gui_state* Gui, char* buttonName, b32* value);
void GuiBoolButtonOnOff(gui_state* Gui, char* buttonName, b32* value);
void GuiCheckbox(gui_state* Gui, char* name, b32* value);

void GuiBeginTree(gui_state* Gui, char* name);
void GuiEndTree(gui_state* Gui);

void GuiBeginRadioGroup(
gui_state* Gui, 
char* name, 
u32* ref, 
u32 defaultId);
void GuiRadioButton(gui_state* Gui, char* name, u32 uniqueId);
void GuiEndRadioGroup(gui_state* Gui);

void GuiInputText(gui_state* Gui, char* name, char* Buf, int BufSize);

enum gui_slider_style{
    GuiSlider_Index,
    GuiSlider_Progress,
};

void GuiSliderInt(gui_state* Gui, 
                  int* Value, 
                  int Min, 
                  int Max, 
                  char* Name, 
                  u32 Style = 0);

void GuiSliderFloat(gui_state* Gui, 
                    float* Value, 
                    float Min, 
                    float Max, 
                    char* Name, 
                    u32 Style = 0);

void GuiTest(gui_state* Gui, float deltaTime);

void GuiAnchor(gui_state* Gui, 
               char* Name, 
               v2 Pos, v2 Dim, 
               b32 Resize,
               b32 Centered, 
               v2* RectP, v2* RectDim);
#endif