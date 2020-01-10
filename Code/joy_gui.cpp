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
        
        window->prevAlloc = gui->windowFreeSentinel.prevAlloc;
        window->nextAlloc = &gui->windowFreeSentinel;
        
        window->prevAlloc->nextAlloc = window;
        window->nextAlloc->prevAlloc = window;
    }
    
    return(windowFreePoolArray);
}

inline Gui_Window* GuiPopFromReturnList(Gui_State* gui){
    Gui_Window* result = gui->windowSentinel4Returning.next;
    
    // NOTE(Dima): deleting from return list
    result->next->prev = result->prev;
    result->prev->next = result->next;
    
    result->next = 0;
    result->prev = 0;
    
    return(result);
}

inline void GuiDeallocateWindow(Gui_State* gui, Gui_Window* todo){
    todo->nextAlloc->prevAlloc = todo->prevAlloc;
    todo->prevAlloc->nextAlloc = todo->nextAlloc;
    
    todo->nextAlloc = gui->windowFreeSentinel.nextAlloc;
    todo->prevAlloc = &gui->windowFreeSentinel;
    
    todo->nextAlloc->prevAlloc = todo;
    todo->prevAlloc->nextAlloc = todo;
}

INTERNAL_FUNCTION void GuiDeallocateElement(Gui_State* gui, Gui_Element* elem)
{
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;
    
    elem->next = gui->freeSentinel.next;
    elem->prev = &gui->freeSentinel;
    
    elem->next->prev = elem;
    elem->prev->next = elem;
}

INTERNAL_FUNCTION Gui_Element* GuiAllocateElement(
Gui_State* gui)
{
    Gui_Element* result = 0;
    
    if(gui->freeSentinel.nextAlloc != &gui->freeSentinel){
        
    }
    else{
        const int count = 128;
        Gui_Element* elemPoolArray = PushArray(gui->mem, Gui_Element, count);
        
        for(int index = 0; 
            index < count;
            index++)
        {
            Gui_Element* elem = elemPoolArray + index;
            
            elem->prevAlloc = gui->freeSentinel.prevAlloc;
            elem->nextAlloc = &gui->freeSentinel;
            
            elem->prevAlloc->nextAlloc = elem;
            elem->nextAlloc->prevAlloc = elem;
        }
    }
    
    result = gui->freeSentinel.nextAlloc;
    
    // NOTE(Dima): Deallocating from free list
    result->nextAlloc->prevAlloc = result->prevAlloc;
    result->prevAlloc->nextAlloc = result->nextAlloc;
    
    // NOTE(Dima): Allocating in use list
    result->nextAlloc = &gui->useSentinel;
    result->prevAlloc = gui->useSentinel.prevAlloc;
    
    result->nextAlloc->prevAlloc = result;
    result->prevAlloc->nextAlloc = result;
    
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
        at = childSentinel->next;
        
        u32 id = StringHashFNV(name);
        
        while(at != childSentinel){
            if(id == at->id){
                found = at;
                break;
            }
            
            at = at->next;
        }
        
        // NOTE(Dima): if element was not found - then allocate and initialize
        if(!found){
            found = GuiAllocateElement(gui);
            
            // NOTE(Dima): Inserting to list
            found->next = childSentinel->next;
            found->prev = childSentinel;
            
            found->next->prev = found;
            found->prev->next = found;
            
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
            fcs->next = fcs;
            fcs->prev = fcs;
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
    elem->nextAlloc->prevAlloc = elem->prevAlloc;
    elem->prevAlloc->nextAlloc = elem->nextAlloc;
    
    elem->nextAlloc = gui->freeSentinel.nextAlloc;
    elem->prevAlloc = &gui->freeSentinel;
    
    elem->nextAlloc->prevAlloc = elem;
    elem->prevAlloc->nextAlloc = elem;
}

// NOTE(Dima): This function allocates as much windows as we need and 
// NOTE(Dima): then adds them to return list. It returns first element
// NOTE(Dima): of that list
INTERNAL_FUNCTION Gui_Window* GuiAllocateWindows(Gui_State* gui, int count)
{
    // NOTE(Dima): If free list is emty then allocate some more to it
    b32 canAllocateArray = 1;
    int canAllocateCount = count;
    Gui_Window* checkAt = gui->windowFreeSentinel.nextAlloc;
    for(int checkIndex = 0; checkIndex < count; checkIndex++){
        if(checkAt == &gui->windowFreeSentinel){
            canAllocateArray = 0;
            canAllocateCount = checkIndex;
            break;
        }
        
        checkAt = checkAt->nextAlloc;
    }
    
    int toAllocateCount = Max(128, count - canAllocateCount);
    if(!canAllocateArray){
        GuiGrowWindowFreePool(gui, gui->mem, toAllocateCount);
    }
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(gui->windowSentinel4Returning.next == &gui->windowSentinel4Returning);
    
    for(int addIndex = 0;
        addIndex < count;
        addIndex++)
    {
        // NOTE(Dima): Before in this algo we ensured that we would
        // NOTE(Dima): have as mush elements as we need. But for sure
        // NOTE(Dima): I'll double check if we can grab one more element.
        Assert(gui->windowFreeSentinel.nextAlloc != &gui->windowFreeSentinel);
        
        // NOTE(Dima): Allocating from free list
        Gui_Window* addWindow = gui->windowFreeSentinel.nextAlloc;
        
        addWindow->prevAlloc->nextAlloc = addWindow->nextAlloc;
        addWindow->nextAlloc->prevAlloc = addWindow->prevAlloc;
        
        // NOTE(Dima): Inserting to use list
        addWindow->nextAlloc = &gui->windowUseSentinel;
        addWindow->prevAlloc = gui->windowUseSentinel.prevAlloc;
        
        addWindow->nextAlloc->prevAlloc = addWindow;
        addWindow->prevAlloc->nextAlloc = addWindow;
        
        // NOTE(Dima): Inserting to return list
        addWindow->next = &gui->windowSentinel4Returning;
        addWindow->prev = gui->windowSentinel4Returning.prev;
        
        addWindow->next->prev = addWindow;
        addWindow->prev->next = addWindow;
    }
    
    Gui_Window* result = gui->windowSentinel4Returning.next;
    
    return(result);
}

INTERNAL_FUNCTION Gui_Window* GuiAllocateWindow(Gui_State* gui){
    GuiAllocateWindows(gui, 1);
    
    Gui_Window* result = GuiPopFromReturnList(gui);
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(gui->windowSentinel4Returning.next == &gui->windowSentinel4Returning);
    
    return(result);
}

inline void GuiAddWindowToList(Gui_Window* window, 
                               Gui_Window* Sentinel)
{
    window->next = Sentinel;
    window->prev = Sentinel->prev;
    
    window->next->prev = window;
    window->prev->next = window;
}

INTERNAL_FUNCTION void GuiInitRoot(Gui_State* gui, Gui_Element** root){
    
    (*root) = GuiAllocateElement(gui);
    (*root)->next = (*root);
    (*root)->prev = (*root);
    (*root)->parent = 0;
    (*root)->type = GuiElement_Root;
    CopyStrings((*root)->name, "RootElement!!!");
    (*root)->id = StringHashFNV((*root)->name);
    
    (*root)->childCount = 0;
    (*root)->childSentinel = GuiAllocateElement(gui);
    Gui_Element* rcs = (*root)->childSentinel;
    rcs->next = rcs;
    rcs->prev = rcs;
    rcs->parent = (*root);
    rcs->childSentinel = 0;
    CopyStrings(rcs->name, "RootChildrenSentinel");
    rcs->id = StringHashFNV(rcs->name);
    rcs->type = GuiElement_ChildrenSentinel;
    
}

void GuiBeginPage(Gui_State* gui, char* name){
    u32 nameID = StringHashFNV(name);
    
    Gui_Page* foundPage = 0;
    Gui_Page* pageAt = gui->rootPage.next;
    for(pageAt; pageAt != &gui->rootPage; pageAt = pageAt->next){
        if(nameID == pageAt->id){
            foundPage = pageAt;
            break;
        }
    }
    
    if(!foundPage){
        foundPage = PushStruct(gui->mem, Gui_Page);
        
        CopyStrings(foundPage->name, sizeof(foundPage->name), name);
        foundPage->id = nameID;
        
        foundPage->next = gui->rootPage.next;
        foundPage->prev = &gui->rootPage;
        foundPage->next->prev = foundPage;
        foundPage->prev->next = foundPage;
        
        ++gui->pageCount;
    }
    
    // NOTE(Dima): Init page element
    Gui_Element* pageElem = GuiBeginElement(gui, 
                                            name,
                                            GuiElement_Page,
                                            JOY_TRUE);
    // NOTE(Dima): Init references
    pageElem->data.page.ref = foundPage;
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
    gui->checkboxMark = &assets->checkboxMark;
    gui->fontScale = 1.0f;
    
    gui->stack = stack;
    gui->input = input;
    gui->mem = mem;
    
    gui->width = width;
    gui->height = height;
    
    // NOTE(Dima): Init layouts
    gui->layoutCount = 1;
    gui->rootLayout = {};
    gui->rootLayout.next = &gui->rootLayout;
    gui->rootLayout.prev = &gui->rootLayout;
    CopyStrings(gui->rootLayout.name, "RootLayout");
    gui->rootLayout.id = StringHashFNV(gui->rootLayout.name);
    
    // NOTE(Dima): Init dim stack
    for(int dimIndex = 0; dimIndex < ARRAY_COUNT(gui->dimStack); dimIndex++){
        gui->dimStack[dimIndex] = {};
    }
    gui->dimStackIndex = 0;
    gui->inPushBlock = 0;
    
    
    // NOTE(Dima): Init pages
    gui->pageCount = 1;
    gui->rootPage = {};
    gui->rootPage.next = &gui->rootPage;
    gui->rootPage.prev = &gui->rootPage;
    CopyStrings(gui->rootPage.name, "RootPage");
    gui->rootPage.id = StringHashFNV(gui->rootPage.name);
    
    // NOTE(Dima): Initializing of window free pool and sentinels
    gui->windowUseSentinel.nextAlloc = &gui->windowUseSentinel;
    gui->windowUseSentinel.prevAlloc = &gui->windowUseSentinel;
    gui->windowFreeSentinel.nextAlloc = &gui->windowFreeSentinel;
    gui->windowFreeSentinel.prevAlloc = &gui->windowFreeSentinel;
    
    GuiGrowWindowFreePool(gui, mem, 128);
    
    // NOTE(Dima): Init window sentinel for returning windows
    // NOTE(Dima): as list when we allocate multiple of them.
    gui->windowSentinel4Returning.next = &gui->windowSentinel4Returning;
    gui->windowSentinel4Returning.prev = &gui->windowSentinel4Returning;
    
    // NOTE(Dima): Init window leaf sentinel
    gui->windowLeafSentinel.next = &gui->windowLeafSentinel;
    gui->windowLeafSentinel.prev = &gui->windowLeafSentinel;
    
    gui->tempWindow1 = GuiAllocateWindow(gui);
    gui->tempWindow1->rect = RcMinDim(V2(10, 10), V2(1000, 600));
    gui->tempWindow1->visible = 1;
    GuiAddWindowToList(gui->tempWindow1, &gui->windowLeafSentinel);
    
    // NOTE(Dima): Initializing elements sentinel
    gui->freeSentinel.nextAlloc = &gui->freeSentinel;
    gui->freeSentinel.prevAlloc = &gui->freeSentinel;
    
    gui->useSentinel.nextAlloc = &gui->useSentinel;
    gui->useSentinel.prevAlloc = &gui->useSentinel;
    
    // NOTE(Dima): Initializing root element
    GuiInitRoot(gui, &gui->rootElement);
    
    // NOTE(Dima): Setting current element
    gui->curElement = gui->rootElement;
    
    // NOTE(Dima): Initializing colors
    InitColorsState(&gui->colorState, mem);
    gui->colors[GuiColor_Text] = GUI_GETCOLOR_COLSYS(Color_White);
    gui->colors[GuiColor_HotText] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    gui->colors[GuiColor_Borders] = GUI_GETCOLOR_COLSYS(Color_Black);
    
    gui->colors[GuiColor_ButtonBackground] = GUI_COLORHEX("#337733");
    gui->colors[GuiColor_ButtonBackgroundHot] = GUI_GETCOLOR_COLSYS(Color_Cyan);
    gui->colors[GuiColor_ButtonForeground] = GUI_GETCOLOR_COLSYS(Color_White);
    gui->colors[GuiColor_ButtonForegroundHot] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    gui->colors[GuiColor_ButtonForegroundDisabled] = gui->colors[GuiColor_ButtonForeground] * 0.75f;
    gui->colors[GuiColor_ButtonGrad1] = ColorFromHex("#2277DD");
    gui->colors[GuiColor_ButtonGrad2] = ColorFromHex("#4b0082");
    
    gui->windowAlpha = 0.85f;
    gui->colors[GuiColor_WindowBackground] = V4(0.0f, 0.0f, 0.0f, gui->windowAlpha);
    gui->colors[GuiColor_WindowBorder] = GUI_GETCOLOR_COLSYS(Color_Black);
    gui->colors[GuiColor_WindowBorderHot] = GUI_GETCOLOR_COLSYS(Color_Magenta);
    gui->colors[GuiColor_WindowBorderActive] = GUI_GETCOLOR_COLSYS(Color_Blue);
}

rc2 PrintTextInternal(Font_Info* font, Render_Stack* stack, char* text, v2 p, u32 textOp, float scale, v4 color){
    rc2 txtRc;
    
    char* at = text;
    
    v2 curP = p;
    
    while(*at){
        
        int glyphIndex = font->Codepoint2Glyph[*at];
        Glyph_Info* glyph = &font->Glyphs[glyphIndex];
        
        float bmpScale = glyph->Height * scale;
        
        v2 bitmapDim = { glyph->Bitmap.WidthOverHeight * bmpScale, bmpScale };
        
        if(textOp == PrintTextOp_Print){
            float bitmapMinY = curP.y + glyph->YOffset * scale;
            float bitmapMinX = curP.x + glyph->XOffset * scale;
            
            PushBitmap(stack, &glyph->Bitmap, V2(bitmapMinX, bitmapMinY), bitmapDim.y, color);
        }
        
        curP.x += ((float)glyph->Advance * scale);
        
        *at++;
    }
    
    txtRc.min.x = p.x;
    txtRc.min.y = p.y - font->AscenderHeight * scale;
    txtRc.max.x = curP.x;
    txtRc.max.y = curP.y - font->DescenderHeight * scale;
    
    return(txtRc);
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

rc2 PrintTextCenteredInRect(Gui_State* gui, char* text, rc2 rect, float scale, v4 color){
    rc2 result = PrintTextCenteredInRectInternal(gui->mainFont,
                                                 gui->stack,
                                                 text, 
                                                 rect,
                                                 gui->fontScale * scale,
                                                 color);
    
    return(result);
}

INTERNAL_FUNCTION void GuiInitLayout(Gui_State* gui, Gui_Layout* layout, u32 layoutType, Gui_Element* layoutElem){
    // NOTE(Dima): initializing references
    layoutElem->data.layout.ref = layout;
    layout->elem = layoutElem;
    
    v2 popDim;
    if(!GuiPopDim(gui, &popDim)){
        popDim = V2(640, 480);
    }
    
    // NOTE(Dima): Layout initializing
    layout->type = layoutType;
    if(!layoutElem->data.isInit){
        layout->start = V2(100.0f, 100.0f) * (gui->layoutCount - 1);
        layout->at = layout->start;
        
        switch(layoutType){
            case GuiLayout_Layout:{
                layout->dim = V2(gui->width, gui->height);
            }break;
            
            case GuiLayout_Window:{
                layout->dim = popDim;
            }break;
        }
        
        layoutElem->data.isInit = JOY_TRUE;
    }
    
    // NOTE(Dima): Initializing initial advance ctx
    layout->advanceRememberStack[0].type = GuiAdvanceType_Column;
    layout->advanceRememberStack[0].rememberValue = layout->start.y;
    layout->advanceRememberStack[0].baseline = layout->start.x;
    
    if(layoutType == GuiLayout_Window){
        rc2 windowRc = RcMinDim(layout->start, layout->dim);
        PushRect(gui->stack, windowRc, GUI_GETCOLOR(GuiColor_WindowBackground));
        
        v4 outlineColor = GUI_GETCOLOR(GuiColor_WindowBorder);
        if(MouseInRect(gui->input, windowRc)){
            outlineColor = GUI_GETCOLOR(GuiColor_WindowBorderHot);
        }
        
        // NOTE(Dima): Pushing inner outline
        PushRectOutline(gui->stack, windowRc, 2, outlineColor);
    }
}

void GuiBeginLayout(Gui_State* gui, char* name, u32 layoutType){
    // NOTE(Dima): In list inserting
    u32 nameID = StringHashFNV(name);
    
    Gui_Layout* foundLayout = 0;
    Gui_Layout* layoutAt = gui->rootLayout.next;
    for(layoutAt; layoutAt != &gui->rootLayout; layoutAt = layoutAt->next){
        if(nameID == layoutAt->id){
            foundLayout = layoutAt;
            break;
        }
    }
    
    if(!foundLayout){
        foundLayout = PushStruct(gui->mem, Gui_Layout);
        
        CopyStrings(foundLayout->name, sizeof(foundLayout->name), name);
        foundLayout->id = nameID;
        
        foundLayout->next = gui->rootLayout.next;
        foundLayout->prev = &gui->rootLayout;
        foundLayout->next->prev = foundLayout;
        foundLayout->prev->next = foundLayout;
        
        ++gui->layoutCount;
    }
    
    // NOTE(Dima): Beginnning layout elem
    Gui_Element* layoutElem = GuiBeginElement(gui, name, GuiElement_Layout, JOY_TRUE);
    
    GuiInitLayout(gui, foundLayout, layoutType, layoutElem);
}

void GuiEndLayout(Gui_State* gui){
    Gui_Layout* lay = GetParentLayout(gui);
    
    lay->at = lay->start;
    
    GuiEndElement(gui, GuiElement_Layout);
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
        
        default:{
            INVALID_CODE_PATH;
        }break;
    }
    
    return(result);
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
        newWindow->next = gui->windowLeafSentinel.next;
        newWindow->prev = &gui->windowLeafSentinel;
        
        newWindow->next->prev = newWindow;
        newWindow->prev->next = newWindow;
        
        newWindow->rect = partsRects[newWindowIndex];
    }
    
    // NOTE(Dima): Deallocating parent because it is not visible
    window->next->prev = window->prev;
    window->prev->next = window->next;
    
    window->next = 0;
    window->prev = 0;
    
    GuiDeallocateWindow(gui, window);
    
    // NOTE(Dima): Return list shoud be empty after usage in this function
    Assert(gui->windowSentinel4Returning.next == &gui->windowSentinel4Returning);
}

INTERNAL_FUNCTION void GuiUpdateWindow(Gui_State* gui, Gui_Window* window){
    
    window->layout.start = window->rect.min;
    
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
                
                v2 mouseP = gui->input->mouseP;
                mouseP = ClampInRect(mouseP, windowRc);
                v2 diffFromCenter = mouseP - windowCenter;
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
    Gui_Window* updateAt = gui->windowLeafSentinel.next;
    while(updateAt != &gui->windowLeafSentinel){
        Gui_Window* tempNext = updateAt->next;
        
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
                                              gui->rootLayout.name, 
                                              GuiElement_Layout, 
                                              JOY_TRUE);
    GuiInitLayout(gui, &gui->rootLayout, GuiLayout_Layout, layoutElem);
}

void GuiFrameEnd(Gui_State* gui){
    // NOTE(Dima): Deinit root layout
    gui->rootLayout.at = gui->rootLayout.start;
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
    GuiAdvanceCtx* ctx = &layout->advanceRememberStack[layout->stackCurrentIndex];
    b32 rowStarted = ctx->type == GuiAdvanceType_Row;
    
    float rememberValue = ctx->rememberValue;
    
    if(rowStarted){
        layout->at.y = ctx->baseline;
    }
    else{
        layout->at.x = ctx->baseline;
        layout->at.y += GuiGetBaseline(gui);
    }
}

inline void GuiPostAdvance(Gui_State* gui, Gui_Layout* layout, rc2 ElementRect){
    GuiAdvanceCtx* ctx = &layout->advanceRememberStack[layout->stackCurrentIndex];
    b32 rowStarted = (ctx->type == GuiAdvanceType_Row);
    
    float RememberValue = ctx->rememberValue;
    
    float toX = ElementRect.max.x + GetScaledAscender(gui->mainFont, gui->fontScale) * 0.5f;
    float toY = ElementRect.max.y + GetLineAdvance(gui->mainFont, gui->fontScale) * 0.15f;
    
    if(rowStarted){
        layout->at.x = toX;
        ctx->maximum = Max(ctx->maximum, toY);
    }
    else{
        layout->at.y = toY;
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
        
        Assert(layout->stackCurrentIndex < ArrayCount(layout->advanceRememberStack));
        
        layout->advanceRememberStack[++layout->stackCurrentIndex] = 
            GuiRowAdvanceCtx(layout->at.x, layout->at.y + GuiGetBaseline(gui));
    }
}

void GuiBeginColumn(Gui_State* gui){
    char name[64];
    stbsp_sprintf(name, "Row or Column: %d", gui->curElement->childCount);
    
    Gui_Element* elem = GuiBeginElement(gui, name, GuiElement_RowColumn, JOY_TRUE);
    if(GuiElementOpenedInTree(elem)){
        
        Gui_Layout* layout = GetParentLayout(gui);
        
        Assert(layout->stackCurrentIndex < ArrayCount(layout->advanceRememberStack));
        
        layout->advanceRememberStack[++layout->stackCurrentIndex] = 
            GuiColumnAdvanceCtx(layout->at.y, layout->at.x);
    }
}

void GuiEndRow(Gui_State* gui){
    if(GuiElementOpenedInTree(gui->curElement)){
        
        Gui_Layout* layout = GetParentLayout(gui);
        
        Assert(layout->stackCurrentIndex >= 1);
        
        GuiAdvanceCtx* ctx = &layout->advanceRememberStack[layout->stackCurrentIndex--];
        Assert(ctx->type == GuiAdvanceType_Row);
        
        // NOTE(Dima): Set X value to the remembered value
        layout->at.x = ctx->rememberValue;
        // NOTE(Dima): Set Y value to the largest vertical value
        layout->at.y = ctx->maxVert;
        
        GuiAdvanceCtx* newCtx = &layout->advanceRememberStack[layout->stackCurrentIndex];
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
        
        Assert(layout->stackCurrentIndex >= 1);
        
        GuiAdvanceCtx* ctx = &layout->advanceRememberStack[layout->stackCurrentIndex--];
        Assert(ctx->type == GuiAdvanceType_Column);
        
        // NOTE(Dima): Set Y Value to the remembered value
        layout->at.y = ctx->rememberValue;
        // NOTE(Dima): Set X value to the maximum horizontal value
        layout->at.x = ctx->maxHorz;
        
        GuiAdvanceCtx* newCtx = &layout->advanceRememberStack[layout->stackCurrentIndex];
        newCtx->maximum = Max(ctx->maximum, newCtx->maximum);
        
        newCtx->maxHorz = Max(ctx->maxHorz, newCtx->maxHorz);
        newCtx->maxVert = Max(ctx->maxVert, newCtx->maxVert);
        
        ctx->maximum = 0.0f;
    }
    
    GuiEndElement(gui, GuiElement_RowColumn);
}

enum Push_But_Type{
    PushBut_Empty,
    PushBut_Back,
    PushBut_Grad,
    PushBut_Outline,
};

INTERNAL_FUNCTION void GuiPushBut(Gui_State* gui, rc2 rect, u32 type = PushBut_Grad, v4 color = V4(0.0f, 0.0f, 0.0f, 0.0f)){
    
    
    
    switch(type){
        case PushBut_Empty:{
            
        }break;
        
        case PushBut_Back:{
            PushRect(gui->stack, rect, GUI_GETCOLOR(GuiColor_ButtonBackground));
        }break;
        
        case PushBut_Grad:{
            PushGradient(
                gui->stack, rect, 
                GUI_GETCOLOR(GuiColor_ButtonGrad1),
                GUI_GETCOLOR(GuiColor_ButtonGrad2),
                RenderEntryGradient_Vertical);
        }break;
        
        case PushBut_Outline:{
            PushRectOutline(gui->stack, rect, 2, color);
        }break;
    }
}

inline b32 PotentiallyVisible(Gui_Layout* lay, v2 dim){
    rc2 layRc = RcMinDim(lay->start, lay->dim);
    rc2 targetRc = RcMinDim(lay->at, dim);
    
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


void GuiBeginTree(Gui_State* gui, char* name){
    Gui_Element* elem = GuiBeginElement(gui, name, GuiElement_Item, JOY_FALSE);
    
    if(GuiElementOpenedInTree(elem)){
        Gui_Layout* layout = GetParentLayout(gui);
        
        GuiPreAdvance(gui, layout);
        
        rc2 textRc = GetTextRect(gui, name, layout->at);
        
        v4 textColor = GUI_GETCOLOR_COLSYS(Color_ToxicGreen);
        v4 oulineColor = GUI_GETCOLOR_COLSYS(Color_Red);
        
        GuiPushBut(gui, textRc, PushBut_Grad, oulineColor);
        if(elem->opened){
            GuiPushBut(gui, textRc, PushBut_Outline, oulineColor);
        }
        
        if(MouseInRect(gui->input, textRc)){
            textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(gui->input, MouseKey_Left)){
                elem->opened = !elem->opened;
            }
        }
        PrintText(gui, name, layout->at, textColor);
        
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
        
        rc2 textRc = PrintText(gui, text, layout->at, GUI_GETCOLOR(GuiColor_Text));
        
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
        rc2 textRc = GetTextRect(gui, buttonName, layout->at);
        textRc = GetTxtElemRect(gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(gui, textRc);
        
        // NOTE(Dima): Event processing
        v4 textColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
        if(MouseInRect(gui->input, textRc)){
            textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(gui->input, MouseKey_Left)){
                result = 1;
            }
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
        rc2 textRc = GetTextRect(gui, buttonName, layout->at);
        textRc = GetTxtElemRect(gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(gui, textRc);
        
        v4 textColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
        
        // NOTE(Dima): Event processing
        if(value){
            if(*value == 0){
                textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
            }
            
            if(MouseInRect(gui->input, textRc)){
                textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
                
                if(KeyWentDown(gui->input, MouseKey_Left)){
                    *value = !*value;
                }
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
        rc2 butRc = GetTextRect(gui, "OFF", layout->at);
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
            
            if(MouseInRect(gui->input, butRc)){
                buttonTextC = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
                
                if(KeyWentDown(gui->input, MouseKey_Left)){
                    *value = !*value;
                }
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
        chkRect.min = V2(layout->at.x, layout->at.y - GetScaledAscender(gui->mainFont, gui->fontScale));
        chkRect.max = chkRect.min + V2(chkSize, chkSize);
        chkRect = GetTxtElemRect(gui, layout, chkRect, V2(2.0f, 2.0f));
        
        // NOTE(Dima): Event processing
        v4 backC = GUI_GETCOLOR(GuiColor_ButtonBackground);
        if(value){
            if(MouseInRect(gui->input, chkRect)){
                backC = GUI_GETCOLOR(GuiColor_ButtonBackgroundHot);
                
                if(KeyWentDown(gui->input, MouseKey_Left)){
                    *value = !*value;
                }
            }
        }
        
        GuiPushBut(gui, chkRect);
        
        if(*value){
            PushBitmap(gui->stack, gui->checkboxMark, chkRect.min, GetRectHeight(chkRect), V4(1.0f, 1.0f, 1.0f, 1.0f));
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
    
    if (!element->data.isInit) {
        element->data.radioGroup.activeId = defaultId;
        element->data.radioGroup.ref = ref;
        
        element->data.isInit = 1;
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
        if (radioGroup->data.radioGroup.activeId == uniqueId) {
            isActive = 1;
        }
        
        
        GuiPreAdvance(gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(gui, name, layout->at);
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
        
        if (MouseInRect(gui->input, textRc)) {
            textC = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if (KeyWentDown(gui->input, MouseKey_Left)) {
                
                if(radioGroup->data.radioGroup.ref){
                    *radioGroup->data.radioGroup.ref = uniqueId;
                }
                radioGroup->data.radioGroup.activeId = uniqueId;
            }
        }
        
        PrintTextCenteredInRect(gui, name, textRc, 1.0f, textC);
        
        GuiPostAdvance(gui, layout, textRc);
    }
    
    GuiEndElement(gui, GuiElement_Item);
}

void GuiEndRadioGroup(Gui_State* gui) {
    GuiEndElement(gui, GuiElement_RadioGroup);
}
