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

enum GuiColorType{
    GuiColor_Graph0,
    GuiColor_Graph1,
    GuiColor_Graph2,
    GuiColor_Graph3,
    GuiColor_Graph4,
    GuiColor_Graph5,
    GuiColor_Graph6,
    GuiColor_Graph7,
    GuiColor_Graph8,
    GuiColor_Graph9,
    GuiColor_Graph10,
    GuiColor_Graph11,
    GuiColor_Graph12,
    GuiColor_Graph13,
    GuiColor_Graph14,
    GuiColor_Graph15,
    GuiColor_Graph16,
    GuiColor_Graph17,
    GuiColor_Graph18,
    GuiColor_Graph19,
    GuiColor_Graph20,
    GuiColor_Graph21,
    GuiColor_Graph22,
    GuiColor_Graph23,
    GuiColor_Graph24,
    GuiColor_Graph25,
    GuiColor_Graph26,
    GuiColor_Graph27,
    GuiColor_Graph28,
    GuiColor_Graph29,
    GuiColor_Graph30,
    GuiColor_Graph31,
    GuiColor_GraphCount,
    
    GuiColor_Text,
    GuiColor_HotText,
    GuiColor_Borders,
    GuiColor_SliderValue,
    GuiColor_Error,
    
    GuiColor_Hot,
    GuiColor_Active,
    
    GuiColor_ActiveGrad1,
    GuiColor_ActiveGrad2,
    GuiColor_InactiveGrad1,
    GuiColor_InactiveGrad2,
    
    GuiColor_HeaderActive,
    GuiColor_HeaderInactive,
    GuiColor_HeaderPreview,
    
    GuiColor_BodyActive,
    GuiColor_BodyInactive,
    GuiColor_BodyPreview,
    
    GuiColor_BackgroundActive,
    GuiColor_BackgroundInactive,
    GuiColor_BackgroundPreview,
    
    GuiColor_OutlineActive,
    GuiColor_OutlineInactive,
    GuiColor_OutlinePreview,
    
    GuiColor_Count,
};

#define GUI_GETCOLOR(color) Gui->colors[color]
#define GUI_GETCOLOR_COLSYS(index) Gui->colorState.colorTable[index].color
#define GUI_COLORHEX(str) ColorFromHex(str)

inline int GetGraphColorIndexFromHash(u32 Hash){
    int Result = GuiColor_Graph0 + Hash % (GuiColor_GraphCount - GuiColor_Graph0);
    
    return(Result);
}

enum GuiAdvanceType{
    GuiAdvanceType_Column,
    GuiAdvanceType_Row,
};

struct GuiAdvanceCtx{
    u32 Type;
    float RememberValue;
    float Baseline;
    float Maximum;
    
    float MaxHorz;
    float MaxVert;
};

struct beginned_dimension{
    u32 Type;
    v2 Dim;
};

enum begin_dimension_type{
    BeginDimension_Width,
    BeginDimension_Height,
    BeginDimension_Both,
};

enum gui_layout_flags{
    GuiLayout_Window = (1 << 0),
    GuiLayout_Resize = (1 << 1),
    GuiLayout_Move = (1 << 2),
};

struct gui_layout{
    v2 At;
    v2 Start;
    v2 Dim;
    rc2 Rect;
    
    char Name[128];
    u32 ID;
    u32 Flags;
    
    struct gui_element* Elem;
    
    gui_layout* Next;
    gui_layout* Prev;
    
    b32 DimensionIsBeginned;
    beginned_dimension BeginnedDimension;
    
    GuiAdvanceCtx AdvanceRememberStack[16];
    int StackCurrentIndex;
};

struct Gui_Window{
    Gui_Window* NextAlloc;
    Gui_Window* PrevAlloc;
    
    Gui_Window* Next;
    Gui_Window* Prev;
    
    rc2 rect;
    
    gui_layout layout;
    
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
    
    gui_element* elem;
    
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

enum gui_element_type{
    GuiElement_None,
    
    GuiElement_Root,
    GuiElement_ChildrenSentinel,
    GuiElement_Tree,
    GuiElement_Item,
    GuiElement_TempItem,
    GuiElement_RowColumn,
    GuiElement_RadioGroup,
    GuiElement_Layout,
    
    GuiElement_GridItem,
};

struct gui_element{
    char Name[64];
    char NameToShow[64];
    
    struct {
        union{
            struct{
                u32* ref;
                u32 activeId;
            } RadioGroup;
            
            struct {
                gui_layout* ref;
            } Layout;
            
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
    
    b32 Opened;
    int TmpCount;
    int Depth;
    
    gui_element* parentInTree;
    
    u32 ID;
    u32 InTreeID;
    u32 Type;
    
    gui_element* Next;
    gui_element* Prev;
    
    gui_element* NextAlloc;
    gui_element* PrevAlloc;
    
    gui_element* Parent;
    gui_element* ChildSentinel;
    int ChildCount;
};

enum gui_interaction_type{
    GuiInteraction_Empty,
    
    GuiInteraction_Move,
    GuiInteraction_BoolInRect,
};

enum gui_priority_type{
    GuiPriority_SuperSmall,
    GuiPriority_Small,
    GuiPriority_Avg,
    GuiPriority_High,
    GuiPriority_SuperHigh,
    GuiPriority_SuperPuperHigh,
};

enum gui_move_interaction_type{
    GuiMoveInteraction_Move,
    
    GuiMoveInteraction_Resize_Default,
    GuiMoveInteraction_Resize_Proportional,
    GuiMoveInteraction_Resize_Horizontal,
    GuiMoveInteraction_Resize_Vertical,
};

struct gui_interaction_data_bool_in_rect{
    b32* Value;
    rc2 Rect;
};

struct gui_interaction_data_move{
    v2* ChangePtr;
    v2 PtrStartInAnchor;
    
    v2* OffsetInAnchor;
    rc2 AnchorRect;
    
    v2 ResizedRectMin;
    v2 MinDim;
    u32 MoveType;
};

struct gui_interaction_ctx{
    u32 ID;
    char* Name;
    u32 Priority;
};

struct gui_interaction{
    gui_element* Owner;
    u32 Type;
    
    gui_interaction_ctx Context;
    
    union{
        gui_interaction_data_move Move;
        gui_interaction_data_bool_in_rect BoolInRect;
    } Data;
    
    b32 WasHotInInteraction;
    b32 WasActiveInInteraction;
};

inline void SetInteractionPriority(gui_interaction* Interaction, u32 Priority){
    Interaction->Context.Priority = Priority;
}

inline gui_interaction CreateInteraction(gui_element* Owner, 
                                         u32 InteractionType,
                                         u32 Priority = GuiPriority_Small)
{
    gui_interaction Result = {};
    
    Result.Owner = Owner;
    Result.Type = InteractionType;
    
    Result.Context.ID = Owner->InTreeID;
    Result.Context.Name = Owner->Name;
    Result.Context.Priority = Priority;
    
    return(Result);
}

struct gui_frame_info{
    render_stack* Stack;
    input_state* Input;
    int Width;
    int Height;
    float DeltaTime;
};

struct gui_state{
    assets* Assets;
    
    b32 ShowGuiTest;
    
    asset_id CheckboxMarkID;
    asset_id ChamomileID;
    
    float FontScale;
    
#define GUI_FONT_STACK_SIZE 16
    font_info* FontStack[GUI_FONT_STACK_SIZE];
    int CurFontStackIndex;
    font_info* MainFont;
    
    gui_frame_info FrameInfo;
    
    render_stack* Stack;
    input_state* Input;
    int Width;
    int Height;
    
    mem_region* Mem;
    
    gui_interaction_ctx HotInteraction;
    gui_interaction_ctx ActiveInteraction;
    
    gui_layout rootLayout;
    int layoutCount;
    
    gui_element* RootElement;
    gui_element* CurElement;
    
    gui_element* CurrentGridHub;
    
    Gui_Window windowUseSentinel;
    Gui_Window windowFreeSentinel;
    Gui_Window windowSentinel4Returning;
    Gui_Window windowLeafSentinel;
    
    Gui_Window* tempWindow1;
    Gui_Window* tempWindow2;
    
    int TotalAllocatedGuiElements;
    gui_element FreeSentinel;
    gui_element UseSentinel;
    
#define GUI_MAX_TOOLTIPS 256
    Gui_Tooltip Tooltips[GUI_MAX_TOOLTIPS];
    int TooltipIndex;
    
    Asset_Atlas* atlas;
    
    Color_State colorState;
    v4 colors[GuiColor_Count];
    float windowAlpha;
};


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

inline float GetScaledAscender(gui_state* Gui){
    float Result = GetScaledAscender(Gui->MainFont, Gui->FontScale);
    
    return(Result);
}

inline gui_element* 
GuiFindElementOfTypeUpInTree(gui_element* curElement, u32 elementType) {
    gui_element* result = 0;
    
    gui_element* At = curElement;
    while (At != 0) {
        if (At->Type == elementType) {
            result = At;
            break;
        }
        
        At = At->Parent;
    }
    
    return(result);
}


inline gui_layout* GetParentLayout(gui_state* Gui){
    gui_layout* res = 0;
    
    gui_element* layoutElem = GuiFindElementOfTypeUpInTree(
        Gui->CurElement, 
        GuiElement_Layout);
    
    if(!layoutElem){
        res = &Gui->rootLayout;
    }
    else{
        res = layoutElem->Data.Layout.ref;
    }
    
    return(res);
}

inline v2 GetDimensionLeftInLayout(gui_layout* Layout){
    v2 Result = Layout->Start + Layout->Dim - Layout->At;
    
    return(Result);
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
            Gui->HotInteraction = {};
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
        GuiSetHot(Gui, interaction, false);
    }
}

inline void GuiReleaseInteraction(gui_state* Gui, gui_interaction* interaction){
    if(interaction){
        if(GuiIsActive(Gui, interaction)){
            Gui->ActiveInteraction = {};
        }
    }
}

inline void GuiGoToGrid(gui_state* Gui, char* GridName){
    gui_element* Parent = Gui->CurrentGridHub;
    ASSERT(Parent);
    // NOTE(Dima): Can call only when grid hub is current
    ASSERT(Parent->Data.GridItem.Type == GuiGridItem_GridHub);
    
    u32 ID = StringHashFNV(GridName);
    
    b32 FoundWithName = false;
    gui_element* AtGrid = Parent->ChildSentinel->Next;
    while(AtGrid != Parent->ChildSentinel){
        
        if(AtGrid->ID == ID){
            FoundWithName = true;
            break;
        }
        
        AtGrid = AtGrid->Next;
    }
    
    if(FoundWithName){
        Parent->Data.GridItem.NextActiveID = ID;
    }
}

inline rc2 GetTxtElemRect(gui_state* Gui, gui_layout* Layout, rc2 PrintedRect){
    v2 AlignPoint = V2(0.0f, 0.0f);
    
    rc2 TempTextRc = PrintedRect;
    v2 ResultDimension = GetRectDim(TempTextRc);
    
    if(Layout->DimensionIsBeginned){
        v2 BeginnedDim = Layout->BeginnedDimension.Dim;
        u32 BegDimType = Layout->BeginnedDimension.Type;
        
        if(BegDimType == BeginDimension_Width){
            ResultDimension.x = BeginnedDim.x;
        }
        else if(BegDimType == BeginDimension_Height){
            ResultDimension.y = BeginnedDim.y;
        }
        else if(BegDimType == BeginDimension_Both){
            ResultDimension = BeginnedDim;
        }
    }
    
    rc2 Result = RcMinDim(PrintedRect.Min, ResultDimension);
    
    return(Result);
}

inline float GuiGetBaseline(gui_state* Gui, float Scale = 1.0f){
    float res = GetBaseline(Gui->MainFont, Gui->FontScale * Scale);
    
    return(res);
}

inline float GuiGetLineAdvance(gui_state* Gui, float Scale = 1.0f){
    float res = GetLineAdvance(Gui->MainFont, Gui->FontScale * Scale);
    
    return(res);
}

inline v2 ScaledAscDim(gui_state* Gui, v2 Dim){
    v2 Result = V2(
        GetScaledAscender(Gui->MainFont, Gui->FontScale * Dim.x),
        GetScaledAscender(Gui->MainFont, Gui->FontScale * Dim.y));
    
    return(Result);
}

enum print_text_operation{
    PrintTextOp_Print,
    PrintTextOp_GetSize,
};

v2 GetTextSize(gui_state* Gui, char* Text, float Scale = 1.0f);
rc2 GetTextRect(gui_state* Gui, char* Text, v2 p, float Scale = 1.0f);
rc2 PrintText(gui_state* Gui, char* text, v2 P, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), float Scale = 1.0f);
rc2 PrintTextCenteredInRect(gui_state* Gui, char* text, rc2 tect, float Scale = 1.0f, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f));

void InitGui(
gui_state* Gui,
assets* Assets);

void GuiFrameBegin(gui_state* Gui, gui_frame_info GuiFrameInfo);
void GuiFrameEnd(gui_state* Gui);

void BeginLayout(gui_state* Gui, char* name, u32 layoutType, v2* P = 0, v2* Dim = 0);
void EndLayout(gui_state* Gui);

void BeginRow(gui_state* Gui);
void EndRow(gui_state* Gui);
void BeginColumn(gui_state* Gui);
void EndColumn(gui_state* Gui);

void GuiFramePrepare4Render(gui_state* Gui);

void ShowTooltip(gui_state* Gui, char* tooltipText, v2 at);
void ShowHeader(gui_state* Gui, char* HeaderText);
void ShowText(gui_state* Gui, char* text);
b32 Button(gui_state* Gui, char* buttonName);
b32 LinkButton(gui_state* Gui, char* buttonName);
b32 BoolButtonTrueFalse(gui_state* Gui, char* buttonName, b32* value);
b32 BoolButtonOnOff(gui_state* Gui, char* buttonName, b32* value);
b32 Checkbox(gui_state* Gui, char* name, b32* value);
void ShowBool(gui_state* Gui, char* Name, b32 Value);
void ShowInt(gui_state* Gui, char* Name, int Value);
void ShowFloat(gui_state* Gui, char* Name, float Value);

void BeginTree(gui_state* Gui, char* name);
void EndTree(gui_state* Gui);

void BeginRadioGroup(
gui_state* Gui, 
char* name, 
u32* ref, 
u32 defaultId);
void RadioButton(gui_state* Gui, char* name, u32 uniqueId);
void EndRadioGroup(gui_state* Gui);

void InputText(gui_state* Gui, char* name, char* Buf, int BufSize);

enum gui_slider_style{
    GuiSlider_Index,
    GuiSlider_Progress,
    GuiSlider_ProgressNonModify,
};

void SliderInt(gui_state* Gui, 
               int* Value, 
               int Min, 
               int Max, 
               char* Name, 
               u32 Style = 0);

void SliderFloat(gui_state* Gui, 
                 float* Value, 
                 float Min, 
                 float Max, 
                 char* Name, 
                 u32 Style = 0);

void ProgressSlider01(gui_state* Gui,
                      char* Name,
                      float Value);

void GuiTest(gui_state* Gui, float deltaTime);

void ShowAnchor(gui_state* Gui, 
                char* Name, 
                v2 Pos, v2 Dim, 
                b32 Resize,
                b32 Centered, 
                v2* RectP, v2* RectDim);
#endif