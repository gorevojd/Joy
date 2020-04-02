#include "joy_gui.h"
#include "joy_defines.h"
#include "joy_assets_render.h"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#include "stb_sprintf.h"

// NOTE(Dima): Returns newly pushed window array
INTERNAL_FUNCTION Gui_Window* GuiGrowWindowFreePool(gui_state* Gui, mem_region* mem,  int count){
    Gui_Window* windowFreePoolArray = PushArray(mem, Gui_Window, count);
    
    for(int index = 0; 
        index < count;
        index++)
    {
        Gui_Window* window = windowFreePoolArray + index;
        
        *window = {};
        
        window->PrevAlloc = Gui->windowFreeSentinel.PrevAlloc;
        window->NextAlloc = &Gui->windowFreeSentinel;
        
        window->PrevAlloc->NextAlloc = window;
        window->NextAlloc->PrevAlloc = window;
    }
    
    return(windowFreePoolArray);
}

inline Gui_Window* GuiPopFromReturnList(gui_state* Gui){
    Gui_Window* result = Gui->windowSentinel4Returning.Next;
    
    // NOTE(Dima): deleting from return list
    result->Next->Prev = result->Prev;
    result->Prev->Next = result->Next;
    
    result->Next = 0;
    result->Prev = 0;
    
    return(result);
}

inline void GuiDeallocateWindow(gui_state* Gui, Gui_Window* todo){
    todo->NextAlloc->PrevAlloc = todo->PrevAlloc;
    todo->PrevAlloc->NextAlloc = todo->NextAlloc;
    
    todo->NextAlloc = Gui->windowFreeSentinel.NextAlloc;
    todo->PrevAlloc = &Gui->windowFreeSentinel;
    
    todo->NextAlloc->PrevAlloc = todo;
    todo->PrevAlloc->NextAlloc = todo;
}

INTERNAL_FUNCTION inline void GuiRemoveElementFromList(gui_element* Elem){
    Elem->Next->Prev = Elem->Prev;
    Elem->Prev->Next = Elem->Next;
}

INTERNAL_FUNCTION inline void GuiDeallocateElement(gui_state* Gui, gui_element* elem)
{
    // NOTE(Dima): Remove from Use list
    elem->NextAlloc->PrevAlloc = elem->PrevAlloc;
    elem->PrevAlloc->NextAlloc = elem->NextAlloc;
    
    // NOTE(Dima): Insert to Free list
    elem->NextAlloc = Gui->freeSentinel.NextAlloc;
    elem->PrevAlloc = &Gui->freeSentinel;
    
    elem->NextAlloc->PrevAlloc = elem;
    elem->PrevAlloc->NextAlloc = elem;
}

INTERNAL_FUNCTION gui_element* GuiAllocateElement(
gui_state* Gui)
{
    gui_element* result = 0;
    
    if(DLIST_FREE_IS_EMPTY(Gui->freeSentinel, NextAlloc)){
        const int count = 128;
        gui_element* elemPoolArray = PushArray(Gui->Mem, gui_element, count);
        
        for(int index = 0; 
            index < count;
            index++)
        {
            gui_element* elem = &elemPoolArray[index];
            
            DLIST_INSERT_BEFORE_SENTINEL(elem, Gui->freeSentinel, NextAlloc, PrevAlloc);
        }
        
        Gui->TotalAllocatedGuiElements += count;
    }
    
    result = Gui->freeSentinel.NextAlloc;
    
    // NOTE(Dima): Deallocating from Free list
    DLIST_REMOVE_ENTRY(result, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Allocating in Use list
    DLIST_INSERT_BEFORE_SENTINEL(result, Gui->useSentinel, NextAlloc, PrevAlloc);
    
    return(result);
}

INTERNAL_FUNCTION gui_element* GuiInitElement(gui_state* Gui,
                                              char* Name,
                                              gui_element** cur,
                                              u32 type, 
                                              b32 initOpened)
{
    gui_element* result = 0;
    
    // NOTE(Dima): Try find element in hierarchy
    gui_element* childSentinel = (*cur)->childSentinel;
    gui_element* at = 0;
    gui_element* found = 0;
    if(childSentinel){
        at = childSentinel->Next;
        
        u32 ID = StringHashFNV(Name);
        u32 InTreeID = ID;
        if(at->parent){
            InTreeID *= at->parent->ID + 7853;
        }
        
        while(at != childSentinel){
            if(ID == at->ID){
                found = at;
                break;
            }
            
            at = at->Next;
        }
        
        // NOTE(Dima): if element was not found - then allocate and initialize
        if(!found){
            found = GuiAllocateElement(Gui);
            
            found->ID = ID;
            CopyStringsSafe(found->Name, sizeof(found->Name), Name);
            
            found->parent = *cur;
            found->type = type;
            
            // NOTE(Dima): freeing data
            found->Data = {};
            
            // NOTE(Dima): Initializing children sentinel
            if(type == GuiElement_ChildrenSentinel ||
               type == GuiElement_TempItem ||
               type == GuiElement_None)
            {
                found->childSentinel = 0;
            }
            else{
                found->childSentinel = GuiAllocateElement(Gui);
                gui_element* fcs = found->childSentinel;
                fcs->Next = fcs;
                fcs->Prev = fcs;
                fcs->parent = found;
                fcs->childSentinel = 0;
                CopyStrings(fcs->Name, "ChildrenSentinel");
                fcs->ID = StringHashFNV(fcs->Name);
                fcs->type = GuiElement_ChildrenSentinel;
            }
            
            // NOTE(Dima): Initializing opened
            found->Opened = initOpened;
            
            // NOTE(Dima): Inserting to list
            found->Next = childSentinel->Next;
            found->Prev = childSentinel;
            
            found->Next->Prev = found;
            found->Prev->Next = found;
        }
        
        ASSERT(found->type == type);
        
        *cur = found;
        
        result = found;
        
        // NOTE(Dima): Incrementing parent childCount
        if(found->parent){
            found->parent->childCount++;
        }
        found->childCount = 0;
        
        // NOTE(Dima): Incrementing temp counts for row column elements
        if(result->parent){
            ++result->parent->TmpCount;
        }
        result->TmpCount = 0;
    }
    
    return(result);
}


INTERNAL_FUNCTION gui_element* GuiBeginElement(gui_state* Gui,
                                               char* name,
                                               u32 type,
                                               b32 opened)
{
    gui_element* Result = GuiInitElement(Gui, 
                                         name, 
                                         &Gui->curElement, 
                                         type,
                                         opened);
    
    return(Result);
}

INTERNAL_FUNCTION b32 GuiElementOpenedInTree(gui_element* elem){
    b32 Result = 1;
    gui_element* at = elem->parent;
    
    while(at->parent != 0){
        if(at->Opened != 1){
            Result = 0;
            break;
        }
        
        at = at->parent;
    }
    
    return(Result);
}

INTERNAL_FUNCTION void GuiEndElement(gui_state* Gui, u32 type)
{
    ASSERT(Gui->curElement->type == type);
    
    gui_element* TmpParent = Gui->curElement->parent;
    int TmpCount = Gui->curElement->TmpCount;
    
    if(type == GuiElement_ChildrenSentinel ||
       type == GuiElement_TempItem ||
       type == GuiElement_None)
    {
        GuiRemoveElementFromList(Gui->curElement);
        GuiDeallocateElement(Gui, Gui->curElement);
    }
    
    Gui->curElement->TmpCount = TmpCount;
    Gui->curElement = TmpParent;
}

INTERNAL_FUNCTION void GuiFreeElement(gui_state* Gui,
                                      gui_element* elem)
{
    elem->NextAlloc->PrevAlloc = elem->PrevAlloc;
    elem->PrevAlloc->NextAlloc = elem->NextAlloc;
    
    elem->NextAlloc = Gui->freeSentinel.NextAlloc;
    elem->PrevAlloc = &Gui->freeSentinel;
    
    elem->NextAlloc->PrevAlloc = elem;
    elem->PrevAlloc->NextAlloc = elem;
}

// NOTE(Dima): This function allocates as much windows as we need and 
// NOTE(Dima): then adds them to return list. It returns first element
// NOTE(Dima): of that list
INTERNAL_FUNCTION Gui_Window* GuiAllocateWindows(gui_state* Gui, int count)
{
    // NOTE(Dima): If free list is emty then allocate some more to it
    b32 canAllocateArray = 1;
    int canAllocateCount = count;
    Gui_Window* checkAt = Gui->windowFreeSentinel.NextAlloc;
    for(int checkIndex = 0; checkIndex < count; checkIndex++){
        if(checkAt == &Gui->windowFreeSentinel){
            canAllocateArray = 0;
            canAllocateCount = checkIndex;
            break;
        }
        
        checkAt = checkAt->NextAlloc;
    }
    
    int toAllocateCount = Max(128, count - canAllocateCount);
    if(!canAllocateArray){
        GuiGrowWindowFreePool(Gui, Gui->Mem, toAllocateCount);
    }
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(Gui->windowSentinel4Returning.Next == &Gui->windowSentinel4Returning);
    
    for(int addIndex = 0;
        addIndex < count;
        addIndex++)
    {
        // NOTE(Dima): Before in this algo we ensured that we would
        // NOTE(Dima): have as mush elements as we need. But for sure
        // NOTE(Dima): I'll double check if we can grab one more element.
        Assert(Gui->windowFreeSentinel.NextAlloc != &Gui->windowFreeSentinel);
        
        // NOTE(Dima): Allocating from free list
        Gui_Window* addWindow = Gui->windowFreeSentinel.NextAlloc;
        
        addWindow->PrevAlloc->NextAlloc = addWindow->NextAlloc;
        addWindow->NextAlloc->PrevAlloc = addWindow->PrevAlloc;
        
        // NOTE(Dima): Inserting to use list
        addWindow->NextAlloc = &Gui->windowUseSentinel;
        addWindow->PrevAlloc = Gui->windowUseSentinel.PrevAlloc;
        
        addWindow->NextAlloc->PrevAlloc = addWindow;
        addWindow->PrevAlloc->NextAlloc = addWindow;
        
        // NOTE(Dima): Inserting to return list
        addWindow->Next = &Gui->windowSentinel4Returning;
        addWindow->Prev = Gui->windowSentinel4Returning.Prev;
        
        addWindow->Next->Prev = addWindow;
        addWindow->Prev->Next = addWindow;
    }
    
    Gui_Window* result = Gui->windowSentinel4Returning.Next;
    
    return(result);
}

INTERNAL_FUNCTION Gui_Window* GuiAllocateWindow(gui_state* Gui){
    GuiAllocateWindows(Gui, 1);
    
    Gui_Window* result = GuiPopFromReturnList(Gui);
    
    // NOTE(Dima): Return list shoud be empty before return
    Assert(Gui->windowSentinel4Returning.Next == &Gui->windowSentinel4Returning);
    
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

INTERNAL_FUNCTION void GuiInitRoot(gui_state* Gui, gui_element** root){
    
    (*root) = GuiAllocateElement(Gui);
    (*root)->Next = (*root);
    (*root)->Prev = (*root);
    (*root)->parent = 0;
    (*root)->type = GuiElement_Root;
    CopyStrings((*root)->Name, "RootElement!!!");
    (*root)->ID = StringHashFNV((*root)->Name);
    
    (*root)->childCount = 0;
    (*root)->childSentinel = GuiAllocateElement(Gui);
    gui_element* rcs = (*root)->childSentinel;
    rcs->Next = rcs;
    rcs->Prev = rcs;
    rcs->parent = (*root);
    rcs->childSentinel = 0;
    CopyStrings(rcs->Name, "RootChildrenSentinel");
    rcs->ID = StringHashFNV(rcs->Name);
    rcs->type = GuiElement_ChildrenSentinel;
}

void GuiBeginPage(gui_state* Gui, char* name){
    u32 nameID = StringHashFNV(name);
    
    Gui_Page* foundPage = 0;
    Gui_Page* pageAt = Gui->rootPage.Next;
    for(pageAt; pageAt != &Gui->rootPage; pageAt = pageAt->Next){
        if(nameID == pageAt->id){
            foundPage = pageAt;
            break;
        }
    }
    
    if(!foundPage){
        foundPage = PushStruct(Gui->Mem, Gui_Page);
        
        CopyStringsSafe(foundPage->name, sizeof(foundPage->name), name);
        foundPage->id = nameID;
        
        foundPage->Next = Gui->rootPage.Next;
        foundPage->Prev = &Gui->rootPage;
        foundPage->Next->Prev = foundPage;
        foundPage->Prev->Next = foundPage;
        
        ++Gui->pageCount;
    }
    
    // NOTE(Dima): Init page element
    gui_element* pageElem = GuiBeginElement(Gui, 
                                            name,
                                            GuiElement_Page,
                                            true);
    // NOTE(Dima): Init references
    pageElem->Data.Page.ref = foundPage;
    foundPage->elem = pageElem;
}

void GuiEndPage(gui_state* Gui){
    GuiEndElement(Gui, GuiElement_Page);
}

void InitGui(
gui_state* Gui, 
assets* Assets)
{
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): memory region is already initialized
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): InitAssets
    Gui->Assets = Assets;
    
    // NOTE(Dima): Getting IDs for needed assets
    Gui->MainFontID = GetFirst(Assets, GameAsset_Inconsolata);
    Gui->TileFontID = GetFirst(Assets, GameAsset_MollyJackFont);
    
    Gui->MainFont = PushOrLoadFont(Assets, Gui->MainFontID, ASSET_LOAD_IMMEDIATE);
    Gui->TileFont = PushOrLoadFont(Assets, Gui->TileFontID, ASSET_LOAD_IMMEDIATE);
    
    Gui->CheckboxMarkID = GetFirst(Assets, GameAsset_CheckboxMark);
    Gui->ChamomileID = GetFirst(Assets, GameAsset_ChamomileIcon);
    
    Gui->fontScale = 1.0f;
    
    // NOTE(Dima): Init layouts
    Gui->layoutCount = 1;
    Gui->rootLayout = {};
    DLIST_REFLECT_PTRS(Gui->rootLayout, Next, Prev);
    CopyStrings(Gui->rootLayout.Name, "RootLayout");
    Gui->rootLayout.ID = StringHashFNV(Gui->rootLayout.Name);
    
    // NOTE(Dima): Init dim stack
    for(int dimIndex = 0; dimIndex < ARRAY_COUNT(Gui->dimStack); dimIndex++){
        Gui->dimStack[dimIndex] = {};
    }
    Gui->dimStackIndex = 0;
    Gui->inPushBlock = 0;
    
    
    // NOTE(Dima): Init pages
    Gui->pageCount = 1;
    Gui->rootPage = {};
    DLIST_REFLECT_PTRS(Gui->rootPage, Next, Prev);
    CopyStrings(Gui->rootPage.name, "RootPage");
    Gui->rootPage.id = StringHashFNV(Gui->rootPage.name);
    
    // NOTE(Dima): Initializing of window free pool and sentinels
    DLIST_REFLECT_PTRS(Gui->windowUseSentinel, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Gui->windowFreeSentinel, NextAlloc, PrevAlloc);
    
    GuiGrowWindowFreePool(Gui, Gui->Mem, 128);
    
    // NOTE(Dima): Init window sentinel for returning windows
    // NOTE(Dima): as list when we allocate multiple of them.
    DLIST_REFLECT_PTRS(Gui->windowSentinel4Returning, Next, Prev);
    
    // NOTE(Dima): Init window leaf sentinel
    DLIST_REFLECT_PTRS(Gui->windowLeafSentinel, Next, Prev);
    
    Gui->tempWindow1 = GuiAllocateWindow(Gui);
    Gui->tempWindow1->rect = RcMinDim(V2(10, 10), V2(1000, 600));
    Gui->tempWindow1->visible = 1;
    GuiAddWindowToList(Gui->tempWindow1, &Gui->windowLeafSentinel);
    
    // NOTE(Dima): Initializing elements sentinel
    Gui->TotalAllocatedGuiElements = 0;
    DLIST_REFLECT_PTRS(Gui->freeSentinel, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Gui->useSentinel, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Initializing root element
    GuiInitRoot(Gui, &Gui->rootElement);
    
    // NOTE(Dima): Setting current element
    Gui->curElement = Gui->rootElement;
    
    // NOTE(Dima): Initializing colors
    InitColorsState(&Gui->colorState, Gui->Mem);
    Gui->colors[GuiColor_Text] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->colors[GuiColor_HotText] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->colors[GuiColor_Borders] = GUI_GETCOLOR_COLSYS(Color_Black);
    
    Gui->colors[GuiColor_Hot] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->colors[GuiColor_Active] = GUI_GETCOLOR_COLSYS(Color_Red);
    
    Gui->colors[GuiColor_ButtonBackground] = GUI_GETCOLOR_COLSYS(Color_PrettyBlue);
    Gui->colors[GuiColor_ButtonBackgroundHot] = GUI_GETCOLOR_COLSYS(Color_Cyan);
    Gui->colors[GuiColor_ButtonForeground] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->colors[GuiColor_ButtonForegroundHot] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->colors[GuiColor_ButtonForegroundDisabled] = Gui->colors[GuiColor_ButtonForeground] * 0.75f;
    Gui->colors[GuiColor_ButtonGrad1] = GUI_GETCOLOR_COLSYS(Color_PrettyBlue);
    Gui->colors[GuiColor_ButtonGrad2] = GUI_GETCOLOR_COLSYS(Color_Blue);
    
    Gui->windowAlpha = 0.85f;
    Gui->colors[GuiColor_WindowBackground] = V4(0.0f, 0.0f, 0.0f, Gui->windowAlpha);
    Gui->colors[GuiColor_WindowBorder] = GUI_GETCOLOR_COLSYS(Color_Black);
    Gui->colors[GuiColor_WindowBorderHot] = GUI_GETCOLOR_COLSYS(Color_Magenta);
    Gui->colors[GuiColor_WindowBorderActive] = GUI_GETCOLOR_COLSYS(Color_Blue);
}

rc2 PrintTextInternal(font_info* Font, 
                      render_stack* stack, 
                      assets* Assets, 
                      char* text, 
                      v2 p, 
                      u32 textOp, 
                      float Scale = 1.0f, 
                      v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), 
                      int CaretP = 0, 
                      v2* CaretPrintPOut = 0)
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
            u32 GlyphAssetID = Font->GlyphIDs[Font->Codepoint2Glyph[*at]];
            asset* GlyphAsset = GetAssetByID(Assets, GlyphAssetID);
            
            glyph_info* Glyph = GET_ASSET_PTR_MEMBER(GlyphAsset, glyph_info);;
            
            if(Glyph){
                float bmpScale = Glyph->Height * Scale;
                
                v2 bitmapDim = { Glyph->WidthOverHeight * bmpScale, bmpScale };
                
                if(textOp == PrintTextOp_Print){
                    float bitmapMinY = curP.y + Glyph->YOffset * Scale;
                    float bitmapMinX = curP.x + Glyph->XOffset * Scale;
                    
                    PushOrLoadGlyph(Assets,
                                    stack, 
                                    V2(bitmapMinX, bitmapMinY), 
                                    bitmapDim, 
                                    Glyph->BitmapID,
                                    color);
                    
                }
                
                curP.x += ((float)Glyph->Advance * Scale);
            }
            
        }
        else if(*at == '\t'){
            curP.x += ((float)Font->AscenderHeight * 4 * Scale);
        }
        
        at++;
    }
    
    if(text + CaretP == at){
        if(CaretPrintPOut){
            *CaretPrintPOut = curP;
        }
    }
    
    txtRc.min.x = p.x;
    txtRc.min.y = p.y - Font->AscenderHeight * Scale;
    txtRc.max.x = curP.x;
    txtRc.max.y = curP.y - Font->DescenderHeight * Scale;
    
    return(txtRc);
}

void PrintCaret(gui_state* Gui, v2 PrintP, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f)){
    float bmpScale = GuiGetLineAdvance(Gui);
    
    float CaretMinY = PrintP.y - GetScaledAscender(Gui->MainFont, Gui->fontScale);
    float CaretMinX = PrintP.x;;
    
    v2 CaretMin = V2(CaretMinX, CaretMinY);
    v2 CaretDim = V2(bmpScale * 0.6f, bmpScale);
    PushRect(Gui->Stack, RcMinDim(CaretMin, CaretDim), Color);
}

v2 GetTextSizeInternal(font_info* Font, assets* Assets, char* Text, float Scale){
    rc2 TextRc = PrintTextInternal(
        Font, 
        0,
        Assets,
        Text, V2(0.0f, 0.0f), 
        PrintTextOp_GetSize, 
        Scale);
    
    v2 result = GetRectDim(TextRc);
    
    return(result);
}

inline v2 GetCenteredTextOffset(font_info* font, float TextDimX, rc2 Rect, float Scale = 1.0f){
    float LineDimY = GetLineAdvance(font, Scale);
    float LineDelta = (font->AscenderHeight + font->LineGap) * Scale - LineDimY * 0.5f;
    
    v2 CenterRc = Rect.min + GetRectDim(Rect) * 0.5f;
    
    v2 TargetP = V2(CenterRc.x - TextDimX * 0.5f, CenterRc.y + LineDelta);
    return(TargetP);
}

inline float GetCenteredTextOffsetY(font_info* font, rc2 rect, float Scale = 1.0f){
    float result = GetCenteredTextOffset(font, 0.0f, rect, Scale).y;
    
    return(result);
}

inline v2 GetCenteredTextOffset(font_info* Font, 
                                assets* Assets, 
                                char* Text, 
                                rc2 rect, 
                                float Scale = 1.0f)
{
    v2 TextDim = GetTextSizeInternal(Font, Assets, Text, Scale);
    
    v2 result = GetCenteredTextOffset(Font, TextDim.x, rect, Scale);
    
    return(result);
}

rc2 PrintTextCenteredInRectInternal(
font_info* Font, 
render_stack* stack,
assets* Assets,
char* text, 
rc2 rect, 
float Scale = 1.0f, 
v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    v2 targetP = GetCenteredTextOffset(Font, Assets, text, rect, Scale);
    rc2 result = PrintTextInternal(Font, stack, Assets, text, targetP, PrintTextOp_Print, Scale, color);
    
    return(result);
}

v2 GetTextSize(gui_state* Gui, char* text, float Scale){
    v2 result = GetTextSizeInternal(Gui->MainFont,
                                    Gui->Assets,
                                    text, 
                                    Gui->fontScale * Scale);
    
    return(result);
}

rc2 GetTextRect(gui_state* Gui, char* text, v2 p, float Scale){
    rc2 TextRc = PrintTextInternal(
        Gui->MainFont, 
        Gui->Stack, 
        Gui->Assets,
        text, p, 
        PrintTextOp_GetSize, 
        Gui->fontScale * Scale);
    
    return(TextRc);
}

rc2 PrintText(gui_state* Gui, 
              char* text, 
              v2 p, 
              v4 Color, 
              float Scale)
{
    rc2 TextRc = PrintTextInternal(
        Gui->MainFont, 
        Gui->Stack, 
        Gui->Assets,
        text, p, 
        PrintTextOp_Print, 
        Gui->fontScale * Scale,
        Color);
    
    return(TextRc);
}


v2 GetCaretPrintP(gui_state* Gui, char* text, v2 p, int CaretP){
    v2 Result;
    
    rc2 TextRc = PrintTextInternal(
        Gui->MainFont, 
        Gui->Stack, 
        Gui->Assets,
        text, p, 
        PrintTextOp_Print, 
        Gui->fontScale * Gui->fontScale,
        V4(0.0f, 0.0f, 0.0f, 0.0f), 
        CaretP, &Result);
    
    return(Result);
}


rc2 PrintTextCenteredInRect(gui_state* Gui, char* text, rc2 rect, float Scale, v4 Color){
    rc2 result = PrintTextCenteredInRectInternal(Gui->MainFont,
                                                 Gui->Stack,
                                                 Gui->Assets,
                                                 text, 
                                                 rect,
                                                 Gui->fontScale * Scale,
                                                 Color);
    
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

inline void GuiSnapInWindowRect(gui_state* Gui, v2* P, v2* Dim, u32 SnapType){
    rc2 WindowRc = RcMinDim(V2(0.0f, 0.0f), V2(Gui->Width, Gui->Height));
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

INTERNAL_FUNCTION void GuiInitLayout(gui_state* Gui, Gui_Layout* layout, u32 layoutType, gui_element* LayoutElem){
    // NOTE(Dima): initializing references
    LayoutElem->Data.Layout.ref = layout;
    layout->Elem = LayoutElem;
    
    v2 popDim;
    if(!GuiPopDim(Gui, &popDim)){
        popDim = V2(640, 480);
    }
    
    // NOTE(Dima): Layout initializing
    layout->Type = layoutType;
    if(!LayoutElem->Data.IsInit){
        layout->Start = V2(200.0f, 200.0f) * (Gui->layoutCount - 1);
        layout->At = layout->Start;
        // NOTE(Dima): Layout dimension should be set anyways
        // NOTE(Dima): because of interaction that account to them
        layout->Dim = V2(Gui->Width, Gui->Height);
        
        switch(layoutType){
            case GuiLayout_Layout:{
                layout->Dim = V2(Gui->Width, Gui->Height);
            }break;
            
            case GuiLayout_Window:{
                layout->Dim = popDim;
            }break;
        }
        
        LayoutElem->Data.IsInit = true;
    }
    
    // NOTE(Dima): Recalculate window rect always for clipping GUI interactions
    layout->Rect = RcMinDim(layout->At, layout->Dim);
    
    // NOTE(Dima): Initializing initial advance ctx
    layout->AdvanceRememberStack[0].type = GuiAdvanceType_Column;
    layout->AdvanceRememberStack[0].rememberValue = layout->Start.y;
    layout->AdvanceRememberStack[0].baseline = layout->Start.x;
    
    if(layoutType == GuiLayout_Window){
        
        
        gui_interaction Interaction = CreateInteraction(LayoutElem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Small);
#if 1    
        
        GuiAnchor(Gui, "Anchor1", 
                  layout->Start + layout->Dim,
                  V2(20, 20),
                  true,
                  true,
                  &layout->Start,
                  &layout->Dim);
        
        GuiAnchor(Gui, "Anchor2", 
                  layout->Start,
                  V2(20, 20),
                  false,
                  true,
                  &layout->Start,
                  &layout->Dim);
        
#endif
        
        rc2 windowRc = layout->Rect;
        
        PushRect(Gui->Stack, windowRc, GUI_GETCOLOR(GuiColor_WindowBackground));
        
        v4 outlineColor = GUI_GETCOLOR(GuiColor_WindowBorder);
        if(MouseInInteractiveArea(Gui, windowRc)){
            GuiSetHot(Gui, &Interaction, true);
            if(GuiIsHot(Gui, &Interaction)){
                outlineColor = GUI_GETCOLOR(GuiColor_Hot);
            }
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
            }
            
        }
        else{
            
            if(KeyWentDown(Gui->Input, MouseKey_Left) ||
               KeyWentDown(Gui->Input, MouseKey_Right))
            {
                GuiReleaseInteraction(Gui, &Interaction);
            }
            
            GuiSetHot(Gui, &Interaction, false);
        }
        
        if(GuiIsActive(Gui, &Interaction)){
            outlineColor = GUI_GETCOLOR(GuiColor_Active);
            
            if(KeyWentDown(Gui->Input, Key_Left)){
                GuiSnapInWindowRect(Gui, &layout->Start, &layout->Dim, GuiWindowSnap_Left);
            }
            
            if(KeyWentDown(Gui->Input, Key_Right)){
                GuiSnapInWindowRect(Gui, &layout->Start, &layout->Dim, GuiWindowSnap_Right);
            }
            
            if(KeyWentDown(Gui->Input, Key_Up)){
                GuiSnapInWindowRect(Gui, &layout->Start, &layout->Dim, GuiWindowSnap_Top);
            }
            
            if(KeyWentDown(Gui->Input, Key_Down)){
                GuiSnapInWindowRect(Gui, &layout->Start, &layout->Dim, GuiWindowSnap_Bottom);
            }
            
            if(KeyWentDown(Gui->Input, MouseKey_Right)){
                GuiSnapInWindowRect(Gui, &layout->Start, &layout->Dim, GuiWindowSnap_CenterHalf);
            }
        }
        
        // NOTE(Dima): Pushing inner outline
        PushRectOutline(Gui->Stack, windowRc, 2, outlineColor);
        
        // NOTE(Dima): Beginning GUI chunk
        BeginGuiChunk(Gui->Stack, windowRc);
    }
}

void GuiBeginLayout(gui_state* Gui, char* name, u32 layoutType){
    // NOTE(Dima): In list inserting
    u32 nameID = StringHashFNV(name);
    
    Gui_Layout* foundLayout = 0;
    Gui_Layout* layoutAt = Gui->rootLayout.Next;
    for(layoutAt; layoutAt != &Gui->rootLayout; layoutAt = layoutAt->Next){
        if(nameID == layoutAt->ID){
            foundLayout = layoutAt;
            break;
        }
    }
    
    if(!foundLayout){
        foundLayout = PushStruct(Gui->Mem, Gui_Layout);
        
        CopyStringsSafe(foundLayout->Name, sizeof(foundLayout->Name), name);
        foundLayout->ID = nameID;
        
        foundLayout->Next = Gui->rootLayout.Next;
        foundLayout->Prev = &Gui->rootLayout;
        foundLayout->Next->Prev = foundLayout;
        foundLayout->Prev->Next = foundLayout;
        
        ++Gui->layoutCount;
    }
    
    // NOTE(Dima): Beginnning layout elem
    gui_element* LayoutElem = GuiBeginElement(Gui, name, GuiElement_Layout, true);
    
    GuiInitLayout(Gui, foundLayout, layoutType, LayoutElem);
}

void GuiEndLayout(gui_state* Gui){
    Gui_Layout* lay = GetParentLayout(Gui);
    
    lay->At = lay->Start;
    
    if(lay->Type == GuiLayout_Window){
        EndGuiChunk(Gui->Stack);
    }
    
    GuiEndElement(Gui, GuiElement_Layout);
}

#if 0
INTERNAL_FUNCTION void GuiSplitWindow(gui_state* Gui, 
                                      Gui_Window* window, 
                                      int partsCount, 
                                      rc2* partsRects)
{
    GuiAllocateWindows(Gui, partsCount);
    
    for(int newWindowIndex = 0;
        newWindowIndex < partsCount;
        newWindowIndex++)
    {
        Gui_Window* newWindow = GuiPopFromReturnList(Gui);
        
        // NOTE(Dima): Adding children to leafs
        newWindow->Next = Gui->windowLeafSentinel.Next;
        newWindow->Prev = &Gui->windowLeafSentinel;
        
        newWindow->Next->Prev = newWindow;
        newWindow->Prev->Next = newWindow;
        
        newWindow->rect = partsRects[newWindowIndex];
    }
    
    // NOTE(Dima): Deallocating parent because it is not visible
    window->Next->Prev = window->Prev;
    window->Prev->Next = window->Next;
    
    window->Next = 0;
    window->Prev = 0;
    
    GuiDeallocateWindow(Gui, window);
    
    // NOTE(Dima): Return list shoud be empty after usage in this function
    Assert(Gui->windowSentinel4Returning.Next == &Gui->windowSentinel4Returning);
}

INTERNAL_FUNCTION void GuiUpdateWindow(gui_state* Gui, Gui_Window* window){
    
    window->layout.Start = window->rect.min;
    
    if(window->visible){
        
        rc2 windowRc = window->rect;
        PushRect(Gui->Stack, windowRc, GUI_GETCOLOR(GuiColor_WindowBackground));
        
        v4 outlineColor = GUI_GETCOLOR(GuiColor_WindowBorder);
        if(MouseInRect(Gui->Input, windowRc)){
            outlineColor = GUI_GETCOLOR(GuiColor_WindowBorderHot);
            
            b32 conditionForSnapping = 0;
            if(conditionForSnapping){
                
                // NOTE(Dima): Processing snapping
                v2 windowDim = GetRectDim(windowRc);
                v2 windowHalfDim = windowDim * 0.5f;
                v2 windowCenter = windowRc.min + windowHalfDim;
                
                v2 MouseP = Gui->Input->MouseP;
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
                    PushRect(Gui->Stack, snapRes.result, snapColor);
                    
                    if(KeyWentDown(Gui->Input, MouseKey_Left)){
                        GuiSplitWindow(Gui, window, 2, snapRes.rects);
                    }
                }
            }
        }
        
        // NOTE(Dima): Pushing inner outline
        PushRectInnerOutline(Gui->Stack, windowRc, 1, outlineColor);
    }
}

void GuiUpdateWindows(gui_state* Gui){
    Gui_Window* updateAt = Gui->windowLeafSentinel.Next;
    while(updateAt != &Gui->windowLeafSentinel){
        Gui_Window* tempNext = updateAt->Next;
        
        GuiUpdateWindow(Gui, updateAt);
        
        updateAt = tempNext;
    }
}

void GuiBeginUpdateWindows(gui_state* Gui){
    
}

void GuiEndUpdateWindows(gui_state* Gui){
    GuiUpdateWindows(Gui);
}
#endif

void GuiFrameBegin(gui_state* Gui, gui_frame_info GuiFrameInfo){
    Gui->FrameInfo = GuiFrameInfo;
    
    Gui->Input = Gui->FrameInfo.Input;
    Gui->Stack = Gui->FrameInfo.Stack;
    Gui->Width = Gui->FrameInfo.Width;
    Gui->Height = Gui->FrameInfo.Height;
    
    // NOTE(Dima): Init dim stack
    // NOTE(Dima): Init dim stack
    Gui->dimStackIndex = 0;
    Gui->inPushBlock = 0;
    
    // NOTE(Dima): Beginning GUI chunk
    BeginGuiChunk(GuiFrameInfo.Stack, 
                  RcMinDim(V2(0.0f, 0.0f), 
                           V2(GuiFrameInfo.Width, GuiFrameInfo.Height)));
    
    // NOTE(Dima): Init root layout
    gui_element* LayoutElem = GuiBeginElement(Gui, 
                                              Gui->rootLayout.Name, 
                                              GuiElement_Layout, 
                                              true);
    
    GuiInitLayout(Gui, &Gui->rootLayout, GuiLayout_Layout, LayoutElem);
}

void GuiFrameEnd(gui_state* Gui){
    // NOTE(Dima): Deinit root layout
    Gui->rootLayout.At = Gui->rootLayout.Start;
    GuiEndElement(Gui, GuiElement_Layout);
}

void GuiFramePrepare4Render(gui_state* Gui){
    for(int tooltipIndex = 0; tooltipIndex < GUI_MAX_TOOLTIPS; tooltipIndex++){
        Gui_Tooltip* ttip= &Gui->tooltips[tooltipIndex];
        
        PrintText(Gui, ttip->text, ttip->at, GUI_GETCOLOR(GuiColor_Text), 1.0f);
    }
    Gui->tooltipIndex = 0;
    
    
    // NOTE(Dima): End GUI chunk
    EndGuiChunk(Gui->Stack);
}



void GuiInteract(gui_state* Gui, 
                 gui_interaction* Interaction)
{
    switch(Interaction->Type){
        case GuiInteraction_Move:{
            gui_interaction_data_move* MoveData = &Interaction->Data.Move;
            
            v2 PtrStartInAnchor = MoveData->PtrStartInAnchor;
            rc2 AnchorRect = MoveData->AnchorRect;
            v2 MouseP = Gui->FrameInfo.Input->MouseP;
            
            v2* OffsetInAnchor = MoveData->OffsetInAnchor;
            v2* WorkPtr = MoveData->ChangePtr;
            v2 ResizedRectMin = MoveData->ResizedRectMin;
            
            if(MouseInRect(Gui->Input, MoveData->AnchorRect)){
                GuiSetHot(Gui, Interaction, true);
                Interaction->WasHotInInteraction = true;
                
                if(KeyWentDown(Gui->Input, MouseKey_Left)){
                    GuiSetActive(Gui, Interaction);
                    Interaction->WasActiveInInteraction = true;
                    
                    *OffsetInAnchor = MouseP - AnchorRect.min;
                }
            }
            else{
                GuiSetHot(Gui, Interaction, false);
            }
            
            v2 AnchorTargetMinP = MouseP - *OffsetInAnchor;
            v2 TargetP = AnchorTargetMinP + PtrStartInAnchor;
            v2 ResizedDim = TargetP - ResizedRectMin;
            
            if(GuiIsActive(Gui, Interaction)){
                
                switch (MoveData->MoveType) {
                    case GuiMoveInteraction_Move:{
                        *WorkPtr = TargetP;
                    }break;
                    
                    case GuiMoveInteraction_Resize_Default: {
                        *WorkPtr = ResizedDim;
                        
                        if (ResizedDim.x < MoveData->MinDim.x) {
                            WorkPtr->x = MoveData->MinDim.x;
                        }
                        
                        if (ResizedDim.y < MoveData->MinDim.y) {
                            WorkPtr->y = MoveData->MinDim.y;
                        }
                    }break;
                    
                    case GuiMoveInteraction_Resize_Horizontal: {
                        WorkPtr->x = ResizedDim.x;
                        if (ResizedDim.x < MoveData->MinDim.x) {
                            WorkPtr->x = MoveData->MinDim.x;
                        }
                    }break;
                    
                    case GuiMoveInteraction_Resize_Vertical: {
                        WorkPtr->y = ResizedDim.y;
                        if (ResizedDim.y < MoveData->MinDim.y) {
                            WorkPtr->y = MoveData->MinDim.y;
                        }
                    }break;
                    
                    case GuiMoveInteraction_Resize_Proportional: {
                        float WidthToHeight = WorkPtr->x / WorkPtr->y;
                        WorkPtr->y = ResizedDim.y;
                        WorkPtr->x = WorkPtr->y * WidthToHeight;
                        
                        if (ResizedDim.y < MoveData->MinDim.y) {
                            WorkPtr->y = MoveData->MinDim.y;
                            WorkPtr->x = WorkPtr->y * WidthToHeight;
                        }
                    }break;
                }
                
                if(KeyWentUp(Gui->Input, MouseKey_Left)){
                    GuiReleaseInteraction(Gui, Interaction);
                    
                    *OffsetInAnchor = {};
                }
            }
        }break;
        
        case GuiInteraction_BoolInRect:{
            gui_interaction_data_bool_in_rect* BoolInRect = &Interaction->Data.BoolInRect;
            
            if(MouseInInteractiveArea(Gui, BoolInRect->Rect)){
                GuiSetHot(Gui, Interaction, true);
                Interaction->WasHotInInteraction = true;
                
                if(KeyWentDown(Gui->FrameInfo.Input, MouseKey_Left)){
                    *BoolInRect->Value = !*BoolInRect->Value;
                    
                    GuiSetActive(Gui, Interaction);
                    Interaction->WasActiveInInteraction = true;
                    
                    GuiReleaseInteraction(Gui, Interaction);
                }
                
            }
            else{
                GuiSetHot(Gui, Interaction, false);
            }
        }break;
    }
}

// NOTE(Dima): Default advance type is Column advance
inline void GuiPreAdvance(gui_state* Gui, Gui_Layout* layout){
    GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
    b32 rowStarted = ctx->type == GuiAdvanceType_Row;
    
    float rememberValue = ctx->rememberValue;
    
    if(rowStarted){
        layout->At.y = ctx->baseline;
    }
    else{
        layout->At.x = ctx->baseline;
        layout->At.y += GuiGetBaseline(Gui);
    }
}

inline void GuiPostAdvance(gui_state* Gui, Gui_Layout* layout, rc2 ElementRect){
    GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
    b32 rowStarted = (ctx->type == GuiAdvanceType_Row);
    
    float RememberValue = ctx->rememberValue;
    
    float toX = ElementRect.max.x + GetScaledAscender(Gui->MainFont, Gui->fontScale) * 0.5f;
    float toY = ElementRect.max.y + GetLineAdvance(Gui->MainFont, Gui->fontScale) * 0.15f;
    
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

void GuiBeginRow(gui_state* Gui){
    char name[64];
    stbsp_sprintf(name, "Row or Column: %d", Gui->curElement->childCount);
    
    gui_element* elem = GuiBeginElement(Gui, name, GuiElement_RowColumn, true);
    if(GuiElementOpenedInTree(elem)){
        
        Gui_Layout* layout = GetParentLayout(Gui);
        
        Assert(layout->StackCurrentIndex < ArrayCount(layout->AdvanceRememberStack));
        
        layout->AdvanceRememberStack[++layout->StackCurrentIndex] = 
            GuiRowAdvanceCtx(layout->At.x, layout->At.y + GuiGetBaseline(Gui));
    }
}

void GuiBeginColumn(gui_state* Gui){
    char name[64];
    stbsp_sprintf(name, "Row or Column: %d", Gui->curElement->childCount);
    
    gui_element* elem = GuiBeginElement(Gui, name, GuiElement_RowColumn, true);
    if(GuiElementOpenedInTree(elem)){
        
        Gui_Layout* layout = GetParentLayout(Gui);
        
        Assert(layout->StackCurrentIndex < ArrayCount(layout->AdvanceRememberStack));
        
        layout->AdvanceRememberStack[++layout->StackCurrentIndex] = 
            GuiColumnAdvanceCtx(layout->At.y, layout->At.x);
    }
}

void GuiEndRow(gui_state* Gui){
    if(GuiElementOpenedInTree(Gui->curElement)){
        
        Gui_Layout* layout = GetParentLayout(Gui);
        
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
    
    GuiEndElement(Gui, GuiElement_RowColumn);
}

void GuiEndColumn(gui_state* Gui){
    if(GuiElementOpenedInTree(Gui->curElement)){
        
        Gui_Layout* layout = GetParentLayout(Gui);
        
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
    
    GuiEndElement(Gui, GuiElement_RowColumn);
}

enum Push_But_Type{
    PushBut_Empty,
    PushBut_Color,
    PushBut_Grad,
    PushBut_Outline,
    PushBut_DefaultBack,
    PushBut_DefaultGrad,
    PushBut_RectAndOutline,
    PushBut_AlphaBlack,
};

INTERNAL_FUNCTION void GuiPushBut(gui_state* Gui, 
                                  rc2 rect, 
                                  u32 type = PushBut_DefaultGrad, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f),
                                  v4 Color2 = V4(0.0f, 0.0f, 0.0f, 1.0f))
{
    
    switch(type){
        case PushBut_Empty:{
            
        }break;
        
        case PushBut_Color:{
            PushRect(Gui->Stack, rect, Color);
        }break;
        
        case PushBut_Grad:{
            PushGradient(Gui->Stack, rect, 
                         Color, Color2, 
                         RenderEntryGradient_Vertical);
        }break;
        
        case PushBut_AlphaBlack:{
            PushRect(Gui->Stack, rect, GUI_GETCOLOR(GuiColor_WindowBackground));
        }break;
        
        case PushBut_DefaultBack:{
            PushRect(Gui->Stack, rect, GUI_GETCOLOR(GuiColor_ButtonBackground));
        }break;
        
        case PushBut_DefaultGrad:{
            PushGradient(
                Gui->Stack, rect, 
                GUI_GETCOLOR(GuiColor_ButtonGrad1),
                GUI_GETCOLOR(GuiColor_ButtonGrad2),
                RenderEntryGradient_Vertical);
        }break;
        
        case PushBut_Outline:{
            PushRectOutline(Gui->Stack, rect, 2, Color);
        }break;
        
        case PushBut_RectAndOutline:{
            PushRect(Gui->Stack, rect, GUI_GETCOLOR(GuiColor_ButtonBackground));
            PushRectOutline(Gui->Stack, rect, 1, Color);
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

void GuiTooltip(gui_state* Gui, char* tooltipText, v2 at){
    Assert(Gui->tooltipIndex < GUI_MAX_TOOLTIPS);
    Gui_Tooltip* ttip = &Gui->tooltips[Gui->tooltipIndex++];
    
    CopyStringsSafe(ttip->text, GUI_TOOLTIP_MAX_SIZE, tooltipText);
    ttip->at = at;
}

void GuiAnchor(gui_state* Gui, 
               char* Name, 
               v2 Pos, v2 Dim, 
               b32 IsResize,
               b32 Centered, 
               v2* RectP, v2* RectDim)
{
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_Item, false);
    Gui_Layout* layout = GetParentLayout(Gui);
    
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
        
        v2 MouseP = Gui->Input->MouseP;
        
        if(!elem->Data.IsInit){
            
            elem->Data.Anchor.OffsetInAnchor = {};
            
            elem->Data.IsInit = true;
        }
        v2* OffsetInAnchor = &elem->Data.Anchor.OffsetInAnchor;
        
        gui_interaction Interaction = CreateInteraction(elem, 
                                                        GuiInteraction_Move,
                                                        GuiPriority_Avg);
        
        gui_interaction_data_move* Move = &Interaction.Data.Move;
        
        Move->AnchorRect = WorkRect;
        Move->OffsetInAnchor = OffsetInAnchor;
        Move->PtrStartInAnchor = Pos - MinP;
        
        if(IsResize){
            Move->ChangePtr = RectDim;
            Move->MoveType = GuiMoveInteraction_Resize_Default;
            Move->MinDim = V2(50, 50);
            Move->ResizedRectMin = *RectP;
        }
        else{
            Move->ChangePtr = RectP;
            Move->MoveType = GuiMoveInteraction_Move;
        }
        
        GuiInteract(Gui, &Interaction);
        
        v4 AnchorColor = GUI_GETCOLOR_COLSYS(Color_Orange);
        if(Interaction.WasActiveInInteraction){
            AnchorColor = GUI_GETCOLOR_COLSYS(Color_Blue);
        }
        
        PushRect(Gui->Stack, WorkRect, AnchorColor);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiBeginTree(gui_state* Gui, char* name){
    gui_element* elem = GuiBeginElement(Gui, name, GuiElement_Item, false);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && PotentiallyVisibleSmall(layout))
    {
        Gui_Layout* layout = GetParentLayout(Gui);
        
        GuiPreAdvance(Gui, layout);
        
        rc2 textRc = GetTextRect(Gui, name, layout->At);
        
        v4 textColor = GUI_GETCOLOR_COLSYS(Color_ToxicGreen);
        v4 oulineColor = GUI_GETCOLOR_COLSYS(Color_Red);
        
        GuiPushBut(Gui, textRc, PushBut_DefaultGrad, oulineColor);
        if(elem->Opened){
            GuiPushBut(Gui, textRc, PushBut_Outline, oulineColor);
        }
        
        gui_interaction Interaction = CreateInteraction(elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, textRc)){
            GuiSetHot(Gui, &Interaction, true);
            textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
                GuiReleaseInteraction(Gui, &Interaction);
                
                elem->Opened = !elem->Opened;
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        PrintText(Gui, name, layout->At, textColor);
        
        GuiPostAdvance(Gui, layout, textRc);
    }
}
void GuiEndTree(gui_state* Gui){
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiText(gui_state* Gui, char* text){
    gui_element* elem = GuiBeginElement(Gui, text, GuiElement_TempItem, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(Gui, layout);
        
        rc2 textRc = PrintText(Gui, text, layout->At, GUI_GETCOLOR(GuiColor_Text));
        
        GuiPostAdvance(Gui, layout, textRc);
    }
    GuiEndElement(Gui, GuiElement_TempItem);
}

b32 GuiButton(gui_state* Gui, char* buttonName){
    b32 result = 0;
    
    gui_element* elem = GuiBeginElement(Gui, buttonName, GuiElement_Item, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(Gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(Gui, buttonName, layout->At);
        textRc = GetTxtElemRect(Gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(Gui, textRc);
        
        // NOTE(Dima): Event processing
        v4 textColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
        
        gui_interaction Interaction = CreateInteraction(elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, textRc)){
            GuiSetHot(Gui, &Interaction, true);
            textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
                GuiReleaseInteraction(Gui, &Interaction);
                result = 1;
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        PrintTextCenteredInRect(Gui, buttonName, textRc, 1.0f, textColor);
        
        GuiPostAdvance(Gui, layout, textRc);
    }
    GuiEndElement(Gui, GuiElement_Item);
    
    return(result);
}

void GuiBoolButton(gui_state* Gui, char* buttonName, b32* Value){
    gui_element* elem = GuiBeginElement(Gui, buttonName, GuiElement_Item, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(Gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(Gui, buttonName, layout->At);
        textRc = GetTxtElemRect(Gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(Gui, textRc);
        
        v4 textColor = GUI_GETCOLOR(GuiColor_ButtonForeground);
        
        // NOTE(Dima): Event processing
        if(Value){
            if(*Value == 0){
                textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
            }
            
            
            gui_interaction Interaction = CreateInteraction(elem, 
                                                            GuiInteraction_Empty,
                                                            GuiPriority_Avg);
            
            gui_interaction_data_bool_in_rect* BoolInRect = &Interaction.Data.BoolInRect;
            
            BoolInRect->Value = Value;
            BoolInRect->Rect = textRc;
            
            GuiInteract(Gui, &Interaction);
            
            if(Interaction.WasHotInInteraction){
                textColor = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            }
        }
        
        PrintTextCenteredInRect(Gui, buttonName, textRc, 1.0f, textColor);
        
        GuiPostAdvance(Gui, layout, textRc);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiBoolButtonOnOff(gui_state* Gui, char* buttonName, b32* Value){
    gui_element* elem = GuiBeginElement(Gui, buttonName, GuiElement_Item, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(Gui, layout);
        
        // NOTE(Dima): Button printing
        rc2 butRc = GetTextRect(Gui, "OFF", layout->At);
        butRc = GetTxtElemRect(Gui, layout, butRc, V2(4.0f, 3.0f));
        GuiPushBut(Gui, butRc);
        
        // NOTE(Dima): Button name text printing
        float nameStartY = GetCenteredTextOffsetY(Gui->MainFont, butRc, Gui->fontScale);
        v2 nameStart = V2(butRc.max.x + GetScaledAscender(Gui->MainFont, Gui->fontScale) * 0.5f, nameStartY);
        rc2 NameRc = PrintText(Gui, buttonName, nameStart, GUI_GETCOLOR(GuiColor_Text));
        
        // NOTE(Dima): Event processing
        char buttonText[4];
        CopyStrings(buttonText, "ERR");
        v4 buttonTextC = GUI_GETCOLOR(GuiColor_ButtonForeground);
        if(Value){
            if(*Value){
                CopyStrings(buttonText, "ON");
            }
            else{
                buttonTextC = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
                
                CopyStrings(buttonText, "OFF");
            }
            
            
            gui_interaction Interaction = CreateInteraction(elem, 
                                                            GuiInteraction_Empty,
                                                            GuiPriority_Avg);
            
            gui_interaction_data_bool_in_rect* BoolInRect = &Interaction.Data.BoolInRect;
            
            BoolInRect->Value = Value;
            BoolInRect->Rect = NameRc;
            
            GuiInteract(Gui, &Interaction);
            
            if(Interaction.WasHotInInteraction){
                buttonTextC = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            }
        }
        
        PrintTextCenteredInRect(Gui, buttonText, butRc, 1.0f, buttonTextC);
        
        rc2 AdvanceRect = GetBoundingRect(butRc, NameRc);
        GuiPostAdvance(Gui, layout, AdvanceRect);
    }
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiCheckbox(gui_state* Gui, char* name, b32* Value){
    gui_element* elem = GuiBeginElement(Gui, name, GuiElement_Item, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        
        GuiPreAdvance(Gui, layout);
        
        // NOTE(Dima): Checkbox rendering
        float chkSize = GetLineAdvance(Gui->MainFont, Gui->fontScale);
        rc2 chkRect;
        chkRect.min = V2(layout->At.x, layout->At.y - GetScaledAscender(Gui->MainFont, Gui->fontScale));
        chkRect.max = chkRect.min + V2(chkSize, chkSize);
        chkRect = GetTxtElemRect(Gui, layout, chkRect, V2(2.0f, 2.0f));
        
        // NOTE(Dima): Event processing
        v4 backC = GUI_GETCOLOR(GuiColor_ButtonBackground);
        if(Value){
            
            gui_interaction Interaction = CreateInteraction(elem, 
                                                            GuiInteraction_Empty,
                                                            GuiPriority_Avg);
            
            gui_interaction_data_bool_in_rect* BoolInRect = &Interaction.Data.BoolInRect;
            
            BoolInRect->Value = Value;
            BoolInRect->Rect = chkRect;
            
            GuiInteract(Gui, &Interaction);
            
            
            if(Interaction.WasHotInInteraction){
                backC = GUI_GETCOLOR(GuiColor_ButtonBackgroundHot);
            }
        }
        
        GuiPushBut(Gui, chkRect);
        
        if(*Value){
            PushOrLoadGlyph(Gui->Assets, 
                            Gui->Stack,
                            chkRect.min, 
                            GetRectDim(chkRect),
                            Gui->CheckboxMarkID,
                            V4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        
        // NOTE(Dima): Button name text printing
        float nameStartY = GetCenteredTextOffsetY(Gui->MainFont, chkRect, Gui->fontScale);
        v2 nameStart = V2(chkRect.max.x + GetScaledAscender(Gui->MainFont, Gui->fontScale) * 0.5f, nameStartY);
        rc2 nameRc = PrintText(Gui, name, nameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 advanceRect = GetBoundingRect(chkRect, nameRc);
        GuiPostAdvance(Gui, layout, advanceRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiBeginRadioGroup(
gui_state* Gui, 
char* name, 
u32* ref, 
u32 defaultId) 
{
    gui_element* element = GuiBeginElement(Gui, 
                                           name, 
                                           GuiElement_RadioGroup, 
                                           true);
    
    if (!element->Data.IsInit) {
        element->Data.RadioGroup.activeId = defaultId;
        element->Data.RadioGroup.ref = ref;
        
        element->Data.IsInit = 1;
    }
}

INTERNAL_FUNCTION inline gui_element* 
GuiFindRadioGroupParent(gui_element* curElement) {
    gui_element* result = 0;
    
    gui_element* at = curElement;
    while (at != 0) {
        if (at->type == GuiElement_RadioGroup) {
            result = at;
            break;
        }
        
        at = at->parent;
    }
    
    return(result);
}


void GuiRadioButton(gui_state* Gui, char* name, u32 uniqueId) {
    gui_element* radioBut = GuiBeginElement(Gui, name, GuiElement_Item, true);
    gui_element* radioGroup = GuiFindRadioGroupParent(Gui->curElement);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if (radioGroup && 
        GuiElementOpenedInTree(radioBut) && 
        PotentiallyVisibleSmall(layout)) 
    {
        b32 isActive = 0;
        if (radioGroup->Data.RadioGroup.activeId == uniqueId) {
            isActive = 1;
        }
        
        GuiPreAdvance(Gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(Gui, name, layout->At);
        textRc = GetTxtElemRect(Gui, layout, textRc, V2(4.0f, 3.0f));
        GuiPushBut(Gui, textRc);
        
        v4 textC;
        if(isActive){
            textC = GUI_GETCOLOR(GuiColor_ButtonForeground);
            v4 oulineColor = GUI_GETCOLOR_COLSYS(Color_Red);
            GuiPushBut(Gui, textRc, PushBut_Outline, oulineColor);
        }
        else{
            textC = GUI_GETCOLOR(GuiColor_ButtonForegroundDisabled);
        }
        
        gui_interaction Interaction = CreateInteraction(radioBut, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if (MouseInInteractiveArea(Gui, textRc)) {
            GuiSetHot(Gui, &Interaction, true);
            textC = GUI_GETCOLOR(GuiColor_ButtonForegroundHot);
            
            if (KeyWentDown(Gui->Input, MouseKey_Left)) {
                GuiSetActive(Gui, &Interaction);
                
                if(radioGroup->Data.RadioGroup.ref){
                    *radioGroup->Data.RadioGroup.ref = uniqueId;
                }
                radioGroup->Data.RadioGroup.activeId = uniqueId;
                
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        PrintTextCenteredInRect(Gui, name, textRc, 1.0f, textC);
        
        GuiPostAdvance(Gui, layout, textRc);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiEndRadioGroup(gui_state* Gui) {
    GuiEndElement(Gui, GuiElement_RadioGroup);
}

void GuiSliderInt(gui_state* Gui, int* Value, int Min, int Max, char* Name, u32 Style){
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_Item, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(Gui, layout);
        
        rc2 SlideRc = GetTextRect(Gui, 
                                  "                ", 
                                  layout->At);
        SlideRc = GetTxtElemRect(Gui, layout, SlideRc, V2(4.0f, 4.0f));
        GuiPushBut(Gui, SlideRc, PushBut_Color, V4(0.05f, 0.05f, 0.1f, 1.0f));
        
        float SlideRcWidth = GetRectWidth(SlideRc);
        float SlideRcHeight = GetRectHeight(SlideRc);
        
        char Buf[32];
        
        gui_interaction Interaction = CreateInteraction(elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, SlideRc)){
            GuiSetHot(Gui, &Interaction, true);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        if(GuiIsActive(Gui, &Interaction)){
            float MousePercentage = Clamp01((Gui->Input->MouseP.x - SlideRc.min.x) / SlideRcWidth);
            
            float FloatedValue = Min + (float)((Max - Min) * MousePercentage);
            
            int NewValue = SafeTruncateToInt(FloatedValue);
            
            *Value = NewValue;
            
            if(KeyWentUp(Gui->Input, MouseKey_Left)){
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        
        if(Value){
            float ValuePercentage = (float)(*Value - Min) / (float)(Max - Min);
            
            if(Style == GuiSlider_Progress){
                rc2 FillRc;
                FillRc.min = SlideRc.min;
                FillRc.max = V2(SlideRc.min.x + SlideRcWidth * ValuePercentage, SlideRc.max.y);
                
                GuiPushBut(Gui, FillRc, PushBut_Color, V4(1.0f, 0.3f, 0.1f, 1.0f));
            }
            else if(Style == GuiSlider_Index){
                float HotRectCenterX = SlideRc.min.x + SlideRcWidth * ValuePercentage;
                float HotRectDimX = SlideRcHeight;
                
                rc2 HotRect = RcMinDim(V2(HotRectCenterX - HotRectDimX * 0.5f, SlideRc.min.y), 
                                       V2(HotRectDimX, HotRectDimX));
                
                PushOrLoadGlyph(Gui->Assets,
                                Gui->Stack,
                                HotRect.min,
                                GetRectDim(HotRect),
                                Gui->ChamomileID,
                                V4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            
            
            stbsp_sprintf(Buf, "%d", *Value);
        }
        
        v4 textC = GUI_GETCOLOR(GuiColor_ButtonForeground);
        PrintTextCenteredInRect(Gui, Buf, SlideRc, 1.0f, textC);
        
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, SlideRc, Gui->fontScale);
        v2 NameStart = V2(SlideRc.max.x + GetScaledAscender(Gui->MainFont, Gui->fontScale) * 0.5f, NameStartY);
        
        rc2 NameRc = PrintText(Gui, Name, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, SlideRc);
        GuiPostAdvance(Gui, layout, AdvanceRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiSliderFloat(gui_state* Gui, float* Value, float Min, float Max, char* Name, u32 Style){
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_Item, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(Gui, layout);
        
        rc2 SlideRc = GetTextRect(Gui, 
                                  "                ", 
                                  layout->At);
        SlideRc = GetTxtElemRect(Gui, layout, SlideRc, V2(4.0f, 4.0f));
        GuiPushBut(Gui, SlideRc, PushBut_Color, V4(0.05f, 0.05f, 0.1f, 1.0f));
        
        float SlideRcWidth = GetRectWidth(SlideRc);
        float SlideRcHeight = GetRectHeight(SlideRc);
        
        char Buf[32];
        
        gui_interaction Interaction = CreateInteraction(elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, SlideRc)){
            GuiSetHot(Gui, &Interaction, true);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        if(GuiIsActive(Gui, &Interaction)){
            float MousePercentage = Clamp01((Gui->Input->MouseP.x - SlideRc.min.x) / SlideRcWidth);
            *Value = Min + (Max - Min) * MousePercentage;
            
            
            if(KeyWentUp(Gui->Input, MouseKey_Left)){
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        
        if(Value){
            float ValuePercentage = (*Value - Min) / (Max - Min);
            
            if(Style == GuiSlider_Progress){
                rc2 FillRc;
                FillRc.min = SlideRc.min;
                FillRc.max = V2(SlideRc.min.x + SlideRcWidth * ValuePercentage, SlideRc.max.y);
                
                GuiPushBut(Gui, FillRc, PushBut_Color, V4(1.0f, 0.3f, 0.1f, 1.0f));
            }
            else if(Style == GuiSlider_Index){
                float HotRectCenterX = SlideRc.min.x + SlideRcWidth * ValuePercentage;
                float HotRectDimX = SlideRcHeight;
                
                rc2 HotRect = RcMinDim(V2(HotRectCenterX - HotRectDimX * 0.5f, SlideRc.min.y), 
                                       V2(HotRectDimX, HotRectDimX));
                
                PushOrLoadGlyph(Gui->Assets,
                                Gui->Stack,
                                HotRect.min,
                                GetRectDim(HotRect),
                                Gui->ChamomileID,
                                V4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            
            
            stbsp_sprintf(Buf, "%.3f", *Value);
        }
        else{
            stbsp_sprintf(Buf, "ERR");
        }
        v4 textC = GUI_GETCOLOR(GuiColor_ButtonForeground);
        PrintTextCenteredInRect(Gui, Buf, SlideRc, 1.0f, textC);
        
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, SlideRc, Gui->fontScale);
        v2 NameStart = V2(SlideRc.max.x + GetScaledAscender(Gui->MainFont, Gui->fontScale) * 0.5f, NameStartY);
        
        rc2 NameRc = PrintText(Gui, Name, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, SlideRc);
        GuiPostAdvance(Gui, layout, AdvanceRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
    
}

void GuiInputText(gui_state* Gui, char* name, char* Buf, int BufSize){
    gui_element* elem = GuiBeginElement(Gui, name, GuiElement_Item, true);
    Gui_Layout* layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(layout))
    {
        GuiPreAdvance(Gui, layout);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(Gui, 
                                 "                            ", 
                                 layout->At);
        textRc = GetTxtElemRect(Gui, layout, textRc, V2(4.0f, 6.0f));
        GuiPushBut(Gui, textRc, PushBut_AlphaBlack);
        
        int* CaretP = &elem->Data.InputText.CaretPos;
        
        gui_interaction Interaction = CreateInteraction(elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, textRc)){
            GuiSetHot(Gui, &Interaction, true);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
            }
        }
        else{
            if(KeyWentDown(Gui->Input, MouseKey_Left) ||
               KeyWentDown(Gui->Input, MouseKey_Right))
            {
                GuiReleaseInteraction(Gui, &Interaction);
            }
            
            GuiSetHot(Gui, &Interaction, false);
        }
        
        if(GuiIsActive(Gui, &Interaction)){
            
            char FrameInputProcessed[32];
            int FrameInputProcessedAt = 0;
            FrameInputProcessed[0] = 0;
            
            for(int i = 0; i < Gui->Input->FrameInputLen; i++){
                char AtChar = Gui->Input->FrameInput[i];
                if(AtChar >= ' ' && AtChar <= '~'){
                    FrameInputProcessed[FrameInputProcessedAt++] = AtChar;
                    FrameInputProcessed[FrameInputProcessedAt] = 0;
                }
            }
            
            char TmpBuf[256];
            *CaretP += 
                InsertStringToString(Buf, BufSize,
                                     TmpBuf, 256,
                                     *CaretP, 
                                     FrameInputProcessed);
            
            if(KeyIsDown(Gui->Input, Key_Left)){
                if(KeyIsDown(Gui->Input, Key_Control)){
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Left].RepeatCount; i++){
                        int FoundIndex = FindSkipPrev(Buf, *CaretP);
                        *CaretP = FoundIndex;
                    }
                }
                else{
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Left].RepeatCount; i++){
                        *CaretP = *CaretP - 1;
                    }
                }
            }
            
            if(KeyIsDown(Gui->Input, Key_Right)){
                if(KeyIsDown(Gui->Input, Key_Control)){
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Right].RepeatCount; i++){
                        int FoundIndex = FindSkipNext(Buf, *CaretP);
                        *CaretP = FoundIndex;
                    }
                }
                else{
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Right].RepeatCount; i++){
                        *CaretP = *CaretP + 1;
                    }
                }
            }
            
            if(KeyIsDown(Gui->Input, Key_Backspace)){
                if(KeyIsDown(Gui->Input, Key_Control)){
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Backspace].RepeatCount; i++){
                        *CaretP += EraseToNonLetterPrev(Buf, *CaretP);
                    }
                }
                else{
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Backspace].RepeatCount; i++){
                        *CaretP += ErasePrevCharFromString(Buf, *CaretP);
                    }
                }
            }
            
            
            if(KeyIsDown(Gui->Input, Key_Delete)){
                if(KeyIsDown(Gui->Input, Key_Control)){
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Backspace].RepeatCount; i++){
                        //CaretP += EraseToNonLetterNext(Buf, *CaretP);
                    }
                }
                else{
                    for(int i = 0; i < Gui->Input->Keyboard.KeyStates[Key_Backspace].RepeatCount; i++){
                        *CaretP += EraseNextCharFromString(Buf, *CaretP);
                    }
                }
            }
            
            v4 oulineColor = GUI_GETCOLOR_COLSYS(Color_Red);
            GuiPushBut(Gui, textRc, PushBut_Outline, oulineColor);
        }
        
        *CaretP = Max(0, *CaretP);
        *CaretP = Min(*CaretP, Min(StringLength(Buf), BufSize - 1));
        
        v4 textC = V4(1.0f, 1.0f, 1.0f, 1.0f);
        //PrintTextCenteredInRect(Gui, Buf, textRc, 1.0f, textC);
        v2 BufTextSize = GetTextSize(Gui, Buf, 1.0f);
        float PrintPX;
        float FieldDimX = GetRectDim(textRc).x;
        if(BufTextSize.x < FieldDimX){
            PrintPX = textRc.min.x;
        }
        else{
            PrintPX = textRc.max.x - BufTextSize.x;
        }
        float PrintPY = GetCenteredTextOffsetY(Gui->MainFont,
                                               textRc, 
                                               Gui->fontScale);
        v2 PrintP = V2(PrintPX, PrintPY);
        v2 CaretPrintP = GetCaretPrintP(Gui, Buf, PrintP, *CaretP);
        
        BeginGuiChunk(Gui->Stack, textRc);
        v4 CaretColor = V4(0.0f, 1.0f, 0.0f, 1.0f);
        PrintCaret(Gui, 
                   CaretPrintP,
                   CaretColor); 
        PrintText(Gui, Buf, PrintP, GUI_GETCOLOR(GuiColor_Text));
        EndGuiChunk(Gui->Stack);
        
        float nameStartY = GetCenteredTextOffsetY(Gui->MainFont, textRc, Gui->fontScale);
        v2 nameStart = V2(textRc.max.x + GetScaledAscender(Gui->MainFont, Gui->fontScale) * 0.5f, nameStartY);
        char DebugBuf[256];
        
#if 1
        stbsp_sprintf(DebugBuf, "CaretP: %d; LeftRC: %d", *CaretP, 
                      Gui->Input->Keyboard.KeyStates[Key_Left].RepeatCount);
        rc2 NameRc = PrintText(Gui, DebugBuf, nameStart, GUI_GETCOLOR(GuiColor_Text));
#else
        rc2 NameRc = PrintText(Gui, name, nameStart, GUI_GETCOLOR(GuiColor_Text));
#endif
        
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, textRc);
        
        GuiPostAdvance(Gui, layout, AdvanceRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

INTERNAL_FUNCTION inline rc2 GuiGetGridRect(float WeightForThis, gui_element* Parent){
    gui_grid_item* Item = &Parent->Data.GridItem;
    
    if(Item->LastSumWeightInChildren < 0.0001f){
        Item->LastSumWeightInChildren = 3.0f;
    }
    
    float ThisAreaPercentage = WeightForThis / Item->LastSumWeightInChildren;
    float ThisStartPercentage = Item->SumWeightInChildren / Item->LastSumWeightInChildren;
    
    v2 ParentDim = GetRectDim(Item->InternalRect);
    
    rc2 Result = {};
    if(Item->Type == GuiGridItem_Row){
        float StartX = Item->InternalRect.min.x + ParentDim.x * ThisStartPercentage;
        float StartY = Item->InternalRect.min.y;
        
        v2 Dim = V2(ParentDim.x * ThisAreaPercentage, ParentDim.y);
        Result = RcMinDim(V2(StartX, StartY), Dim);
    }
    else if((Item->Type == GuiGridItem_Column) ||
            (Item->Type == GuiGridItem_Item) ||
            (Item->Type == GuiGridItem_Grid))
    {
        float StartX = Item->InternalRect.min.x;
        float StartY = Item->InternalRect.min.y + ParentDim.y * ThisStartPercentage;
        
        v2 Dim = V2(ParentDim.x, ParentDim.y * ThisAreaPercentage);
        Result = RcMinDim(V2(StartX, StartY), Dim);
    }
    else{
        // TODO(Dima): ?????
    }
    
    return(Result);
}

INTERNAL_FUNCTION gui_grid_item* GuiGridItemInit(gui_state* Gui, 
                                                 gui_element* elem, 
                                                 u32 GridItemType, 
                                                 float Weight)
{
    gui_grid_item* Item = &elem->Data.GridItem;
    switch(GridItemType){
        case GuiGridItem_GridHub:{
            gui_element* Parent = Gui->CurrentGridHub;
            ASSERT(Parent);
            // NOTE(Dima): Can spawn Grids only in Gridhubs
            ASSERT(Parent->Data.GridItem.Type == GuiGridItem_GridHub);
            
            gui_grid_item* HubItem = &Parent->Data.GridItem;
            
            if(HubItem->NextActiveID){
                HubItem->ActiveID = HubItem->NextActiveID;
                HubItem->NextActiveID = 0;
            }
            
        }break;
        
        case GuiGridItem_Grid:{
            Item->InternalRect = RcMinDim(V2(0.0f, 0.0f), 
                                          V2(Gui->FrameInfo.Width,
                                             Gui->FrameInfo.Height));
            
            gui_element* Parent = Gui->CurrentGridHub;
            ASSERT(Parent);
            // NOTE(Dima): Can spawn Grids only in Gridhubs
            ASSERT(Parent->Data.GridItem.Type == GuiGridItem_GridHub);
            
            gui_element* AtGrid = Parent->childSentinel->Next;
            
            gui_grid_item* HubItem = &Parent->Data.GridItem;
            
            // NOTE(Dima): If first element in grid hub -> set it's to active
            if(AtGrid->Prev == AtGrid->Next){
                HubItem->ActiveID = StringHashFNV(elem->Name);
            }
            
            while(AtGrid != Parent->childSentinel){
                if(AtGrid->ID == HubItem->ActiveID){
                    AtGrid->Opened = true;
                }
                else{
                    AtGrid->Opened = false;
                }
                
                AtGrid = AtGrid->Next;
            }
        }break;
        
        default:{
            Item->InternalRect = GuiGetGridRect(Weight, elem->parent);
            Item->Rect = GrowRectByPixels(Item->InternalRect, -10);
            
            gui_grid_item* ParentItem = &elem->parent->Data.GridItem;
            
            ParentItem->SumWeightInChildren += Weight;
        }break;
    }
    
    Item->Type = GridItemType;
    Item->SumWeightInChildren = 0.0f;
    
    if(!Item->IsInit){
        Item->TimeSinceNotHot = 9999.0f;
        
        Item->IsInit = true;
    }
    
    return(Item);
}

void GuiGridHubBegin(gui_state* Gui){
    char name[64];
    stbsp_sprintf(name, "GridHub: %d", Gui->curElement->childCount);
    
    gui_element* Elem = GuiBeginElement(Gui, name, 
                                        GuiElement_GridItem, 
                                        true);
    
    ASSERT(Gui->CurrentGridHub == 0);
    Gui->CurrentGridHub = Elem;
    
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          Elem,
                                          GuiGridItem_GridHub,
                                          0.0f);
}

void GuiGridHubEnd(gui_state* Gui){
    ASSERT(Gui->CurrentGridHub);
    Gui->CurrentGridHub = 0;
    
    GuiEndElement(Gui, GuiElement_GridItem);
}

void GuiGridBegin(gui_state* Gui, char* Name){
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          elem,
                                          GuiGridItem_Grid,
                                          0.0f);
}

void GuiGridEnd(gui_state* Gui){
    gui_grid_item* Item = &Gui->curElement->Data.GridItem;
    Item->LastSumWeightInChildren = Item->SumWeightInChildren;
    Item->SumWeightInChildren = 0;
    GuiEndElement(Gui, GuiElement_GridItem);
}

void GuiGridBeginRow(gui_state* Gui, float Weight = 1.0f){
    char Name[64];
    stbsp_sprintf(Name, "Row: %d", Gui->curElement->TmpCount);
    
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          elem,
                                          GuiGridItem_Row,
                                          Weight);
}

void GuiGridBeginColumn(gui_state* Gui, float Weight = 1.0f){
    char Name[64];
    stbsp_sprintf(Name, "Column: %d", Gui->curElement->TmpCount);
    
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          elem,
                                          GuiGridItem_Column,
                                          Weight);
}

void GuiGridEndRowOrColumn(gui_state* Gui){
    
    gui_grid_item* Item = &Gui->curElement->Data.GridItem;
    Item->LastSumWeightInChildren = Item->SumWeightInChildren;
    Item->SumWeightInChildren = 0;
    
    GuiEndElement(Gui, GuiElement_GridItem);
}

void GuiGridTileEmpty(gui_state* Gui, float Weight = 1.0f){
    b32 Result = 0;
    
    char Name[64];
    stbsp_sprintf(Name, "EmptyTile: %d", Gui->curElement->TmpCount);
    
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          elem,
                                          GuiGridItem_Item,
                                          Weight);
    
    GuiEndElement(Gui, GuiElement_GridItem);
}

b32 GuiGridTile(gui_state* Gui, char* Name, float Weight = 1.0f){
    b32 Result = 0;
    
    gui_element* elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          elem,
                                          GuiGridItem_Item,
                                          Weight);
    
    if(GuiElementOpenedInTree(elem)){
        v4 TileColor = GUI_GETCOLOR_COLSYS(ColorExt_gray53);
        
        rc2 WorkRect = Item->Rect;
        
        v4 NotActiveColor1 = GUI_GETCOLOR_COLSYS(ColorExt_gray53);
        v4 NotActiveColor2 = GUI_GETCOLOR_COLSYS(ColorExt_gray60);
        
        v4 ActiveColor1 = GUI_GETCOLOR_COLSYS(ColorExt_BlueViolet);
        v4 ActiveColor2 = GUI_GETCOLOR_COLSYS(ColorExt_VioletRed);
        
        v4 Color1;
        v4 Color2;
        
        gui_interaction Interaction = CreateInteraction(elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, WorkRect)){
            GuiSetHot(Gui, &Interaction, true);
            Color1 = ActiveColor1;
            Color2 = ActiveColor2;
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
                GuiReleaseInteraction(Gui, &Interaction);
                
                Result = true;
            }
            
            Item->TimeSinceNotHot = 0;
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
            float Time4Fadeout = 0.3f;
            
            float t = Item->TimeSinceNotHot / Time4Fadeout;
            t = Clamp01(t);
            
            Color1 = Lerp(ActiveColor1, NotActiveColor1, t);
            Color2 = Lerp(ActiveColor2, NotActiveColor2, t);
            
            Item->TimeSinceNotHot += Gui->FrameInfo.DeltaTime;
        }
        
        v4 OutlineColor = GUI_GETCOLOR_COLSYS(Color_Black);
        GuiPushBut(Gui, WorkRect, PushBut_Grad, Color1, Color2);
        GuiPushBut(Gui, WorkRect, PushBut_Outline, OutlineColor); 
        PrintTextCenteredInRectInternal(Gui->TileFont, 
                                        Gui->Stack, 
                                        Gui->Assets,
                                        Name, 
                                        WorkRect, 
                                        1.6f);
    }
    
    GuiEndElement(Gui, GuiElement_GridItem);
    
    return(Result);
}

void GuiTest(gui_state* Gui, float deltaTime){
    
    render_stack* renderStack = Gui->Stack;
    input_state* input = Gui->Input;
    
    char FPSBuf[64];
    stbsp_sprintf(FPSBuf, "FPS %.2f, delta time(sec) %.3f", 1.0f / deltaTime, deltaTime);
    
    static int lastFrameEntryCount = 0;
    static int lastFrameBytesUsed = 0;
    char StackInfo[256];
    stbsp_sprintf(StackInfo, "EntryCount: %d; BytesUsed: %d;", 
                  lastFrameEntryCount, 
                  lastFrameBytesUsed);
    
    char InterInfo[256];
    stbsp_sprintf(InterInfo, "Hot Interaction ID: %u Name: \"%s\" Priority: %u", 
                  Gui->HotInteraction.ID, 
                  Gui->HotInteraction.Name,
                  Gui->HotInteraction.Priority);
    GuiText(Gui, InterInfo);
    stbsp_sprintf(InterInfo, "Active Interaction ID: %u Name: \"%s\" Priority: %u", 
                  Gui->ActiveInteraction.ID,
                  Gui->ActiveInteraction.Name,
                  Gui->ActiveInteraction.Priority);
    GuiText(Gui, InterInfo);
    
#if 0 
    GuiGridHubBegin(Gui);
    GuiGridBegin(Gui, "Grid1");
    GuiGridBeginRow(Gui);
    if(GuiGridTile(Gui, "Tile1", 1.0f)){
        GuiGoToGrid(Gui, "Grid2");
    }
    
    if(GuiGridTile(Gui, "Tile2", 2.0f)){
        
    }
    
    if(GuiGridTile(Gui, "Tile3", 3.0f)){
        
    }
    GuiGridEndRowOrColumn(Gui);
    
    GuiGridTileEmpty(Gui, 3.0f);
    
    GuiGridBeginRow(Gui, 1.5f);
    GuiGridTile(Gui, "Tile4");
    GuiGridTile(Gui, "Tile5");
    GuiGridEndRowOrColumn(Gui);
    GuiGridEnd(Gui);
    
    GuiGridBegin(Gui, "Grid2");
    GuiGridBeginRow(Gui);
    if(GuiGridTile(Gui, "First", 3.0f)){
        GuiGoToGrid(Gui, "Grid1");
    }
    
    if(GuiGridTile(Gui, "Second", 2.0f)){
        
    }
    
    if(GuiGridTile(Gui, "Third", 3.0f)){
        
    }
    GuiGridEndRowOrColumn(Gui);
    GuiGridBeginRow(Gui, 0.5f);
    GuiGridTile(Gui, "4");
    GuiGridTile(Gui, "5");
    GuiGridEndRowOrColumn(Gui);
    GuiGridEnd(Gui);
    GuiGridHubEnd(Gui);
#endif
    
    //GuiUpdateWindows(Gui);
    GuiBeginTree(Gui, "Some text");
    GuiText(Gui, "Hello world");
    GuiText(Gui, FPSBuf);
    GuiText(Gui, StackInfo);
    GuiText(Gui, "I love Kate");
    GuiText(Gui, "I wish joy and happiness for everyone");
    char GuiTmpText[64];
    stbsp_sprintf(GuiTmpText, 
                  "Total GUI allocated elements %d", 
                  Gui->TotalAllocatedGuiElements);
    GuiText(Gui, GuiTmpText);
    
    GuiEndTree(Gui);
    
    //GuiPushDim(Gui, V2(100, 100));
    GuiBeginLayout(Gui, "layout1", GuiLayout_Window);
    static char InputTextBuf[256];
    GuiInputText(Gui, "Input Text", InputTextBuf, 256);
    static float TestFloat4Slider;
    static int TestInt4Slider;
    GuiSliderFloat(Gui, &Gui->fontScale, 0.5f, 1.5f, "Gui font scale", GuiSlider_Index);
    GuiSliderFloat(Gui, &TestFloat4Slider, -5.0f, 10.0f, "FloatSlider1", GuiSlider_Index);
    GuiSliderFloat(Gui, &TestFloat4Slider, -5.0f, 10.0f, "FloatSlider2", GuiSlider_Progress);
    GuiSliderInt(Gui, &TestInt4Slider, -4, 7, "IntSlider1", GuiSlider_Index);
    GuiSliderInt(Gui, &TestInt4Slider, -4, 7, "IntSlider2", GuiSlider_Progress);
    GuiBeginTree(Gui, "AddClearButtons");
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
        PushRect(renderStack, RcMinDim(V2(100 + i * 50, 100), V2(40, 40)));
    }
    GuiEndTree(Gui);
    
    GuiBeginTree(Gui, "Buttons and checkboxes");
    static b32 boolButtonValue;
    GuiBeginRow(Gui);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBoolButton(Gui, "boolButton123", &boolButtonValue);
    GuiBoolButton(Gui, "boolButton1234", &boolButtonValue);
    GuiBoolButton(Gui, "boolButtonasdfga", &boolButtonValue);
    GuiBoolButton(Gui, "boolButtonzxcvzxcb", &boolButtonValue);
    GuiEndRow(Gui);
    
    static b32 boolButtonOnOffValue;
    GuiBeginRow(Gui);
    GuiBeginColumn(Gui);
    GuiBoolButtonOnOff(Gui, "boolButtonOnOff", &boolButtonValue);
    GuiBoolButtonOnOff(Gui, "boolButtonOnOff1", &boolButtonValue);
    GuiBoolButtonOnOff(Gui, "boolButtonOnOff2", &boolButtonValue);
    GuiBoolButtonOnOff(Gui, "boolButtonOnOff3", &boolButtonValue);
    GuiEndColumn(Gui);
    
    GuiBeginColumn(Gui);
    static b32 checkboxValue1;
    static b32 checkboxValue2;
    static b32 checkboxValue3;
    static b32 checkboxValue4;
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiCheckbox(Gui, "Checkbox1", &checkboxValue2);
    GuiCheckbox(Gui, "Checkbox2", &checkboxValue3);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiEndColumn(Gui);
    
    GuiBeginColumn(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiCheckbox(Gui, "Checkbox1", &checkboxValue2);
    GuiCheckbox(Gui, "Checkbox2", &checkboxValue3);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiEndColumn(Gui);
    GuiEndRow(Gui);
    
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff4", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox4", &checkboxValue4);
    GuiEndTree(Gui);
    
    GuiBeginTree(Gui, "Some more buts");
    GuiBeginRow(Gui);
    GuiBeginColumn(Gui);
    GuiPushDim(Gui, V2(100, 40));
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff1", &boolButtonValue);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff2", &boolButtonValue);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff3", &boolButtonValue);
    GuiEndColumn(Gui);
    
    GuiBeginColumn(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiCheckbox(Gui, "Checkbox1", &checkboxValue2);
    GuiCheckbox(Gui, "Checkbox2", &checkboxValue3);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiEndColumn(Gui);
    
    GuiEndRow(Gui);
    GuiEndTree(Gui);
    
    GuiBeginTree(Gui, "TestCull");
    static u32 activeRadio = 0;
    GuiBeginRow(Gui);
    GuiBeginColumn(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiEndColumn(Gui);
    GuiBeginColumn(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiEndColumn(Gui);
    GuiBeginColumn(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiEndColumn(Gui);
    GuiBeginColumn(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiEndColumn(Gui);
    GuiBeginColumn(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiCheckbox(Gui, "Checkbox", &checkboxValue1);
    GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &boolButtonValue);
    GuiCheckbox(Gui, "Checkbox3", &checkboxValue4);
    GuiBoolButton(Gui, "boolButton", &boolButtonValue);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiEndRadioGroup(Gui);
    GuiEndColumn(Gui);
    GuiEndRow(Gui);
    GuiEndTree(Gui);
    
    GuiBeginRow(Gui);
    GuiBeginRadioGroup(Gui, "radioGroup1", &activeRadio, 0);
    GuiRadioButton(Gui, "radio0", 0);
    GuiRadioButton(Gui, "radio1", 1);
    GuiRadioButton(Gui, "radio2", 2);
    GuiRadioButton(Gui, "radio3", 3);
    GuiEndRadioGroup(Gui);
    
    char radioTxt[32];
    stbsp_sprintf(radioTxt, "radio but value: %u", activeRadio);
    GuiEndRow(Gui);
    GuiText(Gui, radioTxt);
    
    GuiTooltip(Gui, "Hello world!", input->MouseP);
    
    GuiEndLayout(Gui);
    
    lastFrameBytesUsed = renderStack->MemBlock.Used;
    lastFrameEntryCount = renderStack->EntryCount;
}