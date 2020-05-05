DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Animation)){
    if(Data){
        anim_system* AnimSys = (anim_system*)Data;
        
        GuiBeginTree(Gui, "Controllers");
        
        anim_controller* ControlAt = AnimSys->ControlUse.Next;
        if(ControlAt != &AnimSys->ControlUse){
            while(ControlAt != &AnimSys->ControlUse){
                anim_controller* Control = ControlAt;
                if(Control){
                    
                    // NOTE(Dima): Begin anim controller tree
                    GuiBeginTree(Gui, ControlAt->Name); 
                    
                    // NOTE(Dima): Printing states
                    GuiBeginTree(Gui, "States");
                    anim_state* StateAt = Control->FirstState;
                    while(StateAt != 0){
                        char StateBuf[128];
                        char StateTypeBuf[64];
                        
                        CopyStrings(StateTypeBuf, "Error");
                        if(StateAt->Type == AnimState_Animation){
                            CopyStrings(StateTypeBuf, "Animation");
                        }
                        else if(StateAt->Type == AnimState_BlendTree){
                            CopyStrings(StateTypeBuf, "BlendTree");
                        }
                        
                        stbsp_sprintf(StateBuf, "%s###: %.0f%%, %s", 
                                      StateAt->Name, StateAt->Contribution * 100.0f,
                                      StateTypeBuf);
                        
                        
                        // NOTE(Dima): Begin state tree 
                        GuiBeginTree(Gui, StateBuf); 
                        {
                            GuiBeginTree(Gui, "Transitions");
                            anim_transition* TranAt = StateAt->FirstTransition;
                            while(TranAt){
                                
                                char* FromName = 0;
                                char* ToName = 0;
                                char TransitionBuf[256];
                                
                                if(TranAt->FromState){
                                    FromName = TranAt->FromState->Name;
                                }
                                
                                if(TranAt->ToState){
                                    ToName = TranAt->ToState->Name;
                                }
                                
                                stbsp_sprintf(TransitionBuf, "%s -> %s", 
                                              FromName, ToName);
                                GuiText(Gui, TransitionBuf);
                                
                                TranAt = TranAt->NextInList;
                            }
                            GuiEndTree(Gui);
                        }
                        GuiEndTree(Gui); // NOTE(Dima): End state tree
                        
                        StateAt = StateAt->NextInList;
                    }
                    GuiEndTree(Gui);
                    
                    // NOTE(Dima): Printing variables
                    GuiBeginTree(Gui, "Variables");
                    anim_variable* VarAt = Control->FirstVariable;
                    while(VarAt != 0){
                        if(VarAt->ValueType == AnimVariable_Float){
                            GuiShowFloat(Gui, VarAt->Name, VarAt->Value.Float);
                        }
                        else if(VarAt->ValueType == AnimVariable_Bool){
                            GuiShowBool(Gui, VarAt->Name, VarAt->Value.Bool);
                        }
                        
                        VarAt = VarAt->NextInList;
                    }
                    GuiEndTree(Gui);
                    
                    GuiShowInt(Gui, "Playing states count", Control->PlayingStatesCount);
                    GuiShowBool(Gui, "In transition", Control->PlayingStatesCount == 2);
                    
                    for(int PlayingStateIndex = 0;
                        PlayingStateIndex < Control->PlayingStatesCount;
                        PlayingStateIndex++)
                    {
                        char BufToShow[64];
                        stbsp_sprintf(BufToShow, "%d state name: %s", PlayingStateIndex,
                                      Control->PlayingStates[PlayingStateIndex]->Name);
                        GuiText(Gui, BufToShow);
                        
                        GuiProgress01(Gui, "Anim phase", 
                                      Control->PlayingStates[PlayingStateIndex]->PlayingAnimation.Phase01);
                    }
                    
                    
                    
                    GuiEndTree(Gui); // End anim controller tree
                    
                    ControlAt = ControlAt->Next;
                }
                else{
                    GuiText(Gui, "Error");
                    break;
                }
            }
        }
        else{
            GuiText(Gui, "None");
        }
        
        GuiEndTree(Gui);
        
        
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Input)){
    if(Data){
        input_state* Input = (input_state*)Data;
        
        char GlobalTimeText[64];
        stbsp_sprintf(GlobalTimeText, "Global time: %.2f sec", Input->Time);
        GuiText(Gui, GlobalTimeText);
        
        char DeltaTimeText[64];
        stbsp_sprintf(DeltaTimeText, "Last frame time: %.3f sec", Input->DeltaTime);
        GuiText(Gui, DeltaTimeText);
        
        char MousePText[64];
        stbsp_sprintf(MousePText, 
                      "MouseP: x(%.2f); y(%.2f)", 
                      Input->MouseP.x, 
                      Input->MouseP.y);
        GuiText(Gui, MousePText);
        
        char DeltaMousePText[64];
        stbsp_sprintf(DeltaMousePText, 
                      "DeltaMouseP: x(%.2f) y(%.2f)", 
                      Input->MouseDeltaP.x, 
                      Input->MouseDeltaP.y);
        GuiText(Gui, DeltaMousePText);
        
        
        GuiBoolButtonOnOff(Gui, "Capturing mouse", &Input->CapturingMouse);
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}

INTERNAL_FUNCTION void ShowTopClocks(gui_state* Gui, 
                                     debug_state* State,
                                     b32 IncludingChildren)
{
    render_state* Render = State->Render;
    render_stack* Stack = Gui->Stack;
    
#if 0    
    gui_element* Elem = GuiBeginElement(Gui, "TopClocks", GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleBig(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        
        
        GuiPostAdvance(Gui, Layout, SliderRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
#endif
    
    debug_profiled_frame* Frame = GetFrameByIndex(State, 
                                                  State->ViewFrameIndex);
    
    if(Frame->FrameUpdateNode){
        FillAndSortStats(State, Frame, IncludingChildren);
        
        for(int StatIndex = 0; 
            StatIndex < Frame->ToSortStatsCount; 
            StatIndex++)
        {
            debug_timing_stat* Stat = Frame->ToSortStats[StatIndex];
            
            u64 ToShowClocks = GetClocksFromStat(Stat, IncludingChildren);
            
            f32 CoveragePercentage = 100.0f * (f32)ToShowClocks / (f32)Frame->FrameUpdateNode->TimingSnapshot.ClocksElapsed;
            
            char StatName[256];
            DEBUGParseNameFromUnique(StatName, 256, Stat->UniqueName);
            
            char StatBuf[256];
            stbsp_sprintf(StatBuf, "%11lluc %8.2f%% %8u  %-30s",
                          ToShowClocks,
                          CoveragePercentage,
                          Stat->Stat.HitCount,
                          StatName);
            
            GuiText(Gui, StatBuf);
        }
    }
}

INTERNAL_FUNCTION inline rc2 GetBarRectHorz(rc2 TotalRect, int BarsCount, int BarIndex){
    float WidthPerOneBar = GetRectWidth(TotalRect) / (float)BarsCount;
    
    float TargetX = TotalRect.Min.x + (float)BarIndex * WidthPerOneBar;
    
    rc2 Result = RcMinDim(V2(TargetX, TotalRect.Min.y), 
                          V2(WidthPerOneBar, GetRectHeight(TotalRect)));
    
    return(Result);
}

INTERNAL_FUNCTION void PrintWebForBaredGraph(gui_state* Gui,
                                             debug_state* State,
                                             int BarsCount,
                                             rc2 TotalRect,
                                             v4 Color)
{
    render_state* Render = State->Render;
    
    float WidthPerOneBar = GetRectDim(TotalRect).x / (float)BarsCount;
    
    // NOTE(Dima): Printing black web
    PushGuiLineRect(Render, TotalRect, Color);
    for(int Index = 1; 
        Index < BarsCount; 
        Index++)
    {
        float Target = TotalRect.Min.x + (float)Index * WidthPerOneBar;
        
        PushGuiLine(Render, 
                    V2(Target, TotalRect.Min.y),
                    V2(Target, TotalRect.Max.y),
                    Color);
    }
    
}

INTERNAL_FUNCTION void ShowFramesSlider(gui_state* Gui, debug_state* State){
    
    render_state* Render = State->Render;
    render_stack* Stack = Gui->Stack;
    
    gui_element* Elem = GuiBeginElement(Gui, "FramesSlider", GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleBig(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        v2 DimLeft = GetDimensionLeftInLayout(Layout);
        
        float ScaledAsc = GetScaledAscender(Gui->MainFont, Gui->FontScale);
        v2 At = Layout->At - V2(0.0f, ScaledAsc);
        rc2 SliderRect = RcMinDim(At, V2(DimLeft.x, ScaledAsc * 3.0f));
        
        float WidthPerOneBar = DimLeft.x / (float)DEBUG_PROFILED_FRAMES_COUNT;
        
        // NOTE(Dima): Printing collation frame bar
        PushRect(Stack, 
                 GetBarRectHorz(SliderRect, DEBUG_PROFILED_FRAMES_COUNT, 
                                State->CollationFrameIndex),
                 V4(0.0f, 1.0f, 0.0f, 1.0f));
        
        
        // NOTE(Dima): Printing viewing frame bar
        PushRect(Stack, 
                 GetBarRectHorz(SliderRect, DEBUG_PROFILED_FRAMES_COUNT, 
                                State->ViewFrameIndex),
                 V4(1.0f, 1.0f, 0.0f, 1.0f));
        
        // NOTE(Dima): Printing black web
        PrintWebForBaredGraph(Gui, State, 
                              DEBUG_PROFILED_FRAMES_COUNT,
                              SliderRect, V4(0.0f, 0.0f, 0.0f, 1.0f));
        
        GuiPostAdvance(Gui, Layout, SliderRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

INTERNAL_FUNCTION void ShowRootViewer(gui_state* Gui, debug_state* State){
    
    render_state* Render = State->Render;
    render_stack* Stack = Gui->Stack;
    
    gui_element* Elem = GuiBeginElement(Gui, "RootViewer", GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleBig(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        v2 DimLeft = GetDimensionLeftInLayout(Layout);
        
        float ScaledAsc = GetScaledAscender(Gui->MainFont, Gui->FontScale);
        v2 At = Layout->At - V2(0.0f, ScaledAsc);
        rc2 WorkRect = RcMinDim(At, V2(DimLeft.x, ScaledAsc * 10.0f));
        float WidthPerOneBar = DimLeft.x / (float)DEBUG_PROFILED_FRAMES_COUNT;
        
        for(int FrameIndex = 0; 
            FrameIndex < DEBUG_PROFILED_FRAMES_COUNT;
            FrameIndex++)
        {
            debug_profiled_frame* Frame = GetFrameByIndex(State, 
                                                          State->ViewFrameIndex);
            
            if(Frame->FrameUpdateNode){
                rc2 BarRc = GetBarRectHorz(WorkRect, 
                                           DEBUG_PROFILED_FRAMES_COUNT, 
                                           FrameIndex);
                
                v2 BarDim = GetRectDim(BarRc);
                
                u64 TotalClocks = Frame->FrameUpdateNode->TimingSnapshot.ClocksElapsed;
                
                debug_profiled_tree_node* At = Frame->FrameUpdateNode->ChildSentinel->Next;
                float CurrentStackedValue = 0.0f;
                while(At != Frame->FrameUpdateNode->ChildSentinel){
                    
                    u64 ThisClocks = At->TimingSnapshot.ClocksElapsed;
                    
                    f32 ThisPercentage = (f32)ThisClocks / (f32)TotalClocks;
                    f32 ThisHeight = BarDim.y * ThisPercentage;
                    
                    rc2 ThisRc = RcMinDim(V2(BarRc.Min.x, BarRc.Max.y - CurrentStackedValue - ThisHeight), 
                                          V2(BarDim.x, ThisHeight));
                    
                    if(MouseInRect(Gui->Input, ThisRc)){
                        char ParsedName[256];
                        DEBUGParseNameFromUnique(ParsedName, 256, At->UniqueName);
                        
                        char TooltipText[256];
                        stbsp_sprintf(TooltipText, "%s", ParsedName);
                        
                        GuiTooltip(Gui, TooltipText, Gui->Input->MouseP);
                    }
                    
                    CurrentStackedValue += ThisHeight;
                    
                    // NOTE(Dima): Printing rect
                    int ThisColorIndex = GuiColor_Graph0 + At->NameID % (GuiColor_GraphCount - GuiColor_Graph0);
                    PushRect(Stack, ThisRc, GUI_GETCOLOR_COLSYS(ThisColorIndex));
                    
                    At = At->Next;
                }
            }
        }
        
        // NOTE(Dima): Printing black web
        PrintWebForBaredGraph(Gui, State, 
                              DEBUG_PROFILED_FRAMES_COUNT,
                              WorkRect, V4(0.0f, 0.0f, 0.0f, 1.0f));
        
        GuiPostAdvance(Gui, Layout, WorkRect);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Profile)){
    if(Data){
        debug_state* State = (debug_state*)Data;
        
        if(GuiBoolButtonOnOff(Gui, "Recording", &State->TargetRecordingValue)){
            State->RecordingChangeRequested = true;
        }
        
        ShowFramesSlider(Gui, State);
        
        GuiBeginRow(Gui);
        GuiBeginRadioGroup(Gui, "MenuRadioGroup", 
                           &State->ToShowProfileMenuType, 
                           State->ToShowProfileMenuType);
        BeginDimension(Gui, BeginDimension_Both, ScaledAscDim(Gui, V2(7, 2)));
        GuiRadioButton(Gui, "ClocksEx", DebugProfileMenu_TopClockEx);
        GuiRadioButton(Gui, "Clocks", DebugProfileMenu_TopClock);
        GuiRadioButton(Gui, "Root", DebugProfileMenu_RootNode);
#if 0        
        GuiRadioButton();
        GuiRadioButton();
#endif
        EndDimension(Gui);
        GuiEndRadioGroup(Gui);
        GuiEndRow(Gui);
        
        switch(State->ToShowProfileMenuType){
            case DebugProfileMenu_TopClock:{
                ShowTopClocks(Gui, State, true);
            }break;
            
            case DebugProfileMenu_TopClockEx:{
                ShowTopClocks(Gui, State, false);
            }break;
            
            case DebugProfileMenu_RootNode:{
                ShowRootViewer(Gui, State);
            }break;
        }
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Assets)){
    assets* Assets = Gui->Assets;
    
    if(Assets){
        char CurBlockBuf[256];
        stbsp_sprintf(CurBlockBuf, 
                      "Current asset block index: %d", 
                      Assets->CurrentBlockIndex);
        GuiText(Gui, CurBlockBuf);
        
        GuiBeginTree(Gui, "Asset file sources");
        asset_file_source* At = Assets->FileSourceUse.Next;
        
        if(At != &Assets->FileSourceUse){
            while(At != &Assets->FileSourceUse){
                
                char FileSourceBuf[256];
                stbsp_sprintf(FileSourceBuf,
                              "Name: %s; Integration base ID: %u",
                              At->FileDescription.FullPath,
                              At->IntegrationBaseID);
                
                GuiText(Gui, FileSourceBuf);
                
                At = At->Next;
            }
        }
        else{
            GuiText(Gui, "None");
        }
        GuiEndTree(Gui);
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Game)){
    if(Data){
        
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Console)){
    if(Data){
        
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_GUI)){
    if(Data){
        float DeltaTime = Gui->FrameInfo.DeltaTime;
        
        char FPSBuf[64];
        stbsp_sprintf(FPSBuf, "FPS %.2f, delta time(sec) %.3f", 
                      1.0f / DeltaTime, DeltaTime);
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
        
        if(GuiBoolButton(Gui, "Show GuiTest", &Gui->ShowGuiTest)){
            GuiTest(Gui, Gui->FrameInfo.DeltaTime);
        }
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_DEBUG)){
    if(Data){
        debug_state* DEBUG = (debug_state*)Data;
        
        
        
        GuiBoolButton(Gui, "Show DEBUG overlays (F3 to toggle)",
                      &DEBUG->ShowDebugOverlay);
        GuiBoolButton(Gui, "Show DEBUG menus (F4 to toggle)", 
                      &DEBUG->ShowDebugMenus);
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Log)){
    if(Data){
        
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}


DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Platform)){
    if(Data){
        
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}


DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Render)){
    if(Data){
        
    }
    else{
        GuiText(Gui, "ERROR. Data was NULL");
    }
}