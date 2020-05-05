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
    elem->NextAlloc = Gui->FreeSentinel.NextAlloc;
    elem->PrevAlloc = &Gui->FreeSentinel;
    
    elem->NextAlloc->PrevAlloc = elem;
    elem->PrevAlloc->NextAlloc = elem;
}

INTERNAL_FUNCTION gui_element* GuiAllocateElement(
gui_state* Gui)
{
    gui_element* result = 0;
    
    if(DLIST_FREE_IS_EMPTY(Gui->FreeSentinel, NextAlloc)){
        const int count = 128;
        gui_element* elemPoolArray = PushArray(Gui->Mem, gui_element, count);
        
        for(int index = 0; 
            index < count;
            index++)
        {
            gui_element* elem = &elemPoolArray[index];
            
            DLIST_INSERT_BEFORE_SENTINEL(elem, Gui->FreeSentinel, NextAlloc, PrevAlloc);
        }
        
        Gui->TotalAllocatedGuiElements += count;
    }
    
    result = Gui->FreeSentinel.NextAlloc;
    
    // NOTE(Dima): Deallocating from Free list
    DLIST_REMOVE_ENTRY(result, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Allocating in Use list
    DLIST_INSERT_BEFORE_SENTINEL(result, Gui->UseSentinel, NextAlloc, PrevAlloc);
    
    return(result);
}

INTERNAL_FUNCTION gui_element* GuiInitElement(gui_state* Gui,
                                              char* Name,
                                              gui_element** Cur,
                                              u32 Type, 
                                              b32 InitOpened)
{
    gui_element* Result = 0;
    
    // NOTE(Dima): Try find element in hierarchy
    gui_element* ChildSentinel = (*Cur)->ChildSentinel;
    gui_element* At = 0;
    gui_element* Found = 0;
    if(ChildSentinel){
        At = ChildSentinel->Next;
        
        // NOTE(Dima): Here i try to find ### symbols. They mean that text after ### 
        // NOTE(Dima): will be ignored when calculating ID for this element
        char ActualNameBuf[64];
        int NameLen = StringLength(Name);
        int FoundIndicatorIndex = -1;
        for(int FindIndex = 0;
            FindIndex < NameLen - 3 + 1;
            FindIndex++)
        {
            if(Name[FindIndex] == '#' &&
               Name[FindIndex + 1] == '#' &&
               Name[FindIndex + 2] == '#')
            {
                FoundIndicatorIndex = FindIndex;
                break;
            }
        }
        
        if(FoundIndicatorIndex != -1){
            CopyStringsCount(ActualNameBuf, Name, FoundIndicatorIndex + 1); 
        }
        else{
            CopyStringsSafe(ActualNameBuf, sizeof(ActualNameBuf), Name);
        }
        
        u32 ID = StringHashFNV(ActualNameBuf);
        u32 InTreeID = ID;
        if(At->Parent){
            InTreeID = InTreeID * At->Parent->ID + 7853;
        }
        
        while(At != ChildSentinel){
            if(ID == At->ID){
                Found = At;
                break;
            }
            
            At = At->Next;
        }
        
        // NOTE(Dima): if element was not found - then allocate and initialize
        if(!Found){
            Found = GuiAllocateElement(Gui);
            
            Found->ID = ID;
            Found->InTreeID = InTreeID;
            
            Found->Parent = *Cur;
            Found->Type = Type;
            
            // NOTE(Dima): freeing data
            Found->Data = {};
            
            // NOTE(Dima): Initializing children sentinel
            if(Type == GuiElement_ChildrenSentinel ||
               Type == GuiElement_TempItem ||
               Type == GuiElement_None)
            {
                Found->ChildSentinel = 0;
            }
            else{
                Found->ChildSentinel = GuiAllocateElement(Gui);
                gui_element* fcs = Found->ChildSentinel;
                fcs->Next = fcs;
                fcs->Prev = fcs;
                fcs->Parent = Found;
                fcs->ChildSentinel = 0;
                CopyStrings(fcs->Name, "ChildrenSentinel");
                fcs->ID = StringHashFNV(fcs->Name);
                fcs->Type = GuiElement_ChildrenSentinel;
            }
            
            // NOTE(Dima): Initializing opened
            Found->Opened = InitOpened;
            
            // NOTE(Dima): Inserting to list
            Found->Next = ChildSentinel->Next;
            Found->Prev = ChildSentinel;
            
            Found->Next->Prev = Found;
            Found->Prev->Next = Found;
        }
        
        ASSERT(Found->Type == Type);
        
        *Cur = Found;
        
        Result = Found;
        
        // NOTE(Dima): Copy to element's name and to show name
        CopyStringsSafe(Found->Name, sizeof(Found->Name), ActualNameBuf);
        if(FoundIndicatorIndex != -1){
            ConcatStringsUnsafe(Found->NameToShow, 
                                ActualNameBuf, 
                                Name + FoundIndicatorIndex + 3);
        }
        else{
            CopyStringsSafe(Found->NameToShow, sizeof(Found->NameToShow),
                            ActualNameBuf);
        }
        
        
        // NOTE(Dima): Incrementing Parent childCount and setting depth
        Found->Depth = 0;
        if(Found->Parent){
            Found->Parent->ChildCount++;
            Found->Depth = Found->Parent->Depth;
            
            if(Found->Parent->Type == GuiElement_Tree){
                Found->Depth++;
            }
        }
        Found->ChildCount = 0;
        
        // NOTE(Dima): Incrementing temp counts for row column elements
        if(Result->Parent){
            ++Result->Parent->TmpCount;
        }
        Result->TmpCount = 0;
    }
    
    return(Result);
}


INTERNAL_FUNCTION gui_element* GuiBeginElement(gui_state* Gui,
                                               char* name,
                                               u32 Type,
                                               b32 opened)
{
    gui_element* Result = GuiInitElement(Gui, 
                                         name, 
                                         &Gui->CurElement, 
                                         Type,
                                         opened);
    
    return(Result);
}

INTERNAL_FUNCTION b32 GuiElementOpenedInTree(gui_element* elem){
    b32 Result = 1;
    gui_element* at = elem->Parent;
    
    while(at->Parent != 0){
        if(at->Opened != 1){
            Result = 0;
            break;
        }
        
        at = at->Parent;
    }
    
    return(Result);
}

INTERNAL_FUNCTION void GuiEndElement(gui_state* Gui, u32 Type)
{
    ASSERT(Gui->CurElement->Type == Type);
    
    gui_element* TmpParent = Gui->CurElement->Parent;
    int TmpCount = Gui->CurElement->TmpCount;
    
    if(Type == GuiElement_ChildrenSentinel ||
       Type == GuiElement_TempItem ||
       Type == GuiElement_None)
    {
        GuiRemoveElementFromList(Gui->CurElement);
        GuiDeallocateElement(Gui, Gui->CurElement);
    }
    
    Gui->CurElement->TmpCount = TmpCount;
    Gui->CurElement = TmpParent;
}

INTERNAL_FUNCTION void GuiFreeElement(gui_state* Gui,
                                      gui_element* elem)
{
    elem->NextAlloc->PrevAlloc = elem->PrevAlloc;
    elem->PrevAlloc->NextAlloc = elem->NextAlloc;
    
    elem->NextAlloc = Gui->FreeSentinel.NextAlloc;
    elem->PrevAlloc = &Gui->FreeSentinel;
    
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
    (*root)->Parent = 0;
    (*root)->Type = GuiElement_Root;
    CopyStrings((*root)->Name, "RootElement!!!");
    (*root)->ID = StringHashFNV((*root)->Name);
    
    (*root)->ChildCount = 0;
    (*root)->ChildSentinel = GuiAllocateElement(Gui);
    gui_element* rcs = (*root)->ChildSentinel;
    rcs->Next = rcs;
    rcs->Prev = rcs;
    rcs->Parent = (*root);
    rcs->ChildSentinel = 0;
    CopyStrings(rcs->Name, "RootChildrenSentinel");
    rcs->ID = StringHashFNV(rcs->Name);
    rcs->Type = GuiElement_ChildrenSentinel;
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
    Gui->MainFontID = GetFirst(Assets, GameAsset_LiberationMono);
    Gui->TileFontID = GetFirst(Assets, GameAsset_MollyJackFont);
    
    Gui->MainFont = LoadFont(Assets, Gui->MainFontID, ASSET_IMPORT_IMMEDIATE);
    Gui->TileFont = LoadFont(Assets, Gui->TileFontID, ASSET_IMPORT_IMMEDIATE);
    
    Gui->CheckboxMarkID = GetFirst(Assets, GameAsset_CheckboxMark);
    Gui->ChamomileID = GetFirst(Assets, GameAsset_ChamomileIcon);
    
    Gui->FontScale = 1.0f;
    
    // NOTE(Dima): Init layouts
    Gui->layoutCount = 1;
    Gui->rootLayout = {};
    DLIST_REFLECT_PTRS(Gui->rootLayout, Next, Prev);
    CopyStrings(Gui->rootLayout.Name, "RootLayout");
    Gui->rootLayout.ID = StringHashFNV(Gui->rootLayout.Name);
    
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
    DLIST_REFLECT_PTRS(Gui->FreeSentinel, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Gui->UseSentinel, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Initializing root element
    GuiInitRoot(Gui, &Gui->RootElement);
    
    // NOTE(Dima): Setting current element
    Gui->CurElement = Gui->RootElement;
    
    // NOTE(Dima): Initializing colors
    InitColorsState(&Gui->colorState, Gui->Mem);
    Gui->colors[GuiColor_Text] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->colors[GuiColor_Borders] = GUI_GETCOLOR_COLSYS(Color_NeoMint);
    Gui->colors[GuiColor_SliderValue] = GUI_GETCOLOR_COLSYS(Color_DarkMagenta);
    
    Gui->colors[GuiColor_Hot] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->colors[GuiColor_Active] = GUI_GETCOLOR_COLSYS(Color_Red);
    
    Gui->colors[GuiColor_ActiveGrad1] = ColorFromHex("#8e3b7a");
    Gui->colors[GuiColor_ActiveGrad2] = ColorFromHex("#dd4d5e");
    
    Gui->colors[GuiColor_InactiveGrad1] = ColorFromHex("#021330");
    Gui->colors[GuiColor_InactiveGrad2] = ColorFromHex("#277185");
    
    Gui->colors[GuiColor_HeaderActive] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->colors[GuiColor_HeaderInactive] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->colors[GuiColor_HeaderPreview] = GUI_GETCOLOR_COLSYS(Color_White);
    
    Gui->colors[GuiColor_BodyActive] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->colors[GuiColor_BodyInactive] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->colors[GuiColor_BodyPreview] = GUI_GETCOLOR_COLSYS(Color_White);
    
    Gui->colors[GuiColor_BackgroundActive] = GUI_GETCOLOR_COLSYS(Color_Orchid);
    Gui->colors[GuiColor_BackgroundInactive] = GUI_GETCOLOR_COLSYS(Color_Black);
    Gui->colors[GuiColor_BackgroundPreview] = GUI_GETCOLOR_COLSYS(ColorExt_gray5);
    
    Gui->colors[GuiColor_OutlineActive] = GUI_GETCOLOR_COLSYS(Color_Red);
    Gui->colors[GuiColor_OutlineInactive] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->colors[GuiColor_OutlinePreview] = GUI_GETCOLOR_COLSYS(Color_Black);
    
    
    Gui->colors[GuiColor_Graph0] = GUI_GETCOLOR_COLSYS(Color_White);
    Gui->colors[GuiColor_Graph1] = GUI_GETCOLOR_COLSYS(Color_Red);
    Gui->colors[GuiColor_Graph2] = GUI_GETCOLOR_COLSYS(Color_Green);
    Gui->colors[GuiColor_Graph3] = GUI_GETCOLOR_COLSYS(Color_Blue);
    Gui->colors[GuiColor_Graph4] = GUI_GETCOLOR_COLSYS(Color_Yellow);
    Gui->colors[GuiColor_Graph5] = GUI_GETCOLOR_COLSYS(Color_Magenta);
    Gui->colors[GuiColor_Graph6] = GUI_GETCOLOR_COLSYS(Color_Cyan);
    Gui->colors[GuiColor_Graph7] = GUI_GETCOLOR_COLSYS(ColorExt_gray50);
    Gui->colors[GuiColor_Graph8] = GUI_GETCOLOR_COLSYS(Color_PrettyBlue);
    Gui->colors[GuiColor_Graph9] = GUI_GETCOLOR_COLSYS(Color_Purple);
    Gui->colors[GuiColor_Graph10] = GUI_GETCOLOR_COLSYS(Color_Orange);
    Gui->colors[GuiColor_Graph11] = GUI_GETCOLOR_COLSYS(Color_Brown);
    Gui->colors[GuiColor_Graph12] = GUI_GETCOLOR_COLSYS(Color_Amber);
    Gui->colors[GuiColor_Graph13] = GUI_GETCOLOR_COLSYS(Color_Burlywood);
    Gui->colors[GuiColor_Graph14] = GUI_GETCOLOR_COLSYS(Color_DarkGoldenrod);
    Gui->colors[GuiColor_Graph15] = GUI_GETCOLOR_COLSYS(Color_OliveDrab);
    Gui->colors[GuiColor_Graph16] = GUI_GETCOLOR_COLSYS(Color_ToxicGreen);
    Gui->colors[GuiColor_Graph17] = GUI_GETCOLOR_COLSYS(Color_NeoMint);
    Gui->colors[GuiColor_Graph18] = GUI_GETCOLOR_COLSYS(Color_UltraViolet);
    Gui->colors[GuiColor_Graph19] = GUI_GETCOLOR_COLSYS(ColorExt_azure2);
    Gui->colors[GuiColor_Graph20] = GUI_GETCOLOR_COLSYS(ColorExt_chocolate);
    Gui->colors[GuiColor_Graph21] = GUI_GETCOLOR_COLSYS(ColorExt_coral2);
    Gui->colors[GuiColor_Graph22] = GUI_GETCOLOR_COLSYS(ColorExt_DarkSeaGreen2);
    Gui->colors[GuiColor_Graph23] = GUI_GETCOLOR_COLSYS(ColorExt_GreenYellow);
    Gui->colors[GuiColor_Graph24] = GUI_GETCOLOR_COLSYS(ColorExt_HotPink);
    Gui->colors[GuiColor_Graph25] = GUI_GETCOLOR_COLSYS(ColorExt_LemonChiffon4);
    Gui->colors[GuiColor_Graph26] = GUI_GETCOLOR_COLSYS(ColorExt_LightSalmon3);
    Gui->colors[GuiColor_Graph27] = GUI_GETCOLOR_COLSYS(ColorExt_MediumOrchid1);
    Gui->colors[GuiColor_Graph28] = GUI_GETCOLOR_COLSYS(ColorExt_NavajoWhite1);
    Gui->colors[GuiColor_Graph29] = GUI_GETCOLOR_COLSYS(ColorExt_PaleVioletRed3);
    Gui->colors[GuiColor_Graph30] = GUI_GETCOLOR_COLSYS(ColorExt_RosyBrown1);
    Gui->colors[GuiColor_Graph31] = GUI_GETCOLOR_COLSYS(ColorExt_SteelBlue3);
    Gui->colors[GuiColor_GraphCount] = GUI_GETCOLOR_COLSYS(Color_Black);
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
    
    v2 CurP = p;
    
    while(*at){
        
        if(text + CaretP == at){
            if(CaretPrintPOut){
                *CaretPrintPOut = CurP;
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
                    float bitmapMinY = CurP.y + Glyph->YOffset * Scale;
                    float bitmapMinX = CurP.x + Glyph->XOffset * Scale;
                    
                    PushOrLoadGlyph(Assets,
                                    stack, 
                                    V2(bitmapMinX, bitmapMinY), 
                                    bitmapDim, 
                                    Glyph->BitmapID,
                                    color);
                    
                }
                
                //CurP.x += ((float)Glyph->Advance * Scale);
                CurP.x += ((float)(Glyph->Advance - 0.5f) * Scale);
            }
            
        }
        else if(*at == '\t'){
            CurP.x += ((float)Font->AscenderHeight * 4 * Scale);
        }
        
        at++;
    }
    
    if(text + CaretP == at){
        if(CaretPrintPOut){
            *CaretPrintPOut = CurP;
        }
    }
    
    txtRc.Min.x = p.x;
    txtRc.Min.y = p.y - Font->AscenderHeight * Scale;
    txtRc.Max.x = CurP.x;
    txtRc.Max.y = CurP.y - Font->DescenderHeight * Scale;
    
    return(txtRc);
}

void PrintCaret(gui_state* Gui, v2 PrintP, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f)){
    float bmpScale = GuiGetLineAdvance(Gui);
    
    float CaretMinY = PrintP.y - GetScaledAscender(Gui->MainFont, Gui->FontScale);
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
    
    v2 CenterRc = Rect.Min + GetRectDim(Rect) * 0.5f;
    
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
                                    Gui->FontScale * Scale);
    
    return(result);
}

rc2 GetTextRect(gui_state* Gui, char* text, v2 p, float Scale){
    rc2 TextRc = PrintTextInternal(
        Gui->MainFont, 
        Gui->Stack, 
        Gui->Assets,
        text, p, 
        PrintTextOp_GetSize, 
        Gui->FontScale * Scale);
    
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
        Gui->FontScale * Scale,
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
        Gui->FontScale * Gui->FontScale,
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
                                                 Gui->FontScale * Scale,
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
    
    v2 Min = WindowRect.Min;
    v2 Max = WindowRect.Max;
    
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
    
    v2 ResultP = Res.result.Min;
    v2 ResultDim = GetRectDim(Res.result);
    
    if(P){
        *P = ResultP;
    }
    
    if(Dim){
        *Dim = ResultDim;
    }
}

inline void GuiGrowWindowRect(v2* P, v2* Dim, int PixelsGrow){
    rc2 CurRc = RcMinDim(*P, *Dim);
    
    rc2 NewRc = GrowRectByPixels(CurRc, PixelsGrow);
    v2 NewDim = GetRectDim(NewRc);
    
    NewDim.x = Max(NewDim.x, 40.0f);
    NewDim.y = Max(NewDim.y, 40.0f);
    
    *P = NewRc.Min;
    *Dim = NewDim;
}

INTERNAL_FUNCTION void GuiInitLayout(gui_state* Gui, 
                                     gui_layout* layout, 
                                     u32 LayoutFlags, 
                                     gui_element* LayoutElem,
                                     v2* Min,
                                     v2* Dim)
{
    // NOTE(Dima): initializing references
    LayoutElem->Data.Layout.ref = layout;
    layout->Elem = LayoutElem;
    
    // NOTE(Dima): Layout initializing
    layout->Flags = LayoutFlags;
    if(!LayoutElem->Data.IsInit){
        layout->Start = V2(0.0f, 0.0f);
        // NOTE(Dima): Layout dimension should be set anyways
        // NOTE(Dima): because of interaction that account to them
        layout->Dim = V2(Gui->Width, Gui->Height);
        
        if(Min){
            layout->Start = *Min;
        }
        
        if(Dim){
            layout->Dim = *Dim;
        }
        layout->At = layout->Start;
        
        LayoutElem->Data.IsInit = true;
    }
    
    // NOTE(Dima): Recalculate window rect always for clipping GUI interactions
    layout->Rect = RcMinDim(layout->At, layout->Dim);
    
    // NOTE(Dima): Initializing initial advance ctx
    layout->AdvanceRememberStack[0].Type = GuiAdvanceType_Column;
    layout->AdvanceRememberStack[0].RememberValue = layout->Start.y;
    layout->AdvanceRememberStack[0].Baseline = layout->Start.x;
    
    if(LayoutFlags & GuiLayout_Window){
        gui_interaction Interaction = CreateInteraction(LayoutElem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Small);
        if(LayoutFlags & GuiLayout_Move){
            GuiAnchor(Gui, "Anchor2", 
                      layout->Start,
                      V2(20, 20),
                      false, true,
                      &layout->Start,
                      &layout->Dim);
        }
        
        if(LayoutFlags & GuiLayout_Resize){
            GuiAnchor(Gui, "Anchor1", 
                      layout->Start + layout->Dim,
                      V2(20, 20),
                      true, true,
                      &layout->Start,
                      &layout->Dim);
        }
        
        rc2 windowRc = layout->Rect;
        
        PushRect(Gui->Stack, windowRc, V4(GUI_GETCOLOR(GuiColor_BackgroundPreview).rgb, 0.95f));
        
        v4 outlineColor = GUI_GETCOLOR_COLSYS(Color_White);
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
            
            if(KeyWentDown(Gui->Input, Key_Add)){
                GuiGrowWindowRect(&layout->Start, &layout->Dim, 20);
            }
            
            if(KeyWentDown(Gui->Input, Key_Subtract)){
                GuiGrowWindowRect(&layout->Start, &layout->Dim, -20);
            }
        }
        
        // NOTE(Dima): Pushing inner outline
        //PushRectOutline(Gui->Stack, windowRc, 2, outlineColor);
        
        // NOTE(Dima): Beginning GUI chunk
        BeginGuiChunk(Gui->Stack, windowRc);
    }
}

void GuiBeginLayout(gui_state* Gui, char* name, u32 layoutType, v2* P, v2* Dim){
    // NOTE(Dima): In list inserting
    u32 nameID = StringHashFNV(name);
    
    gui_layout* FoundLayout = 0;
    gui_layout* layoutAt = Gui->rootLayout.Next;
    for(layoutAt; layoutAt != &Gui->rootLayout; layoutAt = layoutAt->Next){
        if(nameID == layoutAt->ID){
            FoundLayout = layoutAt;
            break;
        }
    }
    
    if(!FoundLayout){
        FoundLayout = PushStruct(Gui->Mem, gui_layout);
        
        CopyStringsSafe(FoundLayout->Name, sizeof(FoundLayout->Name), name);
        FoundLayout->ID = nameID;
        
        FoundLayout->Next = Gui->rootLayout.Next;
        FoundLayout->Prev = &Gui->rootLayout;
        FoundLayout->Next->Prev = FoundLayout;
        FoundLayout->Prev->Next = FoundLayout;
        
        ++Gui->layoutCount;
    }
    
    // NOTE(Dima): Beginnning layout elem
    gui_element* LayoutElem = GuiBeginElement(Gui, name, GuiElement_Layout, true);
    
    GuiInitLayout(Gui, FoundLayout, layoutType, LayoutElem, P, Dim);
}

void GuiEndLayout(gui_state* Gui){
    gui_layout* lay = GetParentLayout(Gui);
    
    lay->At = lay->Start;
    
    if(lay->Flags & GuiLayout_Window){
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
    
    // NOTE(Dima): Deallocating Parent because it is not visible
    window->Next->Prev = window->Prev;
    window->Prev->Next = window->Next;
    
    window->Next = 0;
    window->Prev = 0;
    
    GuiDeallocateWindow(Gui, window);
    
    // NOTE(Dima): Return list shoud be empty after usage in this function
    Assert(Gui->windowSentinel4Returning.Next == &Gui->windowSentinel4Returning);
}

INTERNAL_FUNCTION void GuiUpdateWindow(gui_state* Gui, Gui_Window* window){
    
    window->layout.Start = window->rect.Min;
    
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
                v2 windowCenter = windowRc.Min + windowHalfDim;
                
                v2 MouseP = Gui->Input->MouseP;
                MouseP = ClampInRect(MouseP, windowRc);
                v2 diffFromCenter = MouseP - windowCenter;
                v2 diffRelative;
                diffRelative.x = diffFromCenter.x / windowHalfDim.x;
                diffRelative.y = diffFromCenter.y / windowHalfDim.y;
                v2 absDiff = diffRelative;
                absDiff.x = Abs(absDiff.x);
                absDiff.y = Abs(absDiff.y);
                
                float MaxAbsDiff = 0.35f;
                u32 snapType = GuiWindowSnap_Whole;
                if(diffRelative.x < 0){
                    if(absDiff.x > MaxAbsDiff){
                        MaxAbsDiff = absDiff.x;
                        snapType = GuiWindowSnap_Left;
                    }
                }
                else{
                    if(absDiff.x > MaxAbsDiff){
                        MaxAbsDiff = absDiff.x;
                        snapType = GuiWindowSnap_Right;
                    }
                }
                
                if(diffRelative.y < 0){
                    if(absDiff.y > MaxAbsDiff){
                        MaxAbsDiff = absDiff.y;
                        snapType = GuiWindowSnap_Top;
                    }
                }
                else{
                    if(absDiff.y > MaxAbsDiff){
                        MaxAbsDiff = absDiff.y;
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
    
    DEBUGSetMenuDataSource(DebugMenu_GUI, Gui);
    
    Gui->Input = Gui->FrameInfo.Input;
    Gui->Stack = Gui->FrameInfo.Stack;
    Gui->Width = Gui->FrameInfo.Width;
    Gui->Height = Gui->FrameInfo.Height;
    
    // NOTE(Dima): Beginning GUI chunk
    BeginGuiChunk(GuiFrameInfo.Stack, 
                  RcMinDim(V2(0.0f, 0.0f), 
                           V2(GuiFrameInfo.Width, GuiFrameInfo.Height)));
    
    // NOTE(Dima): Init root layout
    gui_element* LayoutElem = GuiBeginElement(Gui, 
                                              Gui->rootLayout.Name, 
                                              GuiElement_Layout, 
                                              true);
    
    GuiInitLayout(Gui, &Gui->rootLayout, JOY_ZERO_FLAGS, LayoutElem, 0, 0);
}

void GuiFrameEnd(gui_state* Gui){
    // NOTE(Dima): Deinit root layout
    Gui->rootLayout.At = Gui->rootLayout.Start;
    GuiEndElement(Gui, GuiElement_Layout);
}

void GuiFramePrepare4Render(gui_state* Gui){
    for(int TooltipIndex = 0; TooltipIndex < Gui->TooltipIndex; TooltipIndex++){
        Gui_Tooltip* ttip= &Gui->Tooltips[TooltipIndex];
        
        PrintText(Gui, ttip->text, ttip->at, GUI_GETCOLOR(GuiColor_Text), 1.0f);
    }
    Gui->TooltipIndex = 0;
    
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
                    
                    *OffsetInAnchor = MouseP - AnchorRect.Min;
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

// NOTE(Dima): Default advance Type is Column advance
inline void GuiPreAdvance(gui_state* Gui, gui_layout* layout, 
                          int Depth)
{
    GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
    b32 rowStarted = ctx->Type == GuiAdvanceType_Row;
    
    float RememberValue = ctx->RememberValue;
    
    if(rowStarted){
        layout->At.y = ctx->Baseline;
    }
    else{
        layout->At.x = ctx->Baseline;
        layout->At.y += GuiGetBaseline(Gui);
    }
    
    layout->At += V2(Depth * 2.0f * GetScaledAscender(Gui->MainFont, Gui->FontScale), 0.0f);
}

inline void GuiPostAdvance(gui_state* Gui, 
                           gui_layout* layout, 
                           rc2 ElementRect)
{
    GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
    b32 rowStarted = (ctx->Type == GuiAdvanceType_Row);
    
    float RememberValue = ctx->RememberValue;
    
    float toX = ElementRect.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.25f;
    //float toY = ElementRect.Max.y + GetLineAdvance(Gui->MainFont, Gui->FontScale) * 0.15f;
    float toY = ElementRect.Max.y + GetLineAdvance(Gui->MainFont, Gui->FontScale) * 0.05f;
    
    if(rowStarted){
        layout->At.x = toX;
        ctx->Maximum = Max(ctx->Maximum, toY);
    }
    else{
        layout->At.y = toY;
        ctx->Maximum = Max(ctx->Maximum, toX);
    }
    ctx->MaxHorz = Max(ctx->MaxHorz, toX);
    ctx->MaxVert = Max(ctx->MaxVert, toY);
}


inline GuiAdvanceCtx GuiRowAdvanceCtx(float rememberX, float Baseline){
    GuiAdvanceCtx ctx = {};
    
    ctx.Type = GuiAdvanceType_Row;
    ctx.RememberValue = rememberX;
    ctx.Baseline = Baseline;
    
    return(ctx);
}

inline GuiAdvanceCtx GuiColumnAdvanceCtx(float rememberY, float Baseline){
    GuiAdvanceCtx ctx = {};
    
    ctx.Type = GuiAdvanceType_Column;
    ctx.RememberValue = rememberY;
    ctx.Baseline = Baseline;
    
    return(ctx);
}

void GuiBeginRow(gui_state* Gui){
    char name[64];
    stbsp_sprintf(name, "Row or Column: %d", Gui->CurElement->ChildCount);
    
    gui_element* elem = GuiBeginElement(Gui, name, GuiElement_RowColumn, true);
    if(GuiElementOpenedInTree(elem)){
        
        gui_layout* layout = GetParentLayout(Gui);
        
        Assert(layout->StackCurrentIndex < ArrayCount(layout->AdvanceRememberStack));
        
        layout->AdvanceRememberStack[++layout->StackCurrentIndex] = 
            GuiRowAdvanceCtx(layout->At.x, layout->At.y + GuiGetBaseline(Gui));
    }
}

void GuiBeginColumn(gui_state* Gui){
    char name[64];
    stbsp_sprintf(name, "Row or Column: %d", Gui->CurElement->ChildCount);
    
    gui_element* elem = GuiBeginElement(Gui, name, GuiElement_RowColumn, true);
    if(GuiElementOpenedInTree(elem)){
        
        gui_layout* layout = GetParentLayout(Gui);
        
        Assert(layout->StackCurrentIndex < ArrayCount(layout->AdvanceRememberStack));
        
        layout->AdvanceRememberStack[++layout->StackCurrentIndex] = 
            GuiColumnAdvanceCtx(layout->At.y, layout->At.x);
    }
}

void GuiEndRow(gui_state* Gui){
    if(GuiElementOpenedInTree(Gui->CurElement)){
        
        gui_layout* layout = GetParentLayout(Gui);
        
        Assert(layout->StackCurrentIndex >= 1);
        
        GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex--];
        Assert(ctx->Type == GuiAdvanceType_Row);
        
        // NOTE(Dima): Set X value to the remembered value
        layout->At.x = ctx->RememberValue;
        // NOTE(Dima): Set Y value to the largest vertical value
        layout->At.y = ctx->MaxVert;
        
        GuiAdvanceCtx* newCtx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
        newCtx->Maximum = Max(ctx->Maximum, newCtx->Maximum);
        
        newCtx->MaxHorz = Max(ctx->MaxHorz, newCtx->MaxHorz);
        newCtx->MaxVert = Max(ctx->MaxVert, newCtx->MaxVert);
        
        ctx->Maximum = 0.0f;
    }
    
    GuiEndElement(Gui, GuiElement_RowColumn);
}

void GuiEndColumn(gui_state* Gui){
    if(GuiElementOpenedInTree(Gui->CurElement)){
        
        gui_layout* layout = GetParentLayout(Gui);
        
        Assert(layout->StackCurrentIndex >= 1);
        
        GuiAdvanceCtx* ctx = &layout->AdvanceRememberStack[layout->StackCurrentIndex--];
        Assert(ctx->Type == GuiAdvanceType_Column);
        
        // NOTE(Dima): Set Y Value to the remembered value
        layout->At.y = ctx->RememberValue;
        // NOTE(Dima): Set X value to the Maximum horizontal value
        layout->At.x = ctx->MaxHorz;
        
        GuiAdvanceCtx* newCtx = &layout->AdvanceRememberStack[layout->StackCurrentIndex];
        newCtx->Maximum = Max(ctx->Maximum, newCtx->Maximum);
        
        newCtx->MaxHorz = Max(ctx->MaxHorz, newCtx->MaxHorz);
        newCtx->MaxVert = Max(ctx->MaxVert, newCtx->MaxVert);
        
        ctx->Maximum = 0.0f;
    }
    
    GuiEndElement(Gui, GuiElement_RowColumn);
}

void BeginDimension(gui_state* Gui, u32 Type, v2 Dim){
    if(GuiElementOpenedInTree(Gui->CurElement)){
        gui_layout* Layout = GetParentLayout(Gui);
        
        beginned_dimension* BeginnedDim = &Layout->BeginnedDimension;
        BeginnedDim->Type = Type;
        BeginnedDim->Dim = Dim;
        
        Assert(!Layout->DimensionIsBeginned);
        Layout->DimensionIsBeginned = true;
    }
}

void EndDimension(gui_state* Gui){
    if(GuiElementOpenedInTree(Gui->CurElement)){
        gui_layout* Layout = GetParentLayout(Gui);
        
        Assert(Layout->DimensionIsBeginned);
        Layout->DimensionIsBeginned = false;
    }
}

enum Push_But_Type{
    PushBut_Color,
    PushBut_Color1Outline2,
    PushBut_Grad,
    PushBut_Outline,
    
    PushBut_HotOutline,
    PushBut_ActiveGrad,
    PushBut_InactiveGrad,
    
    PushBut_BackgroundActive,
    PushBut_BackgroundInactive,
    PushBut_BackgroundPreview,
    
    PushBut_OutlineActive,
    PushBut_OutlineInactive,
    PushBut_OutlinePreview,
};

#define DEFAULT_OUTLINE_WIDTH 1
INTERNAL_FUNCTION void GuiPushBut(gui_state* Gui, 
                                  rc2 rect, 
                                  u32 Type, 
                                  v4 Color1 = V4(0.0f, 0.0f, 0.0f, 1.0f),
                                  v4 Color2 = V4(0.0f, 0.0f, 0.0f, 1.0f),
                                  int OutlineWidth = DEFAULT_OUTLINE_WIDTH)
{
    
    switch(Type){
        // NOTE(Dima): These are with user colors
        case PushBut_Color:{
            PushRect(Gui->Stack, rect, Color1);
        }break;
        
        case PushBut_Color1Outline2:{
            PushRect(Gui->Stack, rect, Color1);
            PushRectOutline(Gui->Stack, rect, OutlineWidth, Color2);
        }break;
        
        case PushBut_Grad:{
            PushGradient(Gui->Stack, rect, 
                         Color1, Color2, 
                         RenderEntryGradient_Vertical);
        }break;
        
        case PushBut_Outline:{
            PushRectOutline(Gui->Stack, rect, DEFAULT_OUTLINE_WIDTH, Color1);
        }break;
        
        // NOTE(Dima): These take colors from GUI tables
        
        case PushBut_ActiveGrad:{
            PushGradient(
                Gui->Stack, rect, 
                GUI_GETCOLOR(GuiColor_ActiveGrad2),
                GUI_GETCOLOR(GuiColor_ActiveGrad1),
                RenderEntryGradient_Vertical);
        }break;
        
        case PushBut_InactiveGrad:{
            PushGradient(
                Gui->Stack, rect, 
                GUI_GETCOLOR(GuiColor_InactiveGrad2),
                GUI_GETCOLOR(GuiColor_InactiveGrad1),
                RenderEntryGradient_Vertical);
        }break;
        
        case PushBut_BackgroundActive:{
            PushRect(Gui->Stack, rect, 
                     GUI_GETCOLOR(GuiColor_BackgroundActive));
        }break;
        
        case PushBut_BackgroundInactive:{
            PushRect(Gui->Stack, rect, 
                     GUI_GETCOLOR(GuiColor_BackgroundInactive));
        }break;
        
        case PushBut_BackgroundPreview:{
            PushRect(Gui->Stack, rect, 
                     GUI_GETCOLOR(GuiColor_BackgroundPreview));
        }break;
        
        case PushBut_HotOutline:{
            PushRectOutline(Gui->Stack, rect, DEFAULT_OUTLINE_WIDTH, 
                            GUI_GETCOLOR(GuiColor_Hot));
        }break;
    }
}

inline b32 PotentiallyVisible(gui_layout* lay, v2 dim){
    rc2 layRc = RcMinDim(lay->Start, lay->Dim);
    rc2 targetRc = RcMinDim(lay->At, dim);
    
    b32 res = BoxIntersectsWithBox(layRc, targetRc);
    
    return(res);
}

inline b32 PotentiallyVisibleSmall(gui_layout* lay){
    v2 dim = V2(200, 40);
    
    return(PotentiallyVisible(lay, dim));
}

inline b32 PotentiallyVisibleBig(gui_layout* Layout){
    v2 Dim = V2(1000, 100);
    
    return(PotentiallyVisible(Layout, Dim));
}

void GuiTooltip(gui_state* Gui, char* tooltipText, v2 at){
    Assert(Gui->TooltipIndex < GUI_MAX_TOOLTIPS);
    Gui_Tooltip* ttip = &Gui->Tooltips[Gui->TooltipIndex++];
    
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
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(elem) && 
       PotentiallyVisibleSmall(Layout))
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
        if(Interaction.WasHotInInteraction){
            AnchorColor = GUI_GETCOLOR_COLSYS(Color_Blue);
        }
        
        PushRect(Gui->Stack, WorkRect, AnchorColor);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiBeginTree(gui_state* Gui, char* name){
    gui_element* Elem = GuiBeginElement(Gui, name, GuiElement_Tree, false);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && PotentiallyVisibleSmall(Layout))
    {
        gui_layout* Layout = GetParentLayout(Gui);
        
        // NOTE(Dima): Calculate how much Parent element are trees to advance in x a little
        int ParentTreesCount = 0;
        gui_element* At = Elem->Parent;
        while(At != 0){
            if(At->Type == GuiElement_Tree){
                ParentTreesCount++;
            }
            
            At = At->Parent;
        }
        
        //V2(ParentTreesCount * GetScaledAscender(Gui->MainFont, Gui->FontScale), 0.0f)
        
        // NOTE(Dima): Pre-advancing
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        rc2 textRc = GetTextRect(Gui, Elem->NameToShow, Layout->At);
        textRc = GetTxtElemRect(Gui, Layout, textRc);
        
        v4 TextColor;
        if(Elem->Opened){
            TextColor = GUI_GETCOLOR(GuiColor_HeaderActive);
            GuiPushBut(Gui, textRc, PushBut_BackgroundActive);
        }
        else{
            TextColor = GUI_GETCOLOR(GuiColor_HeaderInactive);
        }
        
        gui_interaction Interaction = CreateInteraction(Elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, textRc)){
            GuiSetHot(Gui, &Interaction, true);
            TextColor = GUI_GETCOLOR(GuiColor_Hot);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
                GuiReleaseInteraction(Gui, &Interaction);
                
                Elem->Opened = !Elem->Opened;
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        PrintTextCenteredInRect(Gui, Elem->NameToShow, textRc, 1.0f, TextColor);
        
        GuiPostAdvance(Gui, Layout, textRc);
    }
}

void GuiEndTree(gui_state* Gui){
    GuiEndElement(Gui, GuiElement_Tree);
}

void GuiText(gui_state* Gui, char* text){
    gui_element* Elem = GuiBeginElement(Gui, text, GuiElement_TempItem, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        rc2 textRc = PrintText(Gui, text, Layout->At, GUI_GETCOLOR(GuiColor_Text));
        
        GuiPostAdvance(Gui, Layout, textRc);
    }
    GuiEndElement(Gui, GuiElement_TempItem);
}

b32 GuiLinkButton(gui_state* Gui, char* ButtonName){
    b32 result = 0;
    
    gui_element* Elem = GuiBeginElement(Gui, ButtonName, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(Gui, Elem->NameToShow, Layout->At);
        
        // NOTE(Dima): Event processing
        v4 textColor = GUI_GETCOLOR(GuiColor_HeaderInactive);
        
        gui_interaction Interaction = CreateInteraction(Elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, textRc)){
            GuiSetHot(Gui, &Interaction, true);
            textColor = GUI_GETCOLOR(GuiColor_HeaderActive);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
                GuiReleaseInteraction(Gui, &Interaction);
                result = 1;
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        PrintTextCenteredInRect(Gui, Elem->NameToShow, textRc, 1.0f, textColor);
        
        GuiPostAdvance(Gui, Layout, textRc);
    }
    GuiEndElement(Gui, GuiElement_Item);
    
    return(result);
}

b32 GuiButton(gui_state* Gui, char* ButtonName){
    b32 Result = false;
    
    gui_element* Elem = GuiBeginElement(Gui, ButtonName, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Printing button and text
        rc2 textRc = GetTextRect(Gui, Elem->NameToShow, Layout->At);
        textRc = GetTxtElemRect(Gui, Layout, textRc);
        GuiPushBut(Gui, textRc, PushBut_BackgroundInactive);
        
        // NOTE(Dima): Event processing
        v4 textColor = GUI_GETCOLOR(GuiColor_HeaderInactive);
        
        gui_interaction Interaction = CreateInteraction(Elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, textRc)){
            GuiSetHot(Gui, &Interaction, true);
            
            textColor = GUI_GETCOLOR(GuiColor_Hot);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        if(Interaction.WasActiveInInteraction){
            Result = true;
        }
        
        PrintTextCenteredInRect(Gui, Elem->NameToShow, textRc, 1.0f, textColor);
        
        GuiPostAdvance(Gui, Layout, textRc);
    }
    GuiEndElement(Gui, GuiElement_Item);
    
    return(Result);
}

void GuiShowBool(gui_state* Gui, char* Name, b32 Value){
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_TempItem, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Printing button and text
        rc2 ButRc = GetTextRect(Gui, "False", Layout->At);
        ButRc = GetTxtElemRect(Gui, Layout, ButRc);
        GuiPushBut(Gui, ButRc, PushBut_BackgroundInactive);
        
        char ButtonText[16];
        if(Value){
            CopyStrings(ButtonText, "True");
        }
        else{
            CopyStrings(ButtonText, "False");
        }
        
        PrintTextCenteredInRect(Gui, ButtonText, ButRc, 1.0f, 
                                GUI_GETCOLOR(GuiColor_HeaderInactive));
        
        
        // NOTE(Dima): Button name text printing
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, ButRc, Gui->FontScale);
        v2 NameStart = V2(ButRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        rc2 NameRc = PrintText(Gui, Name, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        GuiPostAdvance(Gui, Layout, GetBoundingRect(NameRc, ButRc));
    }
    
    GuiEndElement(Gui, GuiElement_TempItem);
}

void GuiShowInt(gui_state* Gui, char* Name, int Value){
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_TempItem, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Printing button and text
        char Buf[16];
        IntegerToString(Value, Buf);
        
        rc2 ButRc = GetTextRect(Gui, Buf, Layout->At);
        ButRc = GetTxtElemRect(Gui, Layout, ButRc);
        GuiPushBut(Gui, ButRc, PushBut_BackgroundInactive);
        
        PrintTextCenteredInRect(Gui, Buf, ButRc, 1.0f, 
                                GUI_GETCOLOR(GuiColor_HeaderInactive));
        
        
        // NOTE(Dima): Button name text printing
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, ButRc, Gui->FontScale);
        v2 NameStart = V2(ButRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        rc2 NameRc = PrintText(Gui, Name, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        GuiPostAdvance(Gui, Layout, GetBoundingRect(NameRc, ButRc));
    }
    
    GuiEndElement(Gui, GuiElement_TempItem);
}

void GuiShowFloat(gui_state* Gui, char* Name, float Value){
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_TempItem, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Printing button and text
        char Buf[32];
        stbsp_sprintf(Buf, "%.3f", Value);
        
        rc2 ButRc = GetTextRect(Gui, Buf, Layout->At);
        ButRc = GetTxtElemRect(Gui, Layout, ButRc);
        GuiPushBut(Gui, ButRc, PushBut_BackgroundInactive);
        
        PrintTextCenteredInRect(Gui, Buf, ButRc, 1.0f, 
                                GUI_GETCOLOR(GuiColor_HeaderInactive));
        
        
        // NOTE(Dima): Button name text printing
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, ButRc, Gui->FontScale);
        v2 NameStart = V2(ButRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        rc2 NameRc = PrintText(Gui, Name, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        GuiPostAdvance(Gui, Layout, GetBoundingRect(NameRc, ButRc));
    }
    
    GuiEndElement(Gui, GuiElement_TempItem);
}

b32 GuiBoolButton(gui_state* Gui, char* ButtonName, b32* Value){
    gui_element* Elem = GuiBeginElement(Gui, ButtonName, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    b32 Result = false;
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Printing button and text
        rc2 ButRc = GetTextRect(Gui, "False", Layout->At);
        ButRc = GetTxtElemRect(Gui, Layout, ButRc);
        
        b32 WasHot = false;
        char ButtonText[16];
        CopyStrings(ButtonText, "Error");
        if(Value){
            if(*Value == 0){
                CopyStrings(ButtonText, "False");
            }
            else{
                CopyStrings(ButtonText, "True");
                
                Result = true;
            }
            
            gui_interaction Interaction = CreateInteraction(Elem, 
                                                            GuiInteraction_BoolInRect,
                                                            GuiPriority_Avg);
            
            gui_interaction_data_bool_in_rect* BoolInRect = &Interaction.Data.BoolInRect;
            
            BoolInRect->Value = Value;
            BoolInRect->Rect = ButRc;
            
            GuiInteract(Gui, &Interaction);
            
            if(Interaction.WasHotInInteraction){
                WasHot = true;
            }
        }
        
        v4 TextColor;
        if(Result){
            TextColor = GUI_GETCOLOR(GuiColor_HeaderActive);
            GuiPushBut(Gui, ButRc, PushBut_BackgroundActive);
        }
        else{
            TextColor = GUI_GETCOLOR(GuiColor_HeaderInactive);
            GuiPushBut(Gui, ButRc, PushBut_BackgroundInactive);
        }
        
        if(WasHot){
            TextColor = GUI_GETCOLOR(GuiColor_Hot);
        }
        
        // NOTE(Dima): Printing value
        PrintTextCenteredInRect(Gui, ButtonText, ButRc, 1.0f,  TextColor);
        
        // NOTE(Dima): Button name text printing
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, ButRc, Gui->FontScale);
        v2 NameStart = V2(ButRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        rc2 NameRc = PrintText(Gui, Elem->NameToShow, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        GuiPostAdvance(Gui, Layout, GetBoundingRect(NameRc, ButRc));
    }
    
    GuiEndElement(Gui, GuiElement_Item);
    
    return(Result);
}

b32 GuiBoolButtonOnOff(gui_state* Gui, char* ButtonName, b32* Value){
    gui_element* Elem = GuiBeginElement(Gui, ButtonName, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    b32 Result = false;
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Button printing
        rc2 ButRc = GetTextRect(Gui, "OFF", Layout->At);
        ButRc = GetTxtElemRect(Gui, Layout, ButRc);
        
        // NOTE(Dima): Button name text printing
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, ButRc, Gui->FontScale);
        v2 NameStart = V2(ButRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        rc2 NameRc = PrintText(Gui, Elem->NameToShow, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        // NOTE(Dima): Event processing
        b32 WasHot = false;
        char ButtonText[4];
        CopyStrings(ButtonText, "ERR");
        if(Value){
            if(*Value){
                CopyStrings(ButtonText, "ON");
            }
            else{
                CopyStrings(ButtonText, "OFF");
            }
            
            
            gui_interaction Interaction = CreateInteraction(Elem, 
                                                            GuiInteraction_BoolInRect,
                                                            GuiPriority_Avg);
            
            gui_interaction_data_bool_in_rect* BoolInRect = &Interaction.Data.BoolInRect;
            
            BoolInRect->Value = Value;
            BoolInRect->Rect = GetBoundingRect(ButRc, NameRc);
            
            GuiInteract(Gui, &Interaction);
            
            if(Interaction.WasHotInInteraction){
                WasHot = true;
            }
            
            if(Interaction.WasActiveInInteraction){
                Result = true;
            }
        }
        
        v4 ButtonTextC;
        if(Value && *Value){
            ButtonTextC = GUI_GETCOLOR(GuiColor_HeaderActive);
            GuiPushBut(Gui, ButRc, PushBut_BackgroundActive);
        }
        else{
            ButtonTextC = GUI_GETCOLOR(GuiColor_HeaderInactive);
            GuiPushBut(Gui, ButRc, PushBut_BackgroundInactive);
        }
        
        if(WasHot){
            ButtonTextC = GUI_GETCOLOR(GuiColor_Hot);
        }
        
        PrintTextCenteredInRect(Gui, ButtonText, ButRc, 1.0f, ButtonTextC);
        
        rc2 AdvanceRect = GetBoundingRect(ButRc, NameRc);
        GuiPostAdvance(Gui, Layout, AdvanceRect);
    }
    GuiEndElement(Gui, GuiElement_Item);
    
    return(Result);
}

b32 GuiCheckbox(gui_state* Gui, char* Name, b32* Value){
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    b32 Result = false;
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Checkbox rendering
        float chkSize = GetLineAdvance(Gui->MainFont, Gui->FontScale);
        rc2 chkRect;
        chkRect.Min = V2(Layout->At.x, Layout->At.y - GetScaledAscender(Gui->MainFont, Gui->FontScale));
        chkRect.Max = chkRect.Min + V2(chkSize, chkSize);
        chkRect = GetTxtElemRect(Gui, Layout, chkRect);
        
        // NOTE(Dima): Event processing
        
        b32 WasHot = false;
        if(Value){
            
            gui_interaction Interaction = CreateInteraction(Elem, 
                                                            GuiInteraction_BoolInRect,
                                                            GuiPriority_Avg);
            
            gui_interaction_data_bool_in_rect* BoolInRect = &Interaction.Data.BoolInRect;
            
            BoolInRect->Value = Value;
            BoolInRect->Rect = chkRect;
            
            GuiInteract(Gui, &Interaction);
            
            WasHot = Interaction.WasHotInInteraction;
            
            if(Interaction.WasActiveInInteraction){
                Result = true;
            }
        }
        
        if(Value && *Value){
            GuiPushBut(Gui, chkRect, PushBut_BackgroundActive);
            
            PushOrLoadGlyph(Gui->Assets, 
                            Gui->Stack,
                            chkRect.Min, 
                            GetRectDim(chkRect),
                            Gui->CheckboxMarkID,
                            V4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        else{
            GuiPushBut(Gui, chkRect, PushBut_BackgroundInactive);
        }
        
        if(WasHot){
            
        }
        
        // NOTE(Dima): Button name text printing
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, chkRect, Gui->FontScale);
        v2 NameStart = V2(chkRect.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        rc2 nameRc = PrintText(Gui, Elem->NameToShow, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 advanceRect = GetBoundingRect(chkRect, nameRc);
        GuiPostAdvance(Gui, Layout, advanceRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
    
    return(Result);
}

void GuiBeginRadioGroup(
gui_state* Gui, 
char* name, 
u32* ref, 
u32 defaultId) 
{
    gui_element* Element = GuiBeginElement(Gui, 
                                           name, 
                                           GuiElement_RadioGroup, 
                                           true);
    
    if (!Element->Data.IsInit) {
        Element->Data.RadioGroup.activeId = defaultId;
        Element->Data.RadioGroup.ref = ref;
        
        Element->Data.IsInit = 1;
    }
}

INTERNAL_FUNCTION inline gui_element* 
GuiFindRadioGroupParent(gui_element* CurElement) {
    gui_element* Result = 0;
    
    gui_element* At = CurElement;
    while (At != 0) {
        if (At->Type == GuiElement_RadioGroup) {
            Result = At;
            break;
        }
        
        At = At->Parent;
    }
    
    return(Result);
}


void GuiRadioButton(gui_state* Gui, char* Name, u32 uniqueId) {
    gui_element* RadioBut = GuiBeginElement(Gui, Name, GuiElement_Item, true);
    gui_element* radioGroup = GuiFindRadioGroupParent(Gui->CurElement);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if (radioGroup && 
        GuiElementOpenedInTree(RadioBut) && 
        PotentiallyVisibleSmall(Layout)) 
    {
        b32 IsActive = false;
        if (radioGroup->Data.RadioGroup.activeId == uniqueId) {
            IsActive = true;
        }
        
        GuiPreAdvance(Gui, Layout, RadioBut->Depth);
        
        // NOTE(Dima): Printing button and text
        rc2 TextRc = GetTextRect(Gui, RadioBut->NameToShow, Layout->At);
        TextRc = GetTxtElemRect(Gui, Layout, TextRc);
        
        b32 WasHot = false;
        
        gui_interaction Interaction = CreateInteraction(RadioBut, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if (MouseInInteractiveArea(Gui, TextRc)) {
            GuiSetHot(Gui, &Interaction, true);
            
            WasHot = true;
            
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
        
        v4 TextColor;
        if(IsActive){
            TextColor = GUI_GETCOLOR(GuiColor_HeaderActive);
            GuiPushBut(Gui, TextRc, PushBut_BackgroundActive);
        }
        else{
            TextColor = GUI_GETCOLOR(GuiColor_HeaderInactive);
        }
        
        if(WasHot){
            TextColor = GUI_GETCOLOR(GuiColor_Hot);
        }
        
        PrintTextCenteredInRect(Gui, RadioBut->NameToShow, TextRc, 1.0f, TextColor);
        
        GuiPostAdvance(Gui, Layout, TextRc);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiEndRadioGroup(gui_state* Gui) {
    GuiEndElement(Gui, GuiElement_RadioGroup);
}

void GuiSliderInt(gui_state* Gui, int* Value, int Min, int Max, char* Name, u32 Style){
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        rc2 SlideRc = GetTextRect(Gui, 
                                  "                ", 
                                  Layout->At);
        SlideRc = GetTxtElemRect(Gui, Layout, SlideRc);
        GuiPushBut(Gui, SlideRc, PushBut_BackgroundInactive);
        
        float SlideRcWidth = GetRectWidth(SlideRc);
        float SlideRcHeight = GetRectHeight(SlideRc);
        
        char Buf[32];
        
        gui_interaction Interaction = CreateInteraction(Elem, 
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
        
        if(GuiIsActive(Gui, &Interaction) && (Style != GuiSlider_ProgressNonModify)){
            float MousePercentage = Clamp01((Gui->Input->MouseP.x - SlideRc.Min.x) / SlideRcWidth);
            
            float FloatedValue = Min + (float)((Max - Min) * MousePercentage);
            
            int NewValue = SafeTruncateToInt(FloatedValue);
            
            *Value = NewValue;
            
            if(KeyWentUp(Gui->Input, MouseKey_Left)){
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        
        if(Value){
            float ValuePercentage = (float)(*Value - Min) / (float)(Max - Min);
            
            if((Style == GuiSlider_Progress) || (Style == GuiSlider_ProgressNonModify)){
                rc2 FillRc;
                FillRc.Min = SlideRc.Min;
                FillRc.Max = V2(SlideRc.Min.x + SlideRcWidth * ValuePercentage, SlideRc.Max.y);
                
                GuiPushBut(Gui, FillRc, PushBut_BackgroundActive);
            }
            else if(Style == GuiSlider_Index){
                float HotRectCenterX = SlideRc.Min.x + SlideRcWidth * ValuePercentage;
                float HotRectDimX = SlideRcHeight;
                
                rc2 HotRect = RcMinDim(V2(HotRectCenterX - HotRectDimX * 0.5f, SlideRc.Min.y), 
                                       V2(HotRectDimX, HotRectDimX));
                
                PushOrLoadGlyph(Gui->Assets,
                                Gui->Stack,
                                HotRect.Min,
                                GetRectDim(HotRect),
                                Gui->ChamomileID,
                                V4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            
            
            stbsp_sprintf(Buf, "%d", *Value);
        }
        
        v4 textC = GUI_GETCOLOR(GuiColor_HeaderInactive);
        PrintTextCenteredInRect(Gui, Buf, SlideRc, 1.0f, textC);
        
        //GuiPushBut(Gui, SlideRc, PushBut_Outline, GUI_GETCOLOR(GuiColor_WindowBorderActive));
        
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, SlideRc, Gui->FontScale);
        v2 NameStart = V2(SlideRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        
        rc2 NameRc = PrintText(Gui, Elem->NameToShow, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, SlideRc);
        GuiPostAdvance(Gui, Layout, AdvanceRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

void GuiSliderFloat(gui_state* Gui, float* Value, float Min, float Max, char* Name, u32 Style){
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        rc2 SlideRc = GetTextRect(Gui, 
                                  "                ", 
                                  Layout->At);
        SlideRc = GetTxtElemRect(Gui, Layout, SlideRc);
        GuiPushBut(Gui, SlideRc, PushBut_BackgroundInactive);
        
        float SlideRcWidth = GetRectWidth(SlideRc);
        float SlideRcHeight = GetRectHeight(SlideRc);
        
        char Buf[32];
        
        gui_interaction Interaction = CreateInteraction(Elem, 
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
        
        if(GuiIsActive(Gui, &Interaction) && (Style != GuiSlider_ProgressNonModify)){
            float MousePercentage = Clamp01((Gui->Input->MouseP.x - SlideRc.Min.x) / SlideRcWidth);
            *Value = Min + (Max - Min) * MousePercentage;
            
            
            if(KeyWentUp(Gui->Input, MouseKey_Left)){
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        
        if(Value){
            float ValuePercentage = (*Value - Min) / (Max - Min);
            
            if((Style == GuiSlider_Progress) || (Style == GuiSlider_ProgressNonModify)){
                rc2 FillRc;
                FillRc.Min = SlideRc.Min;
                FillRc.Max = V2(SlideRc.Min.x + SlideRcWidth * ValuePercentage, SlideRc.Max.y);
                
                GuiPushBut(Gui, FillRc, PushBut_BackgroundActive);
            }
            else if(Style == GuiSlider_Index){
                float HotRectCenterX = SlideRc.Min.x + SlideRcWidth * ValuePercentage;
                float HotRectDimX = SlideRcHeight;
                
                rc2 HotRect = RcMinDim(V2(HotRectCenterX - HotRectDimX * 0.5f, SlideRc.Min.y), 
                                       V2(HotRectDimX, HotRectDimX));
                
                PushOrLoadGlyph(Gui->Assets,
                                Gui->Stack,
                                HotRect.Min,
                                GetRectDim(HotRect),
                                Gui->ChamomileID,
                                V4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            
            stbsp_sprintf(Buf, "%.3f", *Value);
        }
        else{
            stbsp_sprintf(Buf, "ERR");
        }
        v4 textC = GUI_GETCOLOR(GuiColor_HeaderInactive);
        PrintTextCenteredInRect(Gui, Buf, SlideRc, 1.0f, textC);
        
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, SlideRc, Gui->FontScale);
        v2 NameStart = V2(SlideRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        
        rc2 NameRc = PrintText(Gui, Elem->NameToShow, NameStart, GUI_GETCOLOR(GuiColor_Text));
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, SlideRc);
        GuiPostAdvance(Gui, Layout, AdvanceRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
    
}

void GuiProgress01(gui_state* Gui,
                   char* Name,
                   float Value)
{
    GuiSliderFloat(Gui, &Value, 0.0f, 1.0f, Name, GuiSlider_ProgressNonModify);
}

void GuiInputText(gui_state* Gui, char* Name, char* Buf, int BufSize){
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleSmall(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        // NOTE(Dima): Printing button and text
        rc2 TextRc = GetTextRect(Gui, 
                                 "                            ", 
                                 Layout->At);
        TextRc = GetTxtElemRect(Gui, Layout, TextRc);
        GuiPushBut(Gui, TextRc, PushBut_BackgroundInactive);
        
        int* CaretP = &Elem->Data.InputText.CaretPos;
        
        gui_interaction Interaction = CreateInteraction(Elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInInteractiveArea(Gui, TextRc)){
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
            GuiPushBut(Gui, TextRc, PushBut_Outline, oulineColor);
        }
        
        *CaretP = Max(0, *CaretP);
        *CaretP = Min(*CaretP, Min(StringLength(Buf), BufSize - 1));
        
        v4 textC = V4(1.0f, 1.0f, 1.0f, 1.0f);
        //PrintTextCenteredInRect(Gui, Buf, TextRc, 1.0f, textC);
        v2 BufTextSize = GetTextSize(Gui, Buf, 1.0f);
        float PrintPX;
        float FieldDimX = GetRectDim(TextRc).x;
        if(BufTextSize.x < FieldDimX){
            PrintPX = TextRc.Min.x;
        }
        else{
            PrintPX = TextRc.Max.x - BufTextSize.x;
        }
        float PrintPY = GetCenteredTextOffsetY(Gui->MainFont,
                                               TextRc, 
                                               Gui->FontScale);
        v2 PrintP = V2(PrintPX, PrintPY);
        v2 CaretPrintP = GetCaretPrintP(Gui, Buf, PrintP, *CaretP);
        
        BeginGuiChunk(Gui->Stack, TextRc);
        v4 CaretColor = V4(0.0f, 1.0f, 0.0f, 1.0f);
        PrintCaret(Gui, 
                   CaretPrintP,
                   CaretColor); 
        PrintText(Gui, Buf, PrintP, GUI_GETCOLOR(GuiColor_HeaderInactive));
        EndGuiChunk(Gui->Stack);
        
        float NameStartY = GetCenteredTextOffsetY(Gui->MainFont, TextRc, Gui->FontScale);
        v2 NameStart = V2(TextRc.Max.x + GetScaledAscender(Gui->MainFont, Gui->FontScale) * 0.5f, NameStartY);
        char DebugBuf[256];
        
#if 1
        stbsp_sprintf(DebugBuf, "CaretP: %d; LeftRC: %d", *CaretP, 
                      Gui->Input->Keyboard.KeyStates[Key_Left].RepeatCount);
        rc2 NameRc = PrintText(Gui, DebugBuf, NameStart, GUI_GETCOLOR(GuiColor_Text));
#else
        rc2 NameRc = PrintText(Gui, Elem->NameToShow, NameStart, GUI_GETCOLOR(GuiColor_Text));
#endif
        
        
        rc2 AdvanceRect = GetBoundingRect(NameRc, TextRc);
        
        GuiPostAdvance(Gui, Layout, AdvanceRect);
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
        float StartX = Item->InternalRect.Min.x + ParentDim.x * ThisStartPercentage;
        float StartY = Item->InternalRect.Min.y;
        
        v2 Dim = V2(ParentDim.x * ThisAreaPercentage, ParentDim.y);
        Result = RcMinDim(V2(StartX, StartY), Dim);
    }
    else if((Item->Type == GuiGridItem_Column) ||
            (Item->Type == GuiGridItem_Item) ||
            (Item->Type == GuiGridItem_Grid))
    {
        float StartX = Item->InternalRect.Min.x;
        float StartY = Item->InternalRect.Min.y + ParentDim.y * ThisStartPercentage;
        
        v2 Dim = V2(ParentDim.x, ParentDim.y * ThisAreaPercentage);
        Result = RcMinDim(V2(StartX, StartY), Dim);
    }
    else{
        // TODO(Dima): ?????
    }
    
    return(Result);
}

INTERNAL_FUNCTION gui_grid_item* GuiGridItemInit(gui_state* Gui, 
                                                 gui_element* Elem, 
                                                 u32 GridItemType, 
                                                 float Weight)
{
    gui_grid_item* Item = &Elem->Data.GridItem;
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
            
            gui_element* AtGrid = Parent->ChildSentinel->Next;
            
            gui_grid_item* HubItem = &Parent->Data.GridItem;
            
            // NOTE(Dima): If first element in grid hub -> set it's to active
            if(AtGrid->Prev == AtGrid->Next){
                HubItem->ActiveID = StringHashFNV(Elem->Name);
            }
            
            while(AtGrid != Parent->ChildSentinel){
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
            Item->InternalRect = GuiGetGridRect(Weight, Elem->Parent);
            Item->Rect = GrowRectByPixels(Item->InternalRect, -10);
            
            gui_grid_item* ParentItem = &Elem->Parent->Data.GridItem;
            
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
    stbsp_sprintf(name, "GridHub: %d", Gui->CurElement->ChildCount);
    
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
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          Elem,
                                          GuiGridItem_Grid,
                                          0.0f);
}

void GuiGridEnd(gui_state* Gui){
    gui_grid_item* Item = &Gui->CurElement->Data.GridItem;
    Item->LastSumWeightInChildren = Item->SumWeightInChildren;
    Item->SumWeightInChildren = 0;
    GuiEndElement(Gui, GuiElement_GridItem);
}

void GuiGridBeginRow(gui_state* Gui, float Weight = 1.0f){
    char Name[64];
    stbsp_sprintf(Name, "Row: %d", Gui->CurElement->TmpCount);
    
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          Elem,
                                          GuiGridItem_Row,
                                          Weight);
}

void GuiGridBeginColumn(gui_state* Gui, float Weight = 1.0f){
    char Name[64];
    stbsp_sprintf(Name, "Column: %d", Gui->CurElement->TmpCount);
    
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          Elem,
                                          GuiGridItem_Column,
                                          Weight);
}

void GuiGridEndRowOrColumn(gui_state* Gui){
    
    gui_grid_item* Item = &Gui->CurElement->Data.GridItem;
    Item->LastSumWeightInChildren = Item->SumWeightInChildren;
    Item->SumWeightInChildren = 0;
    
    GuiEndElement(Gui, GuiElement_GridItem);
}

void GuiGridTileEmpty(gui_state* Gui, float Weight = 1.0f){
    b32 Result = 0;
    
    char Name[64];
    stbsp_sprintf(Name, "EmptyTile: %d", Gui->CurElement->TmpCount);
    
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          Elem,
                                          GuiGridItem_Item,
                                          Weight);
    
    GuiEndElement(Gui, GuiElement_GridItem);
}

b32 GuiGridTile(gui_state* Gui, char* Name, float Weight = 1.0f){
    b32 Result = 0;
    
    gui_element* Elem = GuiBeginElement(Gui, Name, GuiElement_GridItem, true);
    gui_grid_item* Item = GuiGridItemInit(Gui,
                                          Elem,
                                          GuiGridItem_Item,
                                          Weight);
    
    if(GuiElementOpenedInTree(Elem)){
        v4 TileColor = GUI_GETCOLOR_COLSYS(ColorExt_gray53);
        
        rc2 WorkRect = Item->Rect;
        
        v4 NotActiveColor1 = GUI_GETCOLOR_COLSYS(ColorExt_gray53);
        v4 NotActiveColor2 = GUI_GETCOLOR_COLSYS(ColorExt_gray60);
        
        v4 ActiveColor1 = GUI_GETCOLOR_COLSYS(ColorExt_BlueViolet);
        v4 ActiveColor2 = GUI_GETCOLOR_COLSYS(ColorExt_VioletRed);
        
        v4 Color1;
        v4 Color2;
        
        gui_interaction Interaction = CreateInteraction(Elem, 
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
                                        Elem->NameToShow, 
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
    GuiText(Gui, FPSBuf);
    
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
    
    GuiLinkButton(Gui, "LinkButton1");
    GuiLinkButton(Gui, "LinkButton2");
    
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
    GuiText(Gui, "I love Kate");
    GuiText(Gui, "I wish joy and happiness for everyone");
    char GuiTmpText[64];
    stbsp_sprintf(GuiTmpText, 
                  "Total GUI allocated elements %d", 
                  Gui->TotalAllocatedGuiElements);
    GuiText(Gui, GuiTmpText);
    
    GuiEndTree(Gui);
    
    static v2 WindowP = V2(900.0f, 100.0f);
    static v2 WindowDim = V2(300.0f, 600.0f);
    
    static char InputTextBuf[256];
    GuiInputText(Gui, "Input Text", InputTextBuf, 256);
    static float TestFloat4Slider;
    static int TestInt4Slider;
    GuiSliderFloat(Gui, &Gui->FontScale, 0.5f, 1.5f, "Gui font scale", GuiSlider_Index);
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
}