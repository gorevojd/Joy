#include "joy_gui.h"
#include "joy_defines.h"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#include "stb_sprintf.h"

// NOTE(Dima): Returns newly pushed window array
INTERNAL_FUNCTION Gui_Window* GuiGrowWindowFreePool(Gui_State* gui, Memory_Region* mem,  int count){
    Gui_Window* windowFreePoolArray = PushArray(mem, Gui_Window, count);
    
    for(int index = 0; 
        index < count;
        index++)
    {
        Gui_Window* window = windowFreePoolArray + index;
        
        *window = {};
        
        window->PrevAlloc = gui->windowFreeSentinel.PrevAlloc;
        window->NextAlloc = &gui->windowFreeSentinel;
        
        window->PrevAlloc->NextAlloc = window;
        window->NextAlloc->PrevAlloc = window;
    }
    
    return(windowFreePoolArray);
}

inline Gui_Window* GuiPopFromReturnList(Gui_State* gui){
    Gui_Window* result = gui->windowSentinel4Returning.Next;
    
    // NOTE(Dima): deleting from return list
    result->Next->Prev = result->Prev;
    result->Prev->Next = result->Next;
    
    result->Next = 0;
    result->Prev = 0;
    
    return(result);
}

inline void GuiDeallocateWindow(Gui_State* gui, Gui_Window* todo){
    todo->NextAlloc->PrevAlloc = todo->PrevAlloc;
    todo->PrevAlloc->NextAlloc = todo->NextAlloc;
    
    todo->NextAlloc = gui->windowFreeSentinel.NextAlloc;
    todo->PrevAlloc = &gui->windowFreeSentinel;
    
    todo->NextAlloc->PrevAlloc = todo;
    todo->PrevAlloc->NextAlloc = todo;
}

INTERNAL_FUNCTION void GuiDeallocateElement(Gui_State* gui, Gui_Element* elem)
{
    elem->Next->Prev = elem->Prev;
    elem->Prev->Next = elem->Next;
    
    elem->Next = gui->freeSentinel.Next;
    elem->Prev = &gui->freeSentinel;
    
    elem->Next->Prev = elem;
    elem->Prev->Next = elem;
}

INTERNAL_FUNCTION Gui_Element* GuiAllocateElement(
Gui_State* gui)
{
    Gui_Element* result = 0;
    
    if(gui->freeSentinel.NextAlloc != &gui->freeSentinel){
        
    }
    else{
        const int count = 128;
        Gui_Element* elemPoolArray = PushArray(gui->mem, Gui_Element, count);
        
        for(int index = 0; 
            index < count;
            index++)
        {
            Gui_Element* elem = elemPoolArray + index;
            
            elem->PrevAlloc = gui->freeSentinel.PrevAlloc;
            elem->NextAlloc = &gui->freeSentinel;
            
            elem->PrevAlloc->NextAlloc = elem;
            elem->NextAlloc->PrevAlloc = elem;
        }
    }
    
    result = gui->freeSentinel.NextAlloc;
    
    // NOTE(Dima): Deallocating from free list
    result->NextAlloc->PrevAlloc = result->PrevAlloc;
    result->PrevAlloc->NextAlloc = result->NextAlloc;
    
    // NOTE(Dima): Allocating in use list
    result->NextAlloc = &gui->useSentinel;
    result->PrevAlloc = gui->useSentinel.PrevAlloc;
    
    result->NextAlloc->PrevAlloc = result;
    result->PrevAlloc->NextAlloc = result;
    
    return(result);
}

INTERNAL_FUNCTION Gui_Element* GuiInitElement(Gui_State* gui,
                                              char* name,
                                              Gui_Element** cur,
                                              u32 type, 
                                              b32 initOpened)
{
    Gui_Element* result = 0;
    
    // NOTE(Dima): Try find element in hierarchy
    Gui_Element* childSentinel = (*cur)->childSentinel;
    Gui_Element* at = 0;
    Gui_Element* found = 0;
    if(childSentinel){
        at = childSentinel->Next;
        
        u32 id = StringHashFNV(name);
        u32 InTreeID = id;
        if(at->parent){
            InTreeID *= at->parent->id + 7853;
        }
        
        while(at != childSentinel){
            if(id == at->id){
                found = at;
                break;
            }
            
            at = at->Next;
        }
        
        // NOTE(Dima): if element was not found - then allocate and initialize
        if(!found){
            found = GuiAllocateElement(gui);
            
            // NOTE(Dima): Inserting to list
            found->Next = childSentinel->Next;
            found->Prev = childSentinel;
            
            found->Next->Prev = found;
            found->Prev->Next = found;
            
            // NOTE(Dima): Incrementing parent childCount
            if(found->parent){
                found->parent->childCount++;
            }
            
            found->id = id;
            CopyStrings(found->name, sizeof(found->name), name);
            
            found->parent = *cur;
            found->type = type;
            
            // NOTE(Dima): freeing data
            found->data = {};
            
            // NOTE(Dima): Initializing children sentinel
            found->childCount = 0;
            found->childSentinel = GuiAllocateElement(gui);
            Gui_Element* fcs = found->childSentinel;
            fcs->parent = found;
            fcs->childSentinel = 0;
            fcs->Next = fcs;
            fcs->Prev = fcs;
            CopyStrings(fcs->name, "ChildrenSentinel");
            fcs->id = StringHashFNV(fcs->name);
            fcs->type = GuiElement_ChildrenSentinel;
            
            // NOTE(Dima): Initializing opened
            found->opened = initOpened;
        }
        
        ASSERT(found->type == type);
        
        *cur = found;
        
        result = found;
    }
    
    return(result);
}


INTERNAL_FUNCTION Gui_Element* GuiBeginElement(Gui_State* gui,
                                               char* name,
                                               u32 type,
                                               b32 opened)
{
    Gui_Element* Result = GuiInitElement(gui, 
                                         name, 
                                         &gui->curElement, 
                                         type,
                                         opened);
    
    return(Result);
}

INTERNAL_FUNCTION b32 GuiElementOpenedInTree(Gui_Element* elem){
    b32 Result = 1;
    Gui_Element* at = elem->parent;
    
    while(at->parent != 0){
        if(at->opened != 1){
            Result = 0;
            break;
        }
        
        at = at->parent;
    }
    
    return(Result);
}

INTERNAL_FUNCTION void GuiEndElement(Gui_State* gui, u32 type)
{
    ASSERT(gui->curElement->type == type);
    
    gui->curElement = gui->curElement->parent;
}

INTERNAL_FUNCTION void GuiFreeElement(Gui_State* gui,
                                      Gui_Element* elem)
{
    elem->NextAlloc->PrevAlloc = elem->PrevAlloc;
    elem->PrevAlloc->NextAlloc = elem->NextAlloc;
    
    elem->NextAlloc = gui->freeSentinel.NextAlloc;
    elem->PrevAlloc = &gui->freeSentinel;
    
    elem->NextAlloc->PrevAlloc = elem;
    elem->PrevAlloc->NextAlloc = elem;
}

// NOTE(Dima): This function allocates as much windows as we need and 
// NOTE(Dima): then adds them to return list. It returns first element
// NOTE(Dima): of that list
INTERNAL_FUNCTION Gui_Window* GuiAllocateWindows(Gui_State* gui, int count)
{
    // NOTE(Dima): If free list is emty then allocate some more to it
    b32 canAllocateArray = 1;
    int canAllocateCount = count;
    Gui_Window* checkAt = gui->windowFreeSentinel.NextAlloc;
    for(int checkIndex = 0; checkIndex < count; checkIndex++){
        if(checkAt == &gui->windowFreeSentinel){
            canAllocateArray = 0;
            canAllocateCount = checkIndex;
            break;
        }
        
        checkAt = checkAt->NextAlloc;
    }
    
    int toAllocateCount = Max(128, count - canAllocateCount);
    if(!canAllocateArray){
        GuiGrowWindowFreePool(gui, gui->mem, toAllocateCount);
    }
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(gui->windowSentinel4Returning.Next == &gui->windowSentinel4Returning);
    
    for(int addIndex = 0;
        addIndex < count;
        addIndex++)
    {
        // NOTE(Dima): Before in this algo we ensured that we would
        // NOTE(Dima): have as mush elements as we need. But for sure
        // NOTE(Dima): I'll double check if we can grab one more element.
        Assert(gui->windowFreeSentinel.NextAlloc != &gui->windowFreeSentinel);
        
        // NOTE(Dima): Allocating from free list
        Gui_Window* addWindow = gui->windowFreeSentinel.NextAlloc;
        
        addWindow->PrevAlloc->NextAlloc = addWindow->NextAlloc;
        addWindow->NextAlloc->PrevAlloc = addWindow->PrevAlloc;
        
        // NOTE(Dima): Inserting to use list
        addWindow->NextAlloc = &gui->windowUseSentinel;
        addWindow->PrevAlloc = gui->windowUseSentinel.PrevAlloc;
        
        addWindow->NextAlloc->PrevAlloc = addWindow;
        addWindow->PrevAlloc->NextAlloc = addWindow;
        
        // NOTE(Dima): Inserting to return list
        addWindow->Next = &gui->windowSentinel4Returning;
        addWindow->Prev = gui->windowSentinel4Returning.Prev;
        
        addWindow->Next->Prev = addWindow;
        addWindow->Prev->Next = addWindow;
    }
    
    Gui_Window* result = gui->windowSentinel4Returning.Next;
    
    return(result);
}

INTERNAL_FUNCTION Gui_Window* GuiAllocateWindow(Gui_State* gui){
    GuiAllocateWindows(gui, 1);
    
    Gui_Window* result = GuiPopFromReturnList(gui);
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(gui->windowSentinel4Returning.Next == &gui->windowSentinel4Returning);
    
    return(result);
}

inline void GuiAddWindowToList(Gui_Window* window, 
                               Gui_Window* Sentinel)
{
    window->Next = Sentinel;
    window->Prev = Sentinel->Prev;
    
    window->Next->Prev = window;
    window->Prev->Next = window;
}

INTERNAL_FUNCTION void GuiInitRoot(Gui_State* gui, Gui_Element** root){
    
    (*root) = GuiAllocateElement(gui);
    (*root)->Next = (*root);
    (*root)->Prev = (*root);
    (*root)->parent = 0;
    (*root)->type = GuiElement_Root;
    CopyStrings((*root)->name, "RootElement!!!");
    (*root)->id = StringHashFNV((*root)->name);
    
    (*root)->childCount = 0;
    (*root)->childSentinel = GuiAllocateElement(gui);
    Gui_Element* rcs = (*root)->childSentinel;
    rcs->Next = rcs;
    rcs->Prev = rcs;
    rcs->parent = (*root);
    rcs->childSentinel = 0;
    CopyStrings(rcs->name, "RootChildrenSentinel");
    rcs->id = StringHashFNV(rcs->name);
    rcs->type = GuiElement_ChildrenSentinel;
    
}

void GuiBeginPage(Gui_State* gui, char* name){
    u32 nameID = StringHashFNV(name);
    
    Gui_Page* foundPage = 0;
    Gui_Page* pageAt = gui->rootPage.Next;
    for(pageAt; pageAt != &gui->rootPage; pageAt = pageAt->Next){
        if(nameID == pageAt->id){
            foundPage = pageAt;
            break;
        }
    }
    
    if(!foundPage){
        foundPage = PushStruct(gui->mem, Gui_Page);
        
        CopyStrings(foundPage->name, sizeof(foundPage->name), name);
        foundPage->id = nameID;
        
        foundPage->Next = gui->rootPage.Next;
        foundPage->Prev = &gui->rootPage;
        foundPage->Next->Prev = foundPage;
        foundPage->Prev->Next = foundPage;
        
        ++gui->pageCount;
    }
    
    // NOTE(Dima): Init page element
    Gui_Element* pageElem = GuiBeginElement(gui, 
                                            name,
                                            GuiElement_Page,
                                            JOY_TRUE);
    // NOTE(Dima): Init references
    pageElem->data.Page.ref = foundPage;
    foundPage->elem = pageElem;
}

void GuiEndPage(Gui_State* gui){
    GuiEndElement(gui, GuiElement_Page);
}

void InitGui(
Gui_State* gui, 
Input* input, 
Assets* assets, 
Memory_Region* mem, 
Render_Stack* stack,
int width,
int height)
{
    gui->mainFont = &assets->inconsolataBold;
    gui->CheckboxMark = &assets->CheckboxMark;
    gui->fontScale = 1.0f;
    
    gui->stack = stack;
    gui->input = input;
    gui->mem = mem;
    
    gui->width = width;
    gui->height = height;
    
    // NOTE(Dima): Init layouts
    gui->layoutCount = 1;
    gui->rootLayout = {};
    gui->rootLayout.Next = &gui->rootLayout;
    gui->rootLayout.Prev = &gui->rootLayout;
    CopyStrings(gui->rootLayout.Name, "RootLayout");
    gui->rootLayout.ID = StringHashFNV(gui->rootLayout.Name);
    
    // NOTE(Dima): Init dim stack
    for(int dimIndex = 0; dimIndex < ARRAY_COUNT(gui->dimStack); dimIndex++){
        gui->dimStack[dimIndex] = {};
    }
    gui->dimStackIndex = 0;
    gui->inPushBlock = 0;
    
    
    // NOTE(Dima): Init pages
    gui->pageCount = 1;
    gui->rootPage = {};
    gui->rootPage.Next = &gui->rootPage;
    gui->rootPage.Prev = &gui->rootPage;
    CopyStrings(gui->rootPage.name, "RootPage");
    gui->rootPage.id = StringHashFNV(gui->rootPage.name);
    
    // NOTE(Dima): Initializing of window free pool and sentinels
    gui->windowUseSentinel.NextAlloc = &gui->windowUseSentinel;
    gui->windowUseSentinel.PrevAlloc = &gui->windowUseSentinel;
    gui->windowFreeSentinel.NextAlloc = &gui->windowFreeSentinel;
    gui->windowFreeSentinel.PrevAlloc = &gui->windowFreeSentinel;
    
    GuiGrowWindowFreePool(gui, mem, 128);
    
    // NOTE(Dima): Init window sentinel for returning windows
    // NOTE(Dima): as list when we allocate multiple of them.
    gui->windowSentinel4Returning.Next = &gui->windowSentinel4Returning;
    gui->windowSentinel4Returning.Prev = &gui->windowSentinel4Returning;
    
    // NOTE(Dima): Init window leaf sentinel
    gui->windowLeafSentinel.Next = &gui->windowLeafSentinel;
    gui->windowLeafSentinel.Prev = &gui->windowLeafSentinel;
    
    gui->tempWindow1 = GuiAllocateWindow(gui);
    gui->tempWindow1->rect = RcMinDim(V2(10, 10), V2(1000, 600));
    gui->tempWindow1->visible = 1;
    GuiAddWindowToList(gui->tempWindow1, &gui->windowLeafSentinel);
    
    // NOTE(Dima): Initializing elements sentinel
    gui->freeSentinel.NextAlloc = &gui->freeSentinel;
    gui->freeSentinel.PrevAlloc = &gui->freeSentinel;
    
    gui->useSentinel.NextAlloc = &gui->useSentinel;
    gui->useSentinel.PrevAlloc = &gui->useSentinel;
    
    // NOTE(Dima): Initializing root element
    GuiInitRoot(gui, &gui->rootElement);
    
    // NOTE(Dima): Setting current element
    gui->curElement = gui->rootElement;
    
    // NOTE(Dima): Initializing colors
    InitColorsState(&gui->colorState, mem);
    gui->colors[GuiColor_Text] = GUI_GETCOLOR_COLSYS(Color_White);
    gui->colors[GuiColor_HotText] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    gui->colors[GuiColor_Borders] = GUI_GETCOLOR_COLSYS(Color_Black);
    
    gui->colors[GuiColor_Hot] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    gui->colors[GuiColor_Active] = GUI_GETCOLOR_COLSYS(Color_Red);
    
    gui->colors[GuiColor_ButtonBackground] = GUI_COLORHEX("#337733");
    gui->colors[GuiColor_ButtonBackgroundHot] = GUI_GETCOLOR_COLSYS(Color_Cyan);
    gui->colors[GuiColor_ButtonForeground] = GUI_GETCOLOR_COLSYS(Color_White);
    gui->colors[GuiColor_ButtonForegroundHot] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    gui->colors[GuiColor_ButtonForegroundDisabled] = gui->colors[GuiColor_ButtonForeground] * 0.75f;
    gui->colors[GuiColor_ButtonGrad1] = ColorFromHex("#FD6a02");
    gui->colors[GuiColor_ButtonGrad2] = ColorFromHex("#883000");
    
    gui->windowAlpha = 0.85f;
    gui->colors[GuiColor_WindowBackground] = V4(0.0f, 0.0f, 0.0f, gui->windowAlpha);
    gui->colors[GuiColor_WindowBorder] = GUI_GETCOLOR_COLSYS(Color_Black);
    gui->colors[GuiColor_WindowBorderHot] = GUI_GETCOLOR_COLSYS(Color_Magenta);
    gui->colors[GuiColor_WindowBorderActive] = GUI_GETCOLOR_COLSYS(Color_Blue);
}

rc2 PrintTextInternal(Font_Info* font, Render_Stack* stack, char* text, v2 p, u32 textOp, float scale, v4 color, int CaretP, v2* CaretPrintPOut)
{
    rc2 txtRc;
    
    char* at = text;
    
    v2 curP = p;
    
    while(*at){
        
        if(text + CaretP == at){
            if(CaretPrintPOut){
                *CaretPrintPOut = curP;
            }
        }
        
        if(*at >= ' ' && *at <= '~'){
            
            int glyphIndex = font->Codepoint2Glyph[*at];
            Glyph_Info* glyph = &font->Glyphs[glyphIndex];
            
            float bmpScale = glyph->Height * scale;
            
            v2 bitmapDim = { glyph->Bitmap.WidthOverHeight * bmpScale, bmpScale };
            
            if(textOp == PrintTextOp_Print){
                float bitmapMinY = curP.y + glyph->YOffset * scale;
                float bitmapMinX = curP.x + glyph->XOffset * scale;
                
                PushGlyph(stack, 
                          V2(bitmapMinX, bitmapMinY), 
                          bitmapDim, 
                          &glyph->Bitmap,
                          glyph->Bitmap.MinUV,
                          glyph->Bitmap.MaxUV,
                          color);
            }
            
            curP.x += ((float)glyph->Advance * scale);
            
        }
        else if(*at == '\t'){
            curP.x += ((float)font->AscenderHeight * 4 * scale);
        }
        
        at++;
    }
    
    if(text + CaretP == at){
        if(CaretPrintPOut){
            *CaretPrintPOut = curP;
        }
    }
    
    txtRc.min.x = p.x;
    txtRc.min.y = p.y - font->AscenderHeight * scale;
    txtRc.max.x = curP.x;
    txtRc.max.y = curP.y - font->DescenderHeight * scale;
    
    return(txtRc);
}

void PrintCaret(Gui_State* gui, v2 PrintP, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f)){
    float bmpScale = GuiGetLineAdvance(gui);
    
    float CaretMinY = PrintP.y - GetScaledAscender(gui->mainFont, gui->fontScale);
    float CaretMinX = PrintP.x;;
    
    v2 CaretMin = V2(CaretMinX, CaretMinY);
    v2 CaretDim = V2(bmpScale * 0.6f, bmpScale);
    PushRect(gui->stack, RcMinDim(CaretMin, CaretDim), Color);
}

v2 GetTextSizeInternal(Font_Info* font, char* Text, float scale){
    rc2 TextRc = PrintTextInternal(
        font, 
        0, 
        Text, V2(0.0f, 0.0f), 
        PrintTextOp_GetSize, 
        scale);
    
    v2 result = GetRectDim(TextRc);
    
    return(result);
}

inline v2 GetCenteredTextOffset(Font_Info* font, float TextDimX, rc2 Rect, float scale = 1.0f){
    float LineDimY = GetLineAdvance(font, scale);
    float LineDelta = (font->AscenderHeight + font->LineGap) * scale - LineDimY * 0.5f;
    
    v2 CenterRc = Rect.min + GetRectDim(Rect) * 0.5f;
    
    v2 TargetP = V2(CenterRc.x - TextDimX * 0.5f, CenterRc.y + LineDelta);
    return(TargetP);
}

inline float GetCenteredTextOffsetY(Font_Info* font, rc2 rect, float scale = 1.0f){
    float result = GetCenteredTextOffset(font, 0.0f, rect, scale).y;
    
    return(result);
}

inline v2 GetCenteredTextOffset(Font_Info* font, char* Text, rc2 rect, float scale = 1.0f){
    v2 TextDim = GetTextSizeInternal(font, Text, scale);
    
    v2 result = GetCenteredTextOffset(font, TextDim.x, rect, scale);
    
    return(result);
}

rc2 PrintTextCenteredInRectInternal(
Font_Info* font, 
Render_Stack* stack, 
char* text, 
rc2 rect, 
float scale, 
v4 color)
{
    v2 targetP = GetCenteredTextOffset(font, text, rect, scale);
    rc2 result = PrintTextInternal(font, stack, text, targetP, PrintTextOp_Print, scale, color);
    
    return(result);
}

v2 GetTextSize(Gui_State* gui, char* text, float scale){
    v2 result = GetTextSizeInternal(gui->mainFont, 
                                    text, 
                                    gui->fontScale * scale);
    
    return(result);
}

rc2 GetTextRect(Gui_State* gui, char* text, v2 p, float scale){
    rc2 TextRc = PrintTextInternal(
        gui->mainFont, 
        gui->stack, 
        text, p, 
        PrintTextOp_GetSize, 
        gui->fontScale * scale);
    
    return(TextRc);
}

rc2 PrintText(Gui_State* gui, char* text, v2 p, v4 color, float scale){
    rc2 TextRc = PrintTextInternal(
        gui->mainFont, 
        gui->stack, 
        text, p, 
        PrintTextOp_Print, 
        gui->fontScale * scale,
        color);
    
    return(TextRc);
}


v2 GetCaretPrintP(Gui_State* gui, char* text, v2 p, int CaretP){
    v2 Result;
    
    rc2 TextRc = PrintTextInternal(
        gui->mainFont, 
        gui->stack, 
        text, p, 
        PrintTextOp_Print, 
        gui->fontScale * gui->fontScale,
        V4(0.0f, 0.0f, 0.0f, 0.0f), 
        CaretP, &Result);
    
    return(Result);
}


rc2 PrintTextCenteredInRect(Gui_State* gui, char* text, rc2 rect, float scale, v4 color){
    rc2 result = PrintTextCenteredInRectInternal(gui->mainFont,
                                                 gui->stack,
                                                 text, 
                                                 rect,
                                                 gui->fontScale * scale,
                                                 color);
    
    return(result);
}


struct GuiSnapInWindowResult{
    union{
        struct{
            rc2 result;
            rc2 restRect;
        };
        rc2 rects[2];
    };
};

GuiSnapInWindowResult GuiSnapInWindowRect(rc2 WindowRect, u32 SnapType){
    GuiSnapInWindowResult result;
    
    v2 Min = WindowRect.min;
    v2 Max = WindowRect.max;
    
    v2 WindowDim = GetRectDim(WindowRect);
    v2 WindowHalfDim = WindowDim * 0.5f;
    
    switch(SnapType){
        case GuiWindowSnap_Left:{
            result.result = RcMinDim(Min, 
                                     V2(WindowHalfDim.x, WindowDim.y));
            result.restRect = RcMinDim(V2(Min.x + WindowHalfDim.x, Min.y),
                                       V2(WindowHalfDim.x, WindowDim.y));
        }break;
        
        case GuiWindowSnap_Right:{
            result.result = RcMinDim(V2(Min.x + WindowHalfDim.x, Min.y),
                                     V2(WindowHalfDim.x, WindowDim.y));
            result.restRect =  RcMinDim(Min, 
                                        V2(WindowHalfDim.x, WindowDim.y));
        }break;
        
        case GuiWindowSnap_Top:{
            result.result = RcMinDim(Min, V2(WindowDim.x, WindowHalfDim.y));
            result.restRect = RcMinDim(V2(Min.x, Min.y + WindowHalfDim.y), V2(WindowDim.x, WindowHalfDim.y));
        }break;
        
        case GuiWindowSnap_Bottom:{
            result.result = RcMinDim(V2(Min.x, Min.y + WindowHalfDim.y), V2(WindowDim.x, WindowHalfDim.y));
            result.restRect = RcMinDim(Min, V2(WindowDim.x, WindowHalfDim.y));;
        }break;
        
        case GuiWindowSnap_Whole:{
            result.result = WindowRect;
            result.restRect = {};
        }break;
        
        case GuiWindowSnap_CenterHalf:{
            result.result = RcMinDim(Min + WindowHalfDim * 0.5f, WindowHalfDim);
        }break;
        
        default:{
            INVALID_CODE_PATH;
        }break;
    }
    
    return(result);
}

inline void GuiSnapInWindowRect(Gui_State* gui, v2* P, v2* Dim, u32 SnapType){
    rc2 WindowRc = RcMinDim(V2(0.0f, 0.0f), V2(gui->width, gui->height));
    GuiSnapInWindowResult Res = GuiSnapInWindowRect(WindowRc, SnapType);
    
    v2 ResultP = Res.result.min;
    v2 ResultDim = GetRectDim(Res.result);
    
    if(P){
        *P = ResultP;
    }
    
    if(Dim){
        *Dim = ResultDim;
    }
}

INTERNAL_FUNCTION void GuiInitLayout(Gui_State* gui, Gui_Layout* layout, u32 layoutType, Gui_Element* layoutElem){
    // NOTE(Dima): initializing references
    layoutElem->data.Layout.ref = layout;
    layout->Elem = layoutElem;
    
    v2 popDim;
    if(!GuiPopDim(gui, &popDim)){
        popDim = V2(640, 480);
    }
    
    // NOTE(Dima): Layout initializing
    layout->Type = layoutType;
    if(!layoutElem->data.IsInit){
        layout->Start = V2(200.0f, 200.0f) * (gui->layoutCount - 1);
        layout->At = layout->Start;
        
        switch(layoutType){
            case GuiLayout_Layout:{
                layout->Dim = V2(gui->width, gui->height);
            }break;
            
            case GuiLayout_Window:{
                layout->Dim = popDim;
            }break;
        }
        
        layoutElem->data.IsInit = JOY_TRUE;
    }
    
    // NOTE(Dima): Initializing initial advance ctx
    layout->AdvanceRememberStack[0].type = GuiAdvanceType_Column;
    layout->AdvanceRememberStack[0].rememberValue = layout->Start.y;
    layout->AdvanceRememberStack[0].baseline = layout->Start.x;
    
    if(layoutType == GuiLayout_Window){
        
        Gui_Empty_Interaction Interaction(layoutElem);
        
#if 1    
        
        GuiAnchor(gui, "Anchor1", 
                  layout->Start + layout->Dim,
                  V2(10, 10),
                  JOY_TRUE,
                  JOY_TRUE,
                  &layout->Start,
                  &layout->Dim);
        
        GuiAnchor(gui, "Anchor2", 
                  layout->Start,
                  V2(10, 10),
                  JOY_FALSE,
                  JOY_TRUE,
                  &layout->Start,
                  &layout->Dim);
        
#endif
        
        rc2 windowRc = RcMinDim(layout->Start, layout->Dim);
        PushRect(gui->stack, windowRc, GUI_GETCOLOR(GuiColor_WindowBackground));
        
        v4 outlineColor = GUI_GETCOLOR(GuiColor_WindowBorder);
        if(MouseInRect(gui->input, windowRc)){
            GuiSetHot(gui, Interaction.ID, JOY_TRUE);
            if(GuiIsHot(gui, &Interaction)){
                outlineColor = GUI_GETCOLOR(GuiColor_Hot);
            }
            
            if(KeyWentDown(gui->input, MouseKey_Left)){
                GuiSetActive(gui, &Interaction);
            }
            
        }
        else{
            
            if(KeyWentDown(gui->input, MouseKey_Left) ||
               KeyWentDown(gui->input, MouseKey_Right))
            {
                GuiReleaseInteraction(gui, &Interaction);
            }
            
            GuiSetHot(gui, Interaction.ID, JOY_FALSE);
        }
        
        if(GuiIsActive(gui, &Interaction)){
            outlineColor = GUI_GETCOLOR(GuiColor_Active);
            
            if(KeyWentDown(gui->input, Key_Left)){
                GuiSnapInWindowRect(gui, &layout->Start, &layout->Dim, GuiWindowSnap_Left);
            }
            
            if(KeyWentDown(gui->input, Key_Right)){
                GuiSnapInWindowRect(gui, &layout->Start, &layout->Dim, GuiWindowSnap_Right);
            }
            
            if(KeyWentDown(gui->input, Key_Up)){
                GuiSnapInWindowRect(gui, &layout->Start, &layout->Dim, GuiWindowSnap_Top);
            }
            
            if(KeyWentDown(gui->input, Key_Down)){
                GuiSnapInWindowRect(gui, &layout->Start, &layout->Dim, GuiWindowSnap_Bottom);
            }
            
            if(KeyWentDown(gui->input, MouseKey_Right)){
                GuiSnapInWindowRect(gui, &layout->Start, &layout->Dim, GuiWindowSnap_CenterHalf);
            }
        }
        
        // NOTE(Dima): Pushing inner outline
        PushRectOutline(gui->stack, windowRc, 2, outlineColor);
    }
}

void GuiBeginLayout(Gui_State* gui, char* name, u32 layoutType){
    // NOTE(Dima): In list inserting
    u32 nameID = StringHashFNV(name);
    
    Gui_Layout* foundLayout = 0;
    Gui_Layout* layoutAt = gui->rootLayout.Next;
    for(layoutAt; layoutAt != &gui->rootLayout; layoutAt = layoutAt->Next){
        if(nameID == layoutAt->ID){
            foundLayout = layoutAt;
            break;
        }
    }
    
    if(!foundLayout){
        foundLayout = PushStruct(gui->mem, Gui_Layout);
        
        CopyStrings(foundLayout->Name, sizeof(foundLayout->Name), name);
        foundLayout->ID = nameID;
        
        foundLayout->Next = gui->rootLayout.Next;
        foundLayout->Prev = &gui->rootLayout;
        foundLayout->Next->Prev = foundLayout;
        foundLayout->Prev->Next = foundLayout;
        
        ++gui->layoutCount;
    }
    
    // NOTE(Dima): Beginnning layout elem
    Gui_Element* layoutElem = GuiBeginElement(gui, name, GuiElement_Layout, JOY_TRUE);
    
    GuiInitLayout(gui, foundLayout, layoutType, layoutElem);
}

void GuiEndLayout(Gui_State* gui){
    Gui_Layout* lay = GetParentLayout(gui);
    
    lay->At = lay->Start;
    
    GuiEndElement(gui, GuiElement_Layout);
}

INTERNAL_FUNCTION void GuiSplitWindow(Gui_State* gui, 
                                      Gui_Window* window, 
                                      int partsCount, 
                                      rc2* partsRects)
{
    GuiAllocateWindows(gui, partsCount);
    
    for(int newWindowIndex = 0;
        newWindowIndex < partsCount;
        newWindowIndex++)
    {
        Gui_Window* newWindow = GuiPopFromReturnList(gui);
        
        // NOTE(Dima): Adding children to leafs
        newWindow->Next = gui->windowLeafSentinel.Next;
        newWindow->Prev = &gui->windowLeafSentinel;
        
        newWindow->Next->Prev = newWindow;
        newWindow->Prev->Next = newWindow;
        
        newWindow->rect = partsRects[newWindowIndex];
    }
    
    // NOTE(Dima): Deallocating parent because it is not visible
    window->Next->Prev = window->Prev;
    window->Prev->Next = window->Next;
    
    window->Next = 0;
    window->Prev = 0;
    
    GuiDeallocateWindow(gui, window);
    
    // NOTE(Dima): Return list shoud be empty after usage in this function
    Assert(gui->windowSentinel4Returning.Next == &gui->windowSentinel4Returning);
}

INTERNAL_FUNCTION void GuiUpdateWindow(Gui_State* gui, Gui_Window* window){
    
    window->layout.Start = window->rect.min;
    
    if(window->visible){
        
        rc2 windowRc = window->rect;
        PushRect(gui->stack, windowRc, GUI_GETCOLOR(GuiColor_WindowBackground));
        
        v4 outlineColor = GUI_GETCOLOR(GuiColor_WindowBorder);
        if(MouseInRect(gui->input, windowRc)){
            outlineColor = GUI_GETCOLOR(GuiColor_WindowBorderHot);
            
            b32 conditionForSnapping = 0;
            if(conditionForSnapping){
                
                // NOTE(Dima): Processing snapping
                v2 windowDim = GetRectDim(windowRc);
                v2 windowHalfDim = windowDim * 0.5f;
                v2 windowCenter = windowRc.min + windowHalfDim;
                
                v2 MouseP = gui->input->MouseP;
                MouseP = ClampInRect(MouseP, windowRc);
                v2 diffFromCenter = MouseP - windowCenter;
                v2 diffRelative;
                diffRelative.x = diffFromCenter.x / windowHalfDim.x;
                diffRelative.y = diffFromCenter.y / windowHalfDim.y;
                v2 absDiff = diffRelative;
                absDiff.x = Abs(absDiff.x);
                absDiff.y = Abs(absDiff.y);
                
                float maxAbsDiff = 0.35f;
                u32 snapType = GuiWindowSnap_Whole;
                if(diffRelative.x < 0){
                    if(absDiff.x > maxAbsDiff){
                        maxAbsDiff = absDiff.x;
                        snapType = GuiWindowSnap_Left;
                    }
                }
                else{
                    if(absDiff.x > maxAbsDiff){
                        maxAbsDiff = absDiff.x;
                        snapType = GuiWindowSnap_Right;
                    }
                }
                
                if(diffRelative.y < 0){
                    if(absDiff.y > maxAbsDiff){
                        maxAbsDiff = absDiff.y;
                        snapType = GuiWindowSnap_Top;
                    }
                }
                else{
                    if(absDiff.y > maxAbsDiff){
                        maxAbsDiff = absDiff.y;
                        snapType = GuiWindowSnap_Bottom;
                    }
                }
                
                // NOTE(Dima): Pushing snap rectangle
                if(snapType != GuiWindowSnap_Whole){
                    GuiSnapInWindowResult snapRes = GuiSnapInWindowRect(windowRc, snapType);
                    v4 snapColor = V4(1.0f, 0.0f, 1.0f, 0.4f);
                    PushRect(gui->stack, snapRes.result, snapColor);
                    
                    if(KeyWentDown(gui->input, MouseKey_Left)){
                        GuiSplitWindow(gui, window, 2, snapRes.rects);
                    }
                }
            }
        }
        
        // NOTE(Dima): Pushing inner outline
        PushRectInnerOutline(gui->stack, windowRc, 1, outlineColor);
    }
}

void GuiUpdateWindows(Gui_State* gui){
    Gui_Window* updateAt = gui->windowLeafSentinel.Next;
    while(updateAt != &gui->windowLeafSentinel){
        Gui_Window* tempNext = updateAt->Next;
        
        GuiUpdateWindow(gui, updateAt);
        
        updateAt = tempNext;
    }
}

void GuiBeginUpdateWindows(Gui_State* gui){
    
}

void GuiEndUpdateWindows(Gui_State* gui){
    GuiUpdateWindows(gui);
}

void GuiFrameBegin(Gui_State* gui){
    // NOTE(Dima): Init dim stack
    // NOTE(Dima): Init dim stack
    gui->dimStackIndex = 0;
    gui->inPushBlock = 0;
    
    // NOTE(Dima): Init root layout
    Gui_Element* layoutElem = GuiBeginElement(gui, 
                                              gui->rootLayout.Name, 
                                              GuiElement_Layout, 
                                              JOY_TRUE);
    GuiInitLayout(gui, &gui->rootLayout, GuiLayout_Layout, layoutElem);
}

void GuiFrameEnd(Gui_State* gui){
    // NOTE(Dima): Deinit root layout
    gui->rootLayout.At = gui->rootLayout.Start;
    GuiEndElement(gui, GuiElement_Layout);
}

void GuiFramePrepare4Render(Gui_State* gui){
    for(int tooltipIndex = 0; tooltipIndex < GUI_MAX_TOOLTIPS; tooltipIndex++){
        Gui_Tooltip* ttip= &gui->tooltips[tooltipIndex];
        
        PrintText(gui, ttip->text, ttip->at, GUI_GETCOLOR(GuiColor_Text), 1.0f);
    }
    gui->tooltipIndex = 0;
}


// NOTE(Dima): Default advance type is Column advance
inline void GuiPreAdvance(Gui_State* gui, Gui_Layout* layout){
    GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
    b32 rowStarted = ctx->type == GuiAdvanceType_Row;
    
    float rememberValue = ctx->rememberValue;
    
    if(rowStarted){
        layout->At.y = ctx->baseline;
    }
    else{
        layout->At.x = ctx->baseline;
        layout->At.y += GuiGetBaseline(gui);
    }
}

inline void GuiPostAdvance(Gui_State* gui, Gui_Layout* layout, rc2 ElementRect){
    GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
    b32 rowStarted = (ctx->type == GuiAdvanceType_Row);
    
    float RememberValue = ctx->rememberValue;
    
    float toX = ElementRect.max.x + GetScaledAscender(gui->mainFont, gui->fontScale) * 0.5f;
    float toY = ElementRect.max.y + GetLineAdvance(gui->mainFont, gui->fontScale) * 0.15f;
    
    if(rowStarted){
        layout->At.x = toX;
        ctx->maximum = Max(ctx->maximum, toY);
    }
    else{
        layout->At.y = toY;
        ctx->maximum = Max(ctx->maximum, toX);
    }
    ctx->maxHorz = Max(ctx->maxHorz, toX);
    ctx->maxVert = Max(ctx->maxVert, toY);
}


inline GuiAdvanceCtx GuiRowAdvanceCtx(float rememberX, float baseline){
    GuiAdvanceCtx ctx = {};
    
    ctx.type = GuiAdvanceType_Row;
    ctx.rememberValue = rememberX;
    ctx.baseline = baseline;
    
    return(ctx);
}

inline GuiAdvanceCtx GuiColumnAdvanceCtx(float rememberY, float baseline){
    GuiAdvanceCtx ctx = {};
    
    ctx.type = GuiAdvanceType_Column;
    ctx.rememberValue = rememberY;
    ctx.baseline = baseline;
    
    return(ctx);
}

void GuiBeginRow(Gui_State* gui){
    char name[64];
    stbsp_sprintf(name, "Row or Column: %d", gui->curElement->childCount);
    
    Gui_Element* elem = GuiBeginElement(gui, name, GuiElement_RowColumn, JOY_TRUE);
    if(GuiElementOpenedInTree(elem)){
        
        Gui_Layout* layout = GetParentLayout(gui);
        
        Assert(layout->StackCurrentIndex < ArrayCount(layout->AdvanceRememberStack));
        
        layout->AdvanceRememberStack[++layout->StackCurrentIndex] = 
            GuiRowAdvanceCtx(layout->At.x, layout->At.y + GuiGetBaseline(gui));
    }
}

void GuiBeginColumn(Gui_State* gui){
    char name[64];
    stbsp_sprintf(name, "Row or Column: %d", gui->curElement->childCount);
    
    Gui_Element* elem = GuiBeginElement(gui, name, GuiElement_RowColumn, JOY_TRUE);
    if(GuiElementOpenedInTree(elem)){
        
        Gui_Layout* layout = GetParentLayout(gui);
        
        Assert(layout->StackCurrentIndex < ArrayCount(layout->AdvanceRememberStack));
        
        layout->AdvanceRememberStack[++layout->StackCurrentIndex] = 
            GuiColumnAdvanceCtx(layout->At.y, layout->At.x);
    }
}

void GuiEndRow(Gui_State* gui){
    if(GuiElementOpenedInTree(gui->curElement)){
        
        Gui_Layout* layout = GetParentLayout(gui);
        
        Assert(layout->StackCurrentIndex >= 1);
        
        GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex--];
        Assert(ctx->type == GuiAdvanceType_Row);
        
        // NOTE(Dima): Set X value to the remembered value
        layout->At.x = ctx->rememberValue;
        // NOTE(Dima): Set Y value to the largest vertical value
        layout->At.y = ctx->maxVert;
        
        GuiAdvanceCtx* newCtx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
        newCtx->maximum = Max(ctx->maximum, newCtx->maximum);
        
        newCtx->maxHorz = Max(ctx->maxHorz, newCtx->maxHorz);
        newCtx->maxVert = Max(ctx->maxVert, newCtx->maxVert);
        
        ctx->maximum = 0.0f;
    }
    
    GuiEndElement(gui, GuiElement_RowColumn);
}

void GuiEndColumn(Gui_State* gui){
    if(GuiElementOpenedInTree(gui->curElement)){
        
        Gui_Layout* layout = GetParentLayout(gui);
        
        Assert(layout->StackCurrentIndex >= 1);
        
        GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex--];
        Assert(ctx->type == GuiAdvanceType_Column);
        
        // NOTE(Dima): Set Y Value to the remembered value
        layout->At.y = ctx->rememberValue;
        // NOTE(Dima): Set X value to the maximum horizontal value
        layout->At.x = ctx->maxHorz;
        
        GuiAdvanceCtx* newCtx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
        newCtx->maximum = Max(ctx->maximum, newCtx->maximum);
        
        newCtx->maxHorz = Max(ctx->maxHorz, newCtx->maxHorz);
        newCtx->maxVert = Max(ctx->maxVert, newCtx->maxVert);
        
        ctx->maximum = 0.0f;
    }
    
    GuiEndElement(gui, GuiElement_RowColumn);
}

enum Push_But_Type{
    PushBut_Empty,
    PushBut_Color,
    PushBut_Outline,
    PushBut_DefaultBack,
    PushBut_DefaultGrad,
    PushBut_RectAndOutline,
    PushBut_AlphaBlack,
};

INTERNAL_FUNCTION void GuiPushBut(Gui_State* gui, rc2 rect, u32 type = PushBut_DefaultGrad, v4 color = V4(0.0f, 0.0f, 0.0f, 1.0f)){
    
    switch(type){
        case PushBut_Empty:{
            
        }break;
        
        case PushBut_Color:{
            PushRect(gui->stack, rect, color);
        }break;
        
        case PushBut_AlphaBlack:{
            PushRect(gui->stack, rect, GUI_GETCOLOR(GuiColor_WindowBackground));
        }break;
        
        case PushBut_DefaultBack:{
            PushRect(gui->stack, rect, GUI_GETCOLOR(GuiColor_ButtonBackground));
        }break;
        
        case PushBut_DefaultGrad:{
            PushGradient(
                gui->stack, rect, 
                GUI_GETCOLOR(GuiColor_ButtonGrad1),
                GUI_GETCOLOR(GuiColor_ButtonGrad2),
                RenderEntryGradient_Vertical);
        }break;
        
        case PushBut_Outline:{
            PushRectOutline(gui->stack, rect, 2, color);
        }break;
        
        case PushBut_RectAndOutline:{
            PushRect(gui->stack, rect, GUI_GETCOLOR(GuiColor_ButtonBackground));
            PushRectOutline(gui->stack, rect, 1, color);
        }break;
    }
}

inline b32 PotentiallyVisible(Gui_Layout* lay, v2 dim){
    rc2 layRc = RcMinDim(lay->Start, lay->Dim);
    rc2 targetRc = RcMinDim(lay->At, dim);
    
    b32 res = BoxIntersectsWithBox(layRc, targetRc);
    
    return(res);
}

inline b32 PotentiallyVisibleSmall(Gui_Layout* lay){
    v2 dim = V2(200, 40);
    
    return(PotentiallyVisible(lay, dim));
}

void GuiTooltip(Gui_State* gui, char* tooltipText, v2 at){
    Assert(gui->tooltipIndex < GUI_MAX_TOOLTIPS);
    Gui_Tooltip* ttip = &gui->tooltips[gui->tooltipIndex++];
    
    CopyStrings(ttip->text, GUI_TOOLTIP_MAX_SIZE, tooltipText);
    ttip->at = at;
}

void GuiAnchor(Gui_State* gui, 
               char* Name, 
               v2 Pos, v2 Dim, 
               b32 Resize,
               b32 Centered, 
               v2* RectP, v2* RectDim)
{
    Gui_Element* elem = GuiBeginElement(gui, Name, GuiElement_Item, JOY_FALSE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        v2 MinP;
        if(Centered){
            MinP = Pos - Dim * 0.5f;
        }
        else{
            MinP = Pos;
        }
        
        rc2 WorkRect = RcMinDim(MinP, Dim);
        
        v4 WorkColor = GUI_GETCOLOR_COLSYS(Color_Orange);
        
        v2 MouseP = gui->input->MouseP;
        
        if(!elem->data.IsInit){
            
            elem->data.Anchor.OffsetInAnchor = {};
            
            elem->data.IsInit = JOY_TRUE;
        }
        v2* OffsetInAnchor = &elem->data.Anchor.OffsetInAnchor;
        
        Gui_Interaction* interaction = 0;
        if(Resize){
            Gui_Resize_Interaction ResizeInteraction(*RectP,
                                                     RectDim,
                                                     V2(50, 50),
                                                     Gui_Resize_Interaction_Type::Default,
                                                     elem);
            interaction = &ResizeInteraction;
        }
        else{
            Gui_Move_Interaction MoveInteraction(RectP,
                                                 Gui_Move_Interaction_Type::Move,
                                                 elem);
            
            interaction = &MoveInteraction;
        }
        
        
        if(MouseInRect(gui->input, WorkRect)){
            GuiSetHot(gui, interaction->ID, JOY_TRUE);
            
            if(KeyWentDown(gui->input, MouseKey_Left)){
                GuiSetActive(gui, interaction);
                
                *OffsetInAnchor = MouseP - Pos;
            }
        }
        else{
            GuiSetHot(gui, interaction->ID, JOY_FALSE);
        }
        
        if(KeyWentUp(gui->input, MouseKey_Left)){
            GuiReleaseInteraction(gui, interaction);
            
            *OffsetInAnchor = {};
        }
        
        v4 AnchorColor = GUI_GETCOLOR_COLSYS(Color_Orange);
        
        if(GuiIsActive(gui, interaction)){
            MouseP = MouseP - *OffsetInAnchor;
            
            ASSERT((interaction->InteractionType == GuiInteraction_Move) || 
                   (interaction->InteractionType == GuiInteraction_Resize));
            
            interaction->OffsetInAnchor = *OffsetInAnchor;
            
            interaction->Interact(gui);
            
            AnchorColor = GUI_GETCOLOR_COLSYS(Color_Blue);
        }
        
        PushRect(gui->stack, WorkRect, AnchorColor);
    }
    
    GuiEndElement(gui, GuiElement_Item);
}

void GuiBeginTree(Gui_State* gui, char* name){
    Gui_Element* elem = GuiBeginElement(gui, name, GuiElement_Item, JOY_FALSE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && PotentiallyVisibleSmall(layout))
    {
        Gui_Layout* layout = GetParentLayout(gui);
        
        GuiPreAdvance(gui, layout);
        
        rc2 textRc = GetTextRect(gui, name, layout->At);
        
        v4 textColor = GUI_GETCOLOR_COLSYS(Color_ToxicGreen);
        v4 oulineColor = GUI_GETCOLOR_COLSYS(Color_Red);
        
        GuiPushBut(gui, textRc, PushBut_DefaultGrad, oulineColor);
        if(elem->opened){
            GuiPushBut(gui, textRc, PushBut_Outline, oulineColor);
        }
        
        Gui_Empty_Interaction Interaction(elem);
        
        if(MouseInRect(gui->input, textRc)){
            GuiSetHot(gui, Interaction.ID, JOY_TRUE);
            textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(gui->input, MouseKey_Left)){
                GuiSetActive(gui, &Interaction);
                GuiReleaseInteraction(gui, &Interaction);
                
                elem->opened = !elem->opened;
            }
        }
        else{
            GuiSetHot(gui, Interaction.ID, JOY_FALSE);
        }
        PrintText(gui, name, layout->At, textColor);
        
        GuiPostAdvance(gui, layout, textRc);
    }
}
void GuiEndTree(Gui_State* gui){
    GuiEndElement(gui, GuiElement_Item);
}

void GuiText(Gui_State* gui, char* text){
    Gui_Element* elem = GuiBeginElement(gui, text, GuiElement_Item, JOY_TRUE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(gui, layout);
        
        rc2 textRc = PrintText(gui, text, layout->At, GUI_GETCOLOR(GuiColor_Text));
        
        GuiPostAdvance(gui, layout, textRc);
    }
    GuiEndElement(gui, GuiElement_Item);
}

b32 GuiButton(Gui_State* gui, char* buttonName){
    b32 result = 0;
    
    Gui_Element* elem = GuiBeginElement(gui, buttonName, GuiElement_Item, JOY_TRUE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(gui, buttonName, layout->At);
        textRc = GetTxtElemRect(gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(gui, textRc);
        
        // NOTE(Dima): Event processing
        v4 textColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
        
        Gui_Empty_Interaction Interaction(elem);
        
        if(MouseInRect(gui->input, textRc)){
            GuiSetHot(gui, Interaction.ID, JOY_TRUE);
            textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(gui->input, MouseKey_Left)){
                GuiSetActive(gui, &Interaction);
                GuiReleaseInteraction(gui, &Interaction);
                result = 1;
            }
        }
        else{
            GuiSetHot(gui, Interaction.ID, JOY_FALSE);
        }
        
        PrintTextCenteredInRect(gui, buttonName, textRc, 1.0f, textColor);
        
        GuiPostAdvance(gui, layout, textRc);
    }
    GuiEndElement(gui, GuiElement_Item);
    
    return(result);
}

void GuiBoolButton(Gui_State* gui, char* buttonName, b32* value){
    Gui_Element* elem = GuiBeginElement(gui, buttonName, GuiElement_Item, JOY_TRUE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(gui, buttonName, layout->At);
        textRc = GetTxtElemRect(gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(gui, textRc);
        
        v4 textColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
        
        // NOTE(Dima): Event processing
        if(value){
            if(*value == 0){
                textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
            }
            
            Gui_BoolInRect_Interaction Interaction(value, textRc, elem);
            Interaction.Interact(gui);
            
            if(Interaction.WasHotInInteraction){
                textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            }
        }
        
        PrintTextCenteredInRect(gui, buttonName, textRc, 1.0f, textColor);
        
        GuiPostAdvance(gui, layout, textRc);
    }
    
    GuiEndElement(gui, GuiElement_Item);
}

void GuiBoolButtonOnOff(Gui_State* gui, char* buttonName, b32* value){
    Gui_Element* elem = GuiBeginElement(gui, buttonName, GuiElement_Item, JOY_TRUE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(gui, layout);
        
        // NOTE(Dima): Button printing
        rc2 butRc = GetTextRect(gui, "OFF", layout->At);
        butRc = GetTxtElemRect(gui, layout, butRc, V2(4.0f, 3.0f));
        GuiPushBut(gui, butRc);
        
        // NOTE(Dima): Button name text printing
        float nameStartY = GetCenteredTextOffsetY(gui->mainFont, butRc, gui->fontScale);
        v2 nameStart = V2(butRc.max.x + GetScaledAscender(gui->mainFont, gui->fontScale) * 0.5f, nameStartY);
        rc2 NameRc = PrintText(gui, buttonName, nameStart, GUI_GETCOLOR(GuiColor_Text));
        
        // NOTE(Dima): Event processing
        char buttonText[4];
        CopyStrings(buttonText, "ERR");
        v4 buttonTextC = GUI_GETCOLOR(GuiColor_ButtonForeground);
        if(value){
            if(*value){
                CopyStrings(buttonText, "ON");
            }
            else{
                buttonTextC = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
                
                CopyStrings(buttonText, "OFF");
            }
            
            Gui_BoolInRect_Interaction Interaction(value, butRc, elem);
            Interaction.Interact(gui);
            
            if(Interaction.WasHotInInteraction){
                buttonTextC = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            }
        }
        
        PrintTextCenteredInRect(gui, buttonText, butRc, 1.0f, buttonTextC);
        
        rc2 AdvanceRect = GetBoundingRect(butRc, NameRc);
        GuiPostAdvance(gui, layout, AdvanceRect);
    }
    GuiEndElement(gui, GuiElement_Item);
}

void GuiCheckbox(Gui_State* gui, char* name, b32* value){
    Gui_Element* elem = GuiBeginElement(gui, name, GuiElement_Item, JOY_TRUE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        
        GuiPreAdvance(gui, layout);
        
        // NOTE(Dima): Checkbox rendering
        float chkSize = GetLineAdvance(gui->mainFont, gui->fontScale);
        rc2 chkRect;
        chkRect.min = V2(layout->At.x, layout->At.y - GetScaledAscender(gui->mainFont, gui->fontScale));
        chkRect.max = chkRect.min + V2(chkSize, chkSize);
        chkRect = GetTxtElemRect(gui, layout, chkRect, V2(2.0f, 2.0f));
        
        // NOTE(Dima): Event processing
        v4 backC = GUI_GETCOLOR(GuiColor_ButtonBackground);
        if(value){
            
            Gui_BoolInRect_Interaction Interaction(value, chkRect, elem);
            Interaction.Interact(gui);
            
            if(Interaction.WasHotInInteraction){
                backC = GUI_GETCOLOR(GuiColor_ButtonBackgroundHot);
            }
        }
        
        GuiPushBut(gui, chkRect);
        
        if(*value){
            PushGlyph(gui->stack,
                      chkRect.min, 
                      GetRectDim(chkRect),
                      gui->CheckboxMark,
                      gui->CheckboxMark->MinUV,
                      gui->CheckboxMark->MaxUV,
                      V4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        
        // NOTE(Dima): Button name text printing
        float nameStartY = GetCenteredTextOffsetY(gui->mainFont, chkRect, gui->fontScale);
        v2 nameStart = V2(chkRect.max.x + GetScaledAscender(gui->mainFont, gui->fontScale) * 0.5f, nameStartY);
        rc2 nameRc = PrintText(gui, name, nameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 advanceRect = GetBoundingRect(chkRect, nameRc);
        GuiPostAdvance(gui, layout, advanceRect);
    }
    
    GuiEndElement(gui, GuiElement_Item);
}

void GuiBeginRadioGroup(
Gui_State* gui, 
char* name, 
u32* ref, 
u32 defaultId) 
{
    Gui_Element* element = GuiBeginElement(gui, 
                                           name, 
                                           GuiElement_RadioGroup, 
                                           JOY_TRUE);
    
    if (!element->data.IsInit) {
        element->data.RadioGroup.activeId = defaultId;
        element->data.RadioGroup.ref = ref;
        
        element->data.IsInit = 1;
    }
}

INTERNAL_FUNCTION inline Gui_Element* 
GuiFindRadioGroupParent(Gui_Element* curElement) {
    Gui_Element* result = 0;
    
    Gui_Element* at = curElement;
    while (at != 0) {
        if (at->type == GuiElement_RadioGroup) {
            result = at;
            break;
        }
        
        at = at->parent;
    }
    
    return(result);
}


void GuiRadioButton(Gui_State* gui, char* name, u32 uniqueId) {
    Gui_Element* radioBut = GuiBeginElement(gui, name, GuiElement_Item, JOY_TRUE);
    Gui_Element* radioGroup = GuiFindRadioGroupParent(gui->curElement);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if (radioGroup && 
        GuiElementOpenedInTree(radioBut) && 
        PotentiallyVisibleSmall(layout)) 
    {
        b32 isActive = 0;
        if (radioGroup->data.RadioGroup.activeId == uniqueId) {
            isActive = 1;
        }
        
        
        GuiPreAdvance(gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(gui, name, layout->At);
        textRc = GetTxtElemRect(gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(gui, textRc);
        
        v4 textC;
        if(isActive){
            textC = GUI_GETCOLOR(GuiColor_ButtonForeground);
            v4 oulineColor = GUI_GETCOLOR_COLSYS(Color_Red);
            GuiPushBut(gui, textRc, PushBut_Outline, oulineColor);
        }
        else{
            textC = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
        }
        
        Gui_Empty_Interaction Interaction(radioBut);
        if (MouseInRect(gui->input, textRc)) {
            GuiSetHot(gui, Interaction.ID, JOY_TRUE);
            textC = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if (KeyWentDown(gui->input, MouseKey_Left)) {
                GuiSetActive(gui, &Interaction);
                
                if(radioGroup->data.RadioGroup.ref){
                    *radioGroup->data.RadioGroup.ref = uniqueId;
                }
                radioGroup->data.RadioGroup.activeId = uniqueId;
                
                GuiReleaseInteraction(gui, &Interaction);
            }
        }
        else{
            GuiSetHot(gui, Interaction.ID, JOY_FALSE);
        }
        
        PrintTextCenteredInRect(gui, name, textRc, 1.0f, textC);
        
        GuiPostAdvance(gui, layout, textRc);
    }
    
    GuiEndElement(gui, GuiElement_Item);
}

void GuiEndRadioGroup(Gui_State* gui) {
    GuiEndElement(gui, GuiElement_RadioGroup);
}

void GuiSliderFloat(Gui_State* gui, float* Value, float Min, float Max, char* Name){
    Gui_Element* elem = GuiBeginElement(gui, Name, GuiElement_Item, JOY_TRUE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(gui, layout);
        
        rc2 textRc = GetTextRect(gui, 
                                 "                ", 
                                 layout->At);
        textRc = GetTxtElemRect(gui, layout, textRc, V2(4.0f, 4.0f));
        GuiPushBut(gui, textRc, PushBut_Color, V4(0.05f, 0.05f, 0.1f, 1.0f));
        
        char Buf[32];
        stbsp_sprintf(Buf, "%.3f", *Value);
        v4 textC = GUI_GETCOLOR(GuiColor_ButtonForeground);
        PrintTextCenteredInRect(gui, Buf, textRc, 1.0f, textC);
        
        float NameStartY = GetCenteredTextOffsetY(gui->mainFont, textRc, gui->fontScale);
        v2 NameStart = V2(textRc.max.x + GetScaledAscender(gui->mainFont, gui->fontScale) * 0.5f, NameStartY);
        
        rc2 NameRc = PrintText(gui, Name, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, textRc);
        GuiPostAdvance(gui, layout, AdvanceRect);
    }
    
    GuiEndElement(gui, GuiElement_Item);
    
}

void GuiInputText(Gui_State* gui, char* name, char* Buf, int BufSize){
    Gui_Element* elem = GuiBeginElement(gui, name, GuiElement_Item, JOY_TRUE);
    Gui_Layout* layout = GetParentLayout(gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(gui, 
                                 "                            ", 
                                 layout->At);
        textRc = GetTxtElemRect(gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(gui, textRc, PushBut_AlphaBlack);
        
        int* CaretP = &elem->data.InputText.CaretPos;
        
        Gui_Empty_Interaction Interaction(elem);
        
        if(MouseInRect(gui->input, textRc)){
            GuiSetHot(gui, Interaction.ID, JOY_TRUE);
            
            if(KeyWentDown(gui->input, MouseKey_Left)){
                GuiSetActive(gui, &Interaction);
            }
        }
        else{
            if(KeyWentDown(gui->input, MouseKey_Left) ||
               KeyWentDown(gui->input, MouseKey_Right))
            {
                GuiReleaseInteraction(gui, &Interaction);
            }
            
            GuiSetHot(gui, Interaction.ID, JOY_FALSE);
        }
        
        if(GuiIsActive(gui, &Interaction)){
            
            char FrameInputProcessed[32];
            int FrameInputProcessedAt = 0;
            FrameInputProcessed[0] = 0;
            
            for(int i = 0; i < gui->input->FrameInputLen; i++){
                char AtChar = gui->input->FrameInput[i];
                if(AtChar >= ' ' && AtChar <= '~'){
                    FrameInputProcessed[FrameInputProcessedAt++] = AtChar;
                    FrameInputProcessed[FrameInputProcessedAt] = 0;
                }
            }
            
            char TmpBuf[256];
            *CaretP += 
                InsertStringToString(Buf, TmpBuf, BufSize, 
                                     *CaretP, 
                                     FrameInputProcessed);
            
            if(KeyIsDown(gui->input, Key_Left)){
                if(KeyIsDown(gui->input, Key_Control)){
                    for(int i = 0; i < gui->input->KeyStates[Key_Left].RepeatCount; i++){
                        int FoundIndex = FindSkipPrev(Buf, *CaretP);
                        *CaretP = FoundIndex;
                    }
                }
                else{
                    for(int i = 0; i < gui->input->KeyStates[Key_Left].RepeatCount; i++){
                        *CaretP = *CaretP - 1;
                    }
                }
            }
            
            if(KeyIsDown(gui->input, Key_Right)){
                if(KeyIsDown(gui->input, Key_Control)){
                    for(int i = 0; i < gui->input->KeyStates[Key_Right].RepeatCount; i++){
                        int FoundIndex = FindSkipNext(Buf, *CaretP);
                        *CaretP = FoundIndex;
                    }
                }
                else{
                    for(int i = 0; i < gui->input->KeyStates[Key_Right].RepeatCount; i++){
                        *CaretP = *CaretP + 1;
                    }
                }
            }
            
            if(KeyIsDown(gui->input, Key_Backspace)){
                if(KeyIsDown(gui->input, Key_Control)){
                    for(int i = 0; i < gui->input->KeyStates[Key_Backspace].RepeatCount; i++){
                        *CaretP += EraseToNonLetterPrev(Buf, *CaretP);
                    }
                }
                else{
                    for(int i = 0; i < gui->input->KeyStates[Key_Backspace].RepeatCount; i++){
                        *CaretP += ErasePrevCharFromString(Buf, *CaretP);
                    }
                }
            }
            
            
            if(KeyIsDown(gui->input, Key_Delete)){
                if(KeyIsDown(gui->input, Key_Control)){
                    for(int i = 0; i < gui->input->KeyStates[Key_Backspace].RepeatCount; i++){
                        //f***CaretP += EraseToNonLetterNext(Buf, *CaretP);
                    }
                }
                else{
                    for(int i = 0; i < gui->input->KeyStates[Key_Backspace].RepeatCount; i++){
                        *CaretP += EraseNextCharFromString(Buf, *CaretP);
                    }
                }
            }
            
            v4 oulineColor = GUI_GETCOLOR_COLSYS(Color_Red);
            GuiPushBut(gui, textRc, PushBut_Outline, oulineColor);
        }
        
        *CaretP = Max(0, *CaretP);
        *CaretP = Min(*CaretP, Min(StringLength(Buf), BufSize - 1));
        
        v4 textC = V4(1.0f, 1.0f, 1.0f, 1.0f);
        //PrintTextCenteredInRect(gui, Buf, textRc, 1.0f, textC);
        v2 BufTextSize = GetTextSize(gui, Buf, 1.0f);
        float PrintPX;
        float FieldDimX = GetRectDim(textRc).x;
        if(BufTextSize.x < FieldDimX){
            PrintPX = textRc.min.x;
        }
        else{
            PrintPX = textRc.max.x - BufTextSize.x;
        }
        float PrintPY = GetCenteredTextOffsetY(gui->mainFont,
                                               textRc, 
                                               gui->fontScale);
        v2 PrintP = V2(PrintPX, PrintPY);
        v2 CaretPrintP = GetCaretPrintP(gui, Buf, PrintP, *CaretP);
        
        v4 CaretColor = V4(0.0f, 1.0f, 0.0f, 1.0f);
        PrintCaret(gui, 
                   CaretPrintP,
                   CaretColor); 
        PrintText(gui, Buf, PrintP, GUI_GETCOLOR(GuiColor_Text));
        
        
        float nameStartY = GetCenteredTextOffsetY(gui->mainFont, textRc, gui->fontScale);
        v2 nameStart = V2(textRc.max.x + GetScaledAscender(gui->mainFont, gui->fontScale) * 0.5f, nameStartY);
        char DebugBuf[256];
        
#if 1
        stbsp_sprintf(DebugBuf, "CaretP: %d; LeftRC: %d", *CaretP, 
                      gui->input->KeyStates[Key_Left].RepeatCount);
        rc2 NameRc = PrintText(gui, DebugBuf, nameStart, GUI_GETCOLOR(GuiColor_Text));
#else
        rc2 NameRc = PrintText(gui, name, nameStart, GUI_GETCOLOR(GuiColor_Text));
#endif
        
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, textRc);
        
        GuiPostAdvance(gui, layout, AdvanceRect);
    }
    
    GuiEndElement(gui, GuiElement_Item);
}

void GuiTest(Gui_State* gui, float deltaTime){
    
    Render_Stack* renderStack = gui->stack;
    Input* input = gui->input;
    
    char FPSBuf[64];
    stbsp_sprintf(FPSBuf, "FPS %.2f, ms %.3f", 1.0f / deltaTime, deltaTime);
    
    static int lastFrameEntryCount = 0;
    static int lastFrameBytesUsed = 0;
    char StackInfo[256];
    stbsp_sprintf(StackInfo, "EntryCount: %d; BytesUsed: %d;", 
                  lastFrameEntryCount, 
                  lastFrameBytesUsed);
    
    GuiBeginPage(gui, "Page1");
    GuiEndPage(gui);
    
    GuiBeginPage(gui, "Page2");
    GuiEndPage(gui);
    
    GuiBeginPage(gui, "Animation");
    GuiEndPage(gui);
    
    GuiBeginPage(gui, "Platform");
    GuiEndPage(gui);
    
    GuiBeginPage(gui, "GUI");
    GuiEndPage(gui);
    
    //GuiUpdateWindows(gui);
    GuiBeginTree(gui, "Some text");
    GuiText(gui, "Hello world");
    GuiText(gui, FPSBuf);
    GuiText(gui, StackInfo);
    GuiText(gui, "I love Kate");
    GuiText(gui, "I wish joy and happiness for everyone");
    
    GuiEndTree(gui);
    
    //GuiPushDim(gui, V2(100, 100));
    GuiBeginLayout(gui, "layout1", GuiLayout_Window);
    static char InputTextBuf[256];
    GuiInputText(gui, "Input Text", InputTextBuf, 256);
    static float TestFloat4Slider;
    GuiSliderFloat(gui, &TestFloat4Slider, -5.0f, 10.0f, "FloatSlider");
    GuiBeginTree(gui, "AddClearButtons");
    LOCAL_AS_GLOBAL int RectCount = 0;
    GuiBeginRow(gui);
    if(GuiButton(gui, "Add")){
        RectCount++;
    }
    if(GuiButton(gui, "Clear")){
        RectCount--;
        if(RectCount < 0){
            RectCount = 0;
        }
    }
    GuiEndRow(gui);
    for(int i = 0; i < RectCount; i++){
        PushRect(renderStack, RcMinDim(V2(100 + i * 50, 100), V2(40, 40)));
    }
    GuiEndTree(gui);
    
    GuiBeginTree(gui, "Buttons and checkboxes");
    static b32 boolButtonValue;
    GuiBeginRow(gui);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBoolButton(gui, "boolButton123", &boolButtonValue);
    GuiBoolButton(gui, "boolButton1234", &boolButtonValue);
    GuiBoolButton(gui, "boolButtonasdfga", &boolButtonValue);
    GuiBoolButton(gui, "boolButtonzxcvzxcb", &boolButtonValue);
    GuiEndRow(gui);
    
    static b32 boolButtonOnOffValue;
    GuiBeginRow(gui);
    GuiBeginColumn(gui);
    GuiBoolButtonOnOff(gui, "boolButtonOnOff", &boolButtonValue);
    GuiBoolButtonOnOff(gui, "boolButtonOnOff1", &boolButtonValue);
    GuiBoolButtonOnOff(gui, "boolButtonOnOff2", &boolButtonValue);
    GuiBoolButtonOnOff(gui, "boolButtonOnOff3", &boolButtonValue);
    GuiEndColumn(gui);
    
    GuiBeginColumn(gui);
    static b32 checkboxValue1;
    static b32 checkboxValue2;
    static b32 checkboxValue3;
    static b32 checkboxValue4;
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiCheckbox(gui, "Checkbox1", &checkboxValue2);
    GuiCheckbox(gui, "Checkbox2", &checkboxValue3);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiEndColumn(gui);
    
    GuiBeginColumn(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiCheckbox(gui, "Checkbox1", &checkboxValue2);
    GuiCheckbox(gui, "Checkbox2", &checkboxValue3);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiEndColumn(gui);
    GuiEndRow(gui);
    
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff4", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox4", &checkboxValue4);
    GuiEndTree(gui);
    
    GuiBeginTree(gui, "Some more buts");
    GuiBeginRow(gui);
    GuiBeginColumn(gui);
    GuiPushDim(gui, V2(100, 40));
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff1", &boolButtonValue);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff2", &boolButtonValue);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff3", &boolButtonValue);
    GuiEndColumn(gui);
    
    GuiBeginColumn(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiCheckbox(gui, "Checkbox1", &checkboxValue2);
    GuiCheckbox(gui, "Checkbox2", &checkboxValue3);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiEndColumn(gui);
    
    GuiEndRow(gui);
    GuiEndTree(gui);
    
    GuiBeginTree(gui, "TestCull");
    static u32 activeRadio = 0;
    GuiBeginRow(gui);
    GuiBeginColumn(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiEndColumn(gui);
    GuiBeginColumn(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiEndColumn(gui);
    GuiBeginColumn(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiEndColumn(gui);
    GuiBeginColumn(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiEndColumn(gui);
    GuiBeginColumn(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiCheckbox(gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(gui, "radio0", 0);
    GuiEndRadioGroup(gui);
    GuiEndColumn(gui);
    GuiEndRow(gui);
    GuiEndTree(gui);
    
    GuiBeginRow(gui);
    GuiBeginRadioGroup(gui, "radioGroup1", &activeRadio, 0);
    GuiPushDimBegin(gui, V2(100, 40));
    GuiRadioButton(gui, "radio0", 0);
    GuiRadioButton(gui, "radio1", 1);
    GuiRadioButton(gui, "radio2", 2);
    GuiRadioButton(gui, "radio3", 3);
    GuiPushDimEnd(gui);
    GuiEndRadioGroup(gui);
    
    char radioTxt[32];
    stbsp_sprintf(radioTxt, "radio but value: %u", activeRadio);
    GuiEndRow(gui);
    GuiText(gui, radioTxt);
    
    GuiTooltip(gui, "Hello world!", input->MouseP);
    
    GuiEndLayout(gui);
    
    lastFrameBytesUsed = renderStack->MemBlock.Used;
    lastFrameEntryCount = renderStack->EntryCount;
}