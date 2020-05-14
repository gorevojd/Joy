DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Animation)){
    if(Data){
        anim_system* AnimSys = (anim_system*)Data;
        
        BeginDimension(Gui, BeginDimension_Height, ScaledAscDim(Gui, V2(0, 2)));
        
        BeginTree(Gui, "Controllers");
        
        anim_controller* ControlAt = AnimSys->ControlUse.Next;
        if(ControlAt != &AnimSys->ControlUse){
            while(ControlAt != &AnimSys->ControlUse){
                anim_controller* Control = ControlAt;
                if(Control){
                    
                    // NOTE(Dima): Begin anim controller tree
                    BeginTree(Gui, ControlAt->Name); 
                    
                    // NOTE(Dima): Printing states
                    BeginTree(Gui, "States");
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
                        BeginTree(Gui, StateBuf); 
                        {
                            BeginTree(Gui, "Transitions");
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
                                ShowText(Gui, TransitionBuf);
                                
                                TranAt = TranAt->NextInList;
                            }
                            EndTree(Gui);
                        }
                        EndTree(Gui); // NOTE(Dima): End state tree
                        
                        StateAt = StateAt->NextInList;
                    }
                    EndTree(Gui);
                    
                    // NOTE(Dima): Printing variables
                    BeginTree(Gui, "Variables");
                    anim_variable* VarAt = Control->FirstVariable;
                    while(VarAt != 0){
                        if(VarAt->ValueType == AnimVariable_Float){
                            ShowFloat(Gui, VarAt->Name, VarAt->Value.Float);
                        }
                        else if(VarAt->ValueType == AnimVariable_Bool){
                            ShowBool(Gui, VarAt->Name, VarAt->Value.Bool);
                        }
                        
                        VarAt = VarAt->NextInList;
                    }
                    EndTree(Gui);
                    
                    ShowInt(Gui, "Playing states count", Control->PlayingStatesCount);
                    ShowBool(Gui, "In transition", Control->PlayingStatesCount == 2);
                    
                    for(int PlayingStateIndex = 0;
                        PlayingStateIndex < Control->PlayingStatesCount;
                        PlayingStateIndex++)
                    {
                        char BufToShow[64];
                        stbsp_sprintf(BufToShow, "%d state name: %s", PlayingStateIndex,
                                      Control->PlayingStates[PlayingStateIndex]->Name);
                        ShowText(Gui, BufToShow);
                        
                        ProgressSlider01(Gui, "Anim phase", 
                                         Control->PlayingStates[PlayingStateIndex]->PlayingAnimation.Phase01);
                    }
                    
                    
                    
                    EndTree(Gui); // End anim controller tree
                    
                    ControlAt = ControlAt->Next;
                }
                else{
                    ShowText(Gui, "Error");
                    break;
                }
            }
        }
        else{
            ShowText(Gui, "None");
        }
        
        EndTree(Gui);
        
        EndDimension(Gui);
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Input)){
    if(Data){
        input_state* Input = (input_state*)Data;
        
        char GlobalTimeText[64];
        stbsp_sprintf(GlobalTimeText, "Global time: %.2f sec", Input->Time);
        ShowText(Gui, GlobalTimeText);
        
        char DeltaTimeText[64];
        stbsp_sprintf(DeltaTimeText, "Last frame time: %.3f sec", Input->DeltaTime);
        ShowText(Gui, DeltaTimeText);
        
        char MousePText[64];
        stbsp_sprintf(MousePText, 
                      "MouseP: x(%.2f); y(%.2f)", 
                      Input->MouseP.x, 
                      Input->MouseP.y);
        ShowText(Gui, MousePText);
        
        char DeltaMousePText[64];
        stbsp_sprintf(DeltaMousePText, 
                      "DeltaMouseP: x(%.2f) y(%.2f)", 
                      Input->MouseDeltaP.x, 
                      Input->MouseDeltaP.y);
        ShowText(Gui, DeltaMousePText);
        
        
        BoolButtonOnOff(Gui, "Capturing mouse", &Input->CapturingMouse);
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
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
    
    
    debug_thread_frame* MainFrame = GetThreadFrameByIndex(State->MainThread,
                                                          State->ViewFrameIndex);
    
    debug_thread_frame* Frame = GetThreadFrameByIndex(State->WatchThread, 
                                                      State->ViewFrameIndex);
    debug_common_frame* FrameCommon = GetFrameByIndex(State, State->ViewFrameIndex);
    
    if(MainFrame->FrameUpdateNode){
        FillAndSortStats(State, Frame, IncludingChildren);
        
        for(int StatIndex = 0; 
            StatIndex < Frame->ToSortStatsCount; 
            StatIndex++)
        {
            debug_timing_stat* Stat = Frame->ToSortStats[StatIndex];
            
            u64 ToShowClocks = GetClocksFromStat(Stat, IncludingChildren);
            
            f32 CoveragePercentage = 100.0f * (f32)ToShowClocks / (f32)MainFrame->FrameUpdateNode->TimingSnapshot.ClocksElapsed;
            
            f32 CloseInFrameTime = CoveragePercentage * FrameCommon->FrameTime * 0.01f;
            
            char StatName[256];
            DEBUGParseNameFromUnique(StatName, 256, Stat->UniqueName);
            
            char StatBuf[256];
            stbsp_sprintf(StatBuf, "%11lluc %8.2f%% %5.2fms %8uh  %-30s",
                          ToShowClocks,
                          CoveragePercentage,
                          CloseInFrameTime * 1000.0f,
                          Stat->Stat.HitCount,
                          StatName);
            
            ShowText(Gui, StatBuf);
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
        rc2 SliderRectInit = RcMinDim(At, V2(DimLeft.x, ScaledAsc * 4.0f));
        rc2 SliderRect = GrowRectByPixels(SliderRectInit, -5);
        
        gui_interaction Interaction = CreateInteraction(Elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        if(MouseInInteractiveArea(Gui, SliderRect)){
            GuiSetHot(Gui, &Interaction, true);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left)){
                GuiSetActive(Gui, &Interaction);
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        if(GuiIsActive(Gui, &Interaction)){
            if(KeyWentUp(Gui->Input, MouseKey_Left)){
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        
        for(int BarIndex = 0;
            BarIndex < DEBUG_PROFILED_FRAMES_COUNT;
            BarIndex++)
        {
            rc2 BarRc = GetBarRectHorz(SliderRect, DEBUG_PROFILED_FRAMES_COUNT,
                                       BarIndex);
            
            if(MouseInInteractiveArea(Gui, BarRc)){
                if(KeyIsDown(Gui->Input, MouseKey_Left) && 
                   (BarIndex != State->CollationFrameIndex))
                {
                    State->ViewFrameIndex = BarIndex;
                }
            }
        }
        
        // NOTE(Dima): Printing collation frame bar
        PushRect(Stack, 
                 GetBarRectHorz(SliderRect, DEBUG_PROFILED_FRAMES_COUNT, 
                                State->CollationFrameIndex),
                 V4(0.0f, 1.0f, 0.0f, 1.0f));
        
        // NOTE(Dima): Printing newest frame bar
        PushRect(Stack, 
                 GetBarRectHorz(SliderRect, DEBUG_PROFILED_FRAMES_COUNT, 
                                State->NewestFrameIndex),
                 V4(1.0f, 0.0f, 0.0f, 1.0f));
        
        // NOTE(Dima): Printing oldest frame bar
        PushRect(Stack, 
                 GetBarRectHorz(SliderRect, DEBUG_PROFILED_FRAMES_COUNT, 
                                State->OldestFrameIndex),
                 V4(0.2f, 0.3f, 0.9f, 1.0f));
        
        
        
        // NOTE(Dima): Printing viewing frame bar
        PushRect(Stack, 
                 GetBarRectHorz(SliderRect, DEBUG_PROFILED_FRAMES_COUNT, 
                                State->ViewFrameIndex),
                 V4(1.0f, 1.0f, 0.0f, 1.0f));
        
        // NOTE(Dima): Printing black web
        PrintWebForBaredGraph(Gui, State, 
                              DEBUG_PROFILED_FRAMES_COUNT,
                              SliderRect, GUI_GETCOLOR(GuiColor_Borders));
        
        GuiPostAdvance(Gui, Layout, SliderRectInit);
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
        rc2 WorkRectInit = RcMinDim(At, V2(DimLeft.x, ScaledAsc * 10.0f));
        rc2 WorkRect = GrowRectByPixels(WorkRectInit, -5);
        
        for(int FrameIndex = 0; 
            FrameIndex < DEBUG_PROFILED_FRAMES_COUNT;
            FrameIndex++)
        {
            debug_thread_frame* Frame = GetThreadFrameByIndex(State->MainThread, 
                                                              FrameIndex);
            
            if(Frame->FrameUpdateNode){
                rc2 BarRc = GetBarRectHorz(WorkRect, 
                                           DEBUG_PROFILED_FRAMES_COUNT, 
                                           FrameIndex);
                
                v2 BarDim = GetRectDim(BarRc);
                
                u64 TotalClocks = Frame->FrameUpdateNode->TimingSnapshot.ClocksElapsed;
                
                debug_profiled_tree_node* At = Frame->FrameUpdateNode->ChildSentinel->Next;
                float CurrentStackedValue = 0.0f;
                int TempIndex = 0;
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
                        
                        ShowTooltip(Gui, TooltipText, Gui->Input->MouseP);
                    }
                    
                    CurrentStackedValue += ThisHeight;
                    
                    // NOTE(Dima): Printing rect
                    int ThisColorIndex = GuiColor_Graph0 + TempIndex % (GuiColor_GraphCount - GuiColor_Graph0);
                    PushRect(Stack, ThisRc, GUI_GETCOLOR(ThisColorIndex));
                    
                    
                    TempIndex++;
                    At = At->Next;
                }
            }
            
            if(FrameIndex == State->ViewFrameIndex){
                PushRect(Stack, GetBarRectHorz(WorkRect, 
                                               DEBUG_PROFILED_FRAMES_COUNT, 
                                               FrameIndex), 
                         V4(0.0f, 0.0f, 0.0f, 0.5f));
            }
            
            if(FrameIndex == State->CollationFrameIndex){
                PushRect(Stack, GetBarRectHorz(WorkRect, 
                                               DEBUG_PROFILED_FRAMES_COUNT, 
                                               FrameIndex), 
                         V4(0.0f, 1.0f, 0.0f, 1.0f));
            }
        }
        
        // NOTE(Dima): Printing black web
        PrintWebForBaredGraph(Gui, State, 
                              DEBUG_PROFILED_FRAMES_COUNT,
                              WorkRect, GUI_GETCOLOR(GuiColor_Borders));
        
        GuiPostAdvance(Gui, Layout, WorkRectInit);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

inline debug_profiled_tree_node* 
FindProfiledTreeNodeInThreadFrame(debug_thread_frame* Frame, char* UniqueName){
    u32 NameID = StringHashFNV(UniqueName);
    
    debug_profiled_tree_node* Result = 0;
    
    debug_profiled_tree_node* At = &Frame->RootTreeNodeUse;
    if(NameID == At->NameID){
        Result = At;
    }
    else{
        At = At->NextAlloc;
        
        while(At != &Frame->RootTreeNodeUse){
            
            if(NameID == At->NameID){
                Result = At;
                break;
            }
            
            At = At->NextAlloc;
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION void ResetThreadsWatchNodes(debug_state* State){
    debug_thread* ThreadAt = State->ThreadSentinel.NextAlloc;
    while(ThreadAt != &State->ThreadSentinel){
        
        ThreadAt->WatchNodeUniqueName = State->RootNodesName;
        
        ThreadAt = ThreadAt->NextAlloc;
    }
}

INTERNAL_FUNCTION inline rc2
GetLaneRect(rc2 TotalRect, int LaneIndex, int LaneCount){
    rc2 Result = {};
    
    
    if(LaneCount){
        v2 TotalRectDim = GetRectDim(TotalRect);
        f32 ThisLaneStart = TotalRect.Min.x + 
            TotalRectDim.x * (f32)(LaneIndex + 0) / (f32)LaneCount;
        
        f32 ThisLaneEnd = TotalRect.Min.x + 
            TotalRectDim.x * (f32)(LaneIndex + 1) / (f32)LaneCount;
        
        Result = RcMinMax(V2(ThisLaneStart, TotalRect.Min.y),
                          V2(ThisLaneEnd, TotalRect.Max.y));
    }
    
    return(Result);
}

INTERNAL_FUNCTION void ShowThreadsViewer(gui_state* Gui, debug_state* State){
    
    render_state* Render = State->Render;
    render_stack* Stack = Gui->Stack;
    
    gui_element* Elem = GuiBeginElement(Gui, "ThreadsViewer", GuiElement_Item, true);
    gui_layout* Layout = GetParentLayout(Gui);
    
    if(GuiElementOpenedInTree(Elem) && 
       PotentiallyVisibleBig(Layout))
    {
        GuiPreAdvance(Gui, Layout, Elem->Depth);
        
        v2 DimLeft = GetDimensionLeftInLayout(Layout);
        
        float ScaledAsc = GetScaledAscender(Gui->MainFont, Gui->FontScale);
        v2 At = Layout->At - V2(0.0f, ScaledAsc);
        //rc2 SliderRectInit = RcMinDim(At, ScaledAscDim(Gui, V2(20.0f, 30.0f)));
        rc2 SliderRectInit = RcMinDim(At, V2(DimLeft.x, ScaledAsc * 10.0f));
        rc2 SliderRect = GrowRectByPixels(SliderRectInit, -5);
        v2 SliderRectDim = GetRectDim(SliderRect);
        
        
        gui_interaction Interaction = CreateInteraction(Elem, 
                                                        GuiInteraction_Empty,
                                                        GuiPriority_Avg);
        
        if(MouseInRect(Gui->Input, SliderRect)){
            GuiSetHot(Gui, &Interaction, true);
            
            if(KeyWentDown(Gui->Input, MouseKey_Left) || 
               KeyWentDown(Gui->Input, MouseKey_Right))
            {
                GuiSetActive(Gui, &Interaction);
                GuiReleaseInteraction(Gui, &Interaction);
            }
        }
        else{
            GuiSetHot(Gui, &Interaction, false);
        }
        
        PushRect(Stack, SliderRect, V4(0.0f, 0.0f, 0.0f, 0.7f));
        
        int ViewFrame = State->ViewFrameIndex;
        
        int LaneCount = State->ProfiledThreadsCount;
        
        debug_thread* MainThread = State->MainThread;
        debug_thread_frame* ProfFrame = &State->MainThread->Frames[ViewFrame];
        b32 ShowWatchingOutline = false;
        int WatchThreadLaneAt = 0;
        
        if(ProfFrame->FrameUpdateNode){
            u64 FrameUpdateStart = ProfFrame->FrameUpdateNode->TimingSnapshot.StartClock;
            f32 OneOverFramesClock = 1.0f / (f32)ProfFrame->FrameUpdateNode->TimingSnapshot.ClocksElapsed;
            
            int ThreadLaneAt = 0;
            debug_thread* ThreadAt = State->ThreadSentinel.NextAlloc;
            while(ThreadAt != &State->ThreadSentinel){
                
                debug_thread_frame* Frame = &ThreadAt->Frames[ViewFrame];
                
                if(LaneCount){
                    ShowWatchingOutline = true;
                    
                    rc2 LaneRect = GetLaneRect(SliderRect, ThreadLaneAt, 
                                               State->ProfiledThreadsCount);
                    
                    f32 ThisLaneStart = LaneRect.Min.x;
                    f32 ThisLaneEnd = LaneRect.Max.x;
                    
                    if(ThreadAt->WatchNodeUniqueName == 0){
                        ThreadAt->WatchNodeUniqueName = ProfFrame->FrameUpdateNode->UniqueName;
                    }
                    
                    debug_profiled_tree_node* Node = FindProfiledTreeNodeInThreadFrame(Frame, ThreadAt->WatchNodeUniqueName);
                    
                    if(!Node){
                        Node = &Frame->RootTreeNodeUse;
                        ThreadAt->WatchNodeUniqueName = Node->UniqueName;
                    }
                    
                    if(Node){
                        debug_profiled_tree_node* At = Node->ChildSentinel->Next;
                        while(At != Node->ChildSentinel){
                            f32 ThisClocks = (f32)At->TimingSnapshot.ClocksElapsed;
                            
                            f32 ThisPercentage = ThisClocks * OneOverFramesClock;
                            f32 ThisStartPercentage = (At->TimingSnapshot.StartClockFirstEntry - FrameUpdateStart)  * OneOverFramesClock;
                            
                            if(ThisPercentage > 0.01f){
                                f32 ThisHeight = SliderRectDim.y * ThisPercentage;
                                f32 ThisStart = SliderRect.Max.y - (SliderRectDim.y * ThisStartPercentage);
                                
                                rc2 ThisRect = RcMinMax(V2(ThisLaneStart, ThisStart - ThisHeight), 
                                                        V2(ThisLaneEnd, ThisStart));
                                
                                u32 GuiColorID = GetGraphColorIndexFromHash(At->NameID);
                                v4 RectColor = GUI_GETCOLOR(GuiColorID);
                                PushRect(Stack, ThisRect, RectColor);
                                PushRectInnerOutline(Stack, ThisRect, 1, GUI_GETCOLOR(GuiColor_Borders));
                                
                                if(MouseInRect(Gui->Input, ThisRect)){
                                    b32 HasChildren = At->ChildSentinel->Next != At->ChildSentinel;
                                    
                                    ShowTooltip(Gui, At->UniqueName, Gui->Input->MouseP);
                                    
                                    if(KeyWentDown(Gui->Input, MouseKey_Left) && HasChildren){
                                        ThreadAt->WatchNodeUniqueName = At->UniqueName;
                                    }
                                }
                            }
                            
                            At = At->Next;
                        }
                    }
                    else{
                        PushRectInnerOutline(Stack, LaneRect, 3, GUI_GETCOLOR(GuiColor_Error));
                    }
                    
                    
                    if(MouseInRect(Gui->Input, LaneRect)){
                        if(KeyWentDown(Gui->Input, MouseKey_Right) && Node->Parent){
                            ThreadAt->WatchNodeUniqueName = Node->Parent->UniqueName;
                        }
                        
                        if(KeyWentDown(Gui->Input, MouseKey_Middle)){
                            State->WatchThread = ThreadAt;
                        }
                    }
                    
                    if(ThreadAt->ThreadID == State->WatchThread->ThreadID){
                        WatchThreadLaneAt = ThreadLaneAt;
                    }
                }
                
                ThreadLaneAt++;
                ThreadAt = ThreadAt->NextAlloc;
            }
        }
        else{
            PushRectInnerOutline(Stack, SliderRect, 3, GUI_GETCOLOR(GuiColor_Error));
            
            PrintTextCenteredInRect(Gui, "Can't find frame update node in frame",
                                    SliderRect, 0.7f, GUI_GETCOLOR(GuiColor_Error));
        }
        
        PushRectOutline(Stack, SliderRect, 2, GUI_GETCOLOR(GuiColor_Borders));
        
        if(ShowWatchingOutline){
            // NOTE(Dima): Pushing outline for watching thread
            rc2 WatchLaneRect = GetLaneRect(SliderRect, WatchThreadLaneAt,
                                            State->ProfiledThreadsCount);
            
            PushRectOutline(Stack, WatchLaneRect, 2, GUI_GETCOLOR(GuiColor_Active));
        }
        
        GuiPostAdvance(Gui, Layout, SliderRectInit);
    }
    
    GuiEndElement(Gui, GuiElement_Item);
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Profile)){
    if(Data){
        debug_state* State = (debug_state*)Data;
        
        BeginDimension(Gui, BeginDimension_Both, ScaledAscDim(Gui, V2(6, 2)));
        BeginRow(Gui);
        if(BoolButton(Gui, "REC", &State->IsRecording)){
            State->RecordingChangeRequested = true;
        }
        if(Button(Gui, "Newest")){
            State->ViewFrameIndex = State->NewestFrameIndex;
        }
        if(Button(Gui, "Oldest")){
            State->ViewFrameIndex = State->OldestFrameIndex;
        }
        EndRow(Gui);
        EndDimension(Gui);
        
        ShowFramesSlider(Gui, State);
        
        BeginDimension(Gui, BeginDimension_Both, ScaledAscDim(Gui, V2(6, 2)));
        BeginRow(Gui);
        if(Button(Gui, "Reset")){
            ResetThreadsWatchNodes(State);
        }
        if(Button(Gui, "To Main")){
            State->WatchThread = State->MainThread;
        }
        if(Button(Gui, "Next")){
            if(State->WatchThread->NextAlloc == &State->ThreadSentinel){
                State->WatchThread = State->ThreadSentinel.NextAlloc;
            }
            else{
                State->WatchThread = State->WatchThread->NextAlloc;
            }
        }
        EndRow(Gui);
        EndDimension(Gui);
        
        ShowThreadsViewer(Gui, State);
        
        debug_common_frame* Frame = &State->Frames[State->ViewFrameIndex];
        
        // NOTE(Dima): Viewing frame info
        char FrameInfo[256];
        stbsp_sprintf(FrameInfo,
                      "Viewing frame: %.2fms, %.0fFPS, index=%d",
                      Frame->FrameTime * 1000.0f, 
                      1.0f / Frame->FrameTime, 
                      State->ViewFrameIndex);
        ShowText(Gui, FrameInfo);
        
        BeginRow(Gui);
        BeginRadioGroup(Gui, "MenuRadioGroup", 
                        &State->ToShowProfileMenuType, 
                        State->ToShowProfileMenuType);
        BeginDimension(Gui, BeginDimension_Both, ScaledAscDim(Gui, V2(7, 2)));
        RadioButton(Gui, "ClocksEx", DebugProfileMenu_TopClockEx);
        RadioButton(Gui, "Clocks", DebugProfileMenu_TopClock);
        RadioButton(Gui, "Root", DebugProfileMenu_RootNode);
        
#if 0        
        GuiRadioButton();
#endif
        EndDimension(Gui);
        EndRadioGroup(Gui);
        EndRow(Gui);
        
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
        ShowText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Assets)){
    assets* Assets = Gui->Assets;
    
    if(Assets){
        char CurBlockBuf[256];
        stbsp_sprintf(CurBlockBuf, 
                      "Current asset block index: %d", 
                      Assets->CurrentBlockIndex);
        ShowText(Gui, CurBlockBuf);
        
        BeginTree(Gui, "Asset file sources");
        asset_file_source* At = Assets->FileSourceUse.Next;
        
        if(At != &Assets->FileSourceUse){
            while(At != &Assets->FileSourceUse){
                
                char FileSourceBuf[256];
                stbsp_sprintf(FileSourceBuf,
                              "Name: %s; Integration base ID: %u",
                              At->FileDescription.FullPath,
                              At->IntegrationBaseID);
                
                ShowText(Gui, FileSourceBuf);
                
                At = At->Next;
            }
        }
        else{
            ShowText(Gui, "None");
        }
        EndTree(Gui);
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Game)){
    if(Data){
        
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Console)){
    if(Data){
        
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_GUI)){
    if(Data){
        float DeltaTime = Gui->FrameInfo.DeltaTime;
        
        char FPSBuf[64];
        stbsp_sprintf(FPSBuf, "FPS %.2f, delta time(sec) %.3f", 
                      1.0f / DeltaTime, DeltaTime);
        ShowText(Gui, FPSBuf);
        
        char InterInfo[256];
        stbsp_sprintf(InterInfo, "Hot Interaction ID: %u Name: \"%s\" Priority: %u", 
                      Gui->HotInteraction.ID, 
                      Gui->HotInteraction.Name,
                      Gui->HotInteraction.Priority);
        ShowText(Gui, InterInfo);
        
        stbsp_sprintf(InterInfo, "Active Interaction ID: %u Name: \"%s\" Priority: %u", 
                      Gui->ActiveInteraction.ID,
                      Gui->ActiveInteraction.Name,
                      Gui->ActiveInteraction.Priority);
        ShowText(Gui, InterInfo);
        
        SliderFloat(Gui, &Gui->FontScale,
                    0.5f, 1.5f,
                    "FontScale");
        
        BoolButtonTrueFalse(Gui, "Show GuiTest", &Gui->ShowGuiTest);
        if(Gui->ShowGuiTest){
            GuiTest(Gui, Gui->FrameInfo.DeltaTime);
        }
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_DEBUG)){
    if(Data){
        debug_state* DEBUG = (debug_state*)Data;
        
        
        
        BoolButtonTrueFalse(Gui, "Show DEBUG overlays (F3 to toggle)",
                            &DEBUG->ShowDebugOverlay);
        BoolButtonTrueFalse(Gui, "Show DEBUG menus (F4 to toggle)", 
                            &DEBUG->ShowDebugMenus);
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}

DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Log)){
    if(Data){
        
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}


DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Platform)){
    if(Data){
        
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}


DEBUG_MENU_GUI_FUNC_CALLBACK(DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Render)){
    if(Data){
        
    }
    else{
        ShowText(Gui, "ERROR. Data was NULL");
    }
}