#if defined(JOY_DEBUG_BUILD)

#include "joy_debug_menu_gui.cpp"

void DEBUGSetMenuDataSource(u32 Type, void* Data){
    DEBUGGlobalTable->MenuDataSources[Type] = Data;
}

inline debug_primitive* DEBUGAddPrimitive(v3 Color,
                                          f32 Duration,
                                          b32 DepthEnabled,
                                          u32 Type)
{
    DLIST_ALLOCATE_FUNCTION_BODY(debug_primitive, 
                                 DEBUGGlobalTable->Region,
                                 Next, Prev,
                                 DEBUGGlobalTable->PrimitiveFree,
                                 DEBUGGlobalTable->PrimitiveUse,
                                 2048, Result);
    
    Result->Color = Color;
    Result->Duration = Duration;
    Result->DepthEnabled = DepthEnabled;
    Result->Type = Type;
    
    return(Result);
}

inline debug_primitive* DEBUGRemovePrimitive(debug_primitive* Primitive){
    DLIST_DEALLOCATE_FUNCTION_BODY(Primitive, 
                                   Next, Prev,
                                   DEBUGGlobalTable->PrimitiveFree);
    
    return(Primitive);
}

void DEBUGAddLine(v3 From,
                  v3 To,
                  v3 Color,
                  f32 Duration,
                  b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_Line);
    
    debug_primitive_data_line* Line = &Primitive->Line;
    
    Line->From = From;
    Line->To = To;
}

void DEBUGAddCross(v3 CenterP,
                   v3 Color,
                   f32 Size,
                   f32 Duration,
                   b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_Cross);
    
    debug_primitive_data_cross* Cross = &Primitive->Cross;
    
    Cross->CenterP = CenterP;
    Cross->Size = Size;
}

void DEBUGAddCircleX(v3 CenterP,
                     v3 Color,
                     f32 Radius,
                     f32 Duration,
                     b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_Circle);
    
    debug_primitive_data_circle* Circle = &Primitive->Circle;
    
    Circle->Radius = Radius;
    Circle->CenterP = CenterP;
    Circle->DirectionIndicator = 0;
}

void DEBUGAddCircleY(v3 CenterP,
                     v3 Color,
                     f32 Radius,
                     f32 Duration,
                     b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_Circle);
    
    debug_primitive_data_circle* Circle = &Primitive->Circle;
    
    Circle->Radius = Radius;
    Circle->CenterP = CenterP;
    Circle->DirectionIndicator = 1;
}

void DEBUGAddCircleZ(v3 CenterP,
                     v3 Color,
                     f32 Radius,
                     f32 Duration,
                     b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_Circle);
    
    debug_primitive_data_circle* Circle = &Primitive->Circle;
    
    Circle->Radius = Radius;
    Circle->CenterP = CenterP;
    Circle->DirectionIndicator = 2;
}

void DEBUGAddSphere(v3 CenterP,
                    v3 Color,
                    f32 Radius,
                    f32 Duration,
                    b32 DepthEnabled)
{
    f32 Min01Bound = -0.8;
    f32 Max01Bound = 0.8;
    
    f32 BoundDist = Max01Bound - Min01Bound;
    
#define DEBUG_SPHERE_AXIS_SEGMENTS 4
    f32 CircleStep = BoundDist / (f32)DEBUG_SPHERE_AXIS_SEGMENTS;
    
    for(int i = 0; i <= DEBUG_SPHERE_AXIS_SEGMENTS; i++){
        f32 t = Min01Bound + CircleStep * (f32)i;
        
        f32 TargetRadius = Sqrt(Radius * Radius * (1.0f - t * t));
        
        v3 XPos = V3(1.0f, 0.0f, 0.0f) * t;
        v3 ZPos = V3(0.0f, 0.0f, 1.0f) * t;
        
        DEBUGAddCircleX(CenterP + XPos, Color, TargetRadius, Duration, DepthEnabled);
        DEBUGAddCircleZ(CenterP + ZPos, Color, TargetRadius, Duration, DepthEnabled);
    }
}

void DEBUGAddAxes(const m44& Tfm,
                  f32 Size,
                  f32 Duration,
                  b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(V3(1.0f), Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_Axes);
    
    debug_primitive_data_axes* Axes = &Primitive->Axes;
    
    Axes->Left = NOZ(Tfm.Rows[0].xyz);
    Axes->Up = NOZ(Tfm.Rows[1].xyz);
    Axes->Front = NOZ(Tfm.Rows[2].xyz);
    Axes->CenterP = Tfm.Rows[3].xyz;
    Axes->Size = Size;
}

void DEBUGAddTriangle(v3 Point0,
                      v3 Point1,
                      v3 Point2,
                      v3 Color,
                      f32 Duration,
                      b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_Triangle);
    
    debug_primitive_data_triangle* Tri = &Primitive->Triangle;
    
    Tri->P0 = Point0;
    Tri->P1 = Point1;
    Tri->P2 = Point2;
}

void DEBUGAddAABB(v3 Min,
                  v3 Max,
                  v3 Color,
                  f32 Duration,
                  b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_AABB);
    
    debug_primitive_data_aabb* AABB = &Primitive->AABB;
    
    AABB->Min = Min;
    AABB->Max = Max;
}

void DEBUGAddOBB(const m44& CenterTransform,
                 v3 ScaleXYZ,
                 v3 Color,
                 f32 Duration,
                 b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, 
                                                   DepthEnabled, 
                                                   DebugPrimitive_OBB);
    
    debug_primitive_data_obb* OBB = &Primitive->OBB;
    
    OBB->Left = NOZ(CenterTransform.Rows[0].xyz);
    OBB->Up = NOZ(CenterTransform.Rows[1].xyz);
    OBB->Front = NOZ(CenterTransform.Rows[2].xyz);
    OBB->CenterP = CenterTransform.Rows[3].xyz;
    OBB->ScaleXYZ = ScaleXYZ;
}

INTERNAL_FUNCTION void DEBUGPushFromPointsArray(render_state* Render,
                                                v3 Center, 
                                                int Count, v3* Array, 
                                                f32 Radius, v3 Color,
                                                b32 DepthEnabled,
                                                f32 LineWidth)
{
    
    render_pushed_line_primitive Prim = BeginLinePrimitive(Render, Count, LineWidth, DepthEnabled);
    
    for(int SegmentIndex = 0;
        SegmentIndex < Count;
        SegmentIndex++)
    {
        int Ind0 = SegmentIndex;
        int Ind1 = (SegmentIndex + 1) % Count;
        
        v3 ResultP0 = Array[Ind0] * Radius;
        v3 ResultP1 = Array[Ind1] * Radius;
        
        AddLine(&Prim, Center + ResultP0, Center + ResultP1, Color);
    }
}


INTERNAL_FUNCTION void DEBUGPushBoxFromPoints(render_state* Render,
                                              v3 Low0, v3 Low1, v3 Low2, v3 Low3,
                                              v3 Hig0, v3 Hig1, v3 Hig2, v3 Hig3,
                                              v3 Color, b32 DepthEnabled, 
                                              f32 LineWidth)
{
    render_pushed_line_primitive Prim = BeginLinePrimitive(Render, 12, LineWidth, DepthEnabled);
    
    // NOTE(Dima): Lower lines
    AddLine(&Prim, Low0, Low1, Color);
    AddLine(&Prim, Low1, Low2, Color);
    AddLine(&Prim, Low2, Low3, Color);
    AddLine(&Prim, Low3, Low0, Color);
    
    // NOTE(Dima): Upper lines
    AddLine(&Prim, Hig0, Hig1, Color);
    AddLine(&Prim, Hig1, Hig2, Color);
    AddLine(&Prim, Hig2, Hig3, Color);
    AddLine(&Prim, Hig3, Hig0, Color);
    
    // NOTE(Dima): Wall lines
    AddLine(&Prim, Low0, Hig0, Color);
    AddLine(&Prim, Low1, Hig1, Color);
    AddLine(&Prim, Low2, Hig2, Color);
    AddLine(&Prim, Low3, Hig3, Color);
}


void DEBUGUpdatePrimitives(debug_state* State, float DeltaTime){
    debug_global_table* Table = DEBUGGlobalTable;
    
    debug_primitive* At = Table->PrimitiveUse.Next;
    
    // NOTE(Dima): Push to render loop
    while(At != &Table->PrimitiveUse){
        switch(At->Type){
            case DebugPrimitive_Line:{
                debug_primitive_data_line* Line = &At->Line;
                
                PushLine(State->Render, Line->From, Line->To, 
                         At->Color, 
                         At->DepthEnabled);
            }break;
            
            case DebugPrimitive_Cross:{
                debug_primitive_data_cross* Cross = &At->Cross;
                
                render_pushed_line_primitive CrossPrim = BeginLinePrimitive(State->Render,
                                                                            3, 1.0f, 
                                                                            At->DepthEnabled);
                f32 HalfSize = Cross->Size * 0.5f;
                v3 HalfSizeX = V3(HalfSize, 0.0f, 0.0f);
                v3 HalfSizeY = V3(0.0f, HalfSize, 0.0f);
                v3 HalfSizeZ = V3(0.0f, 0.0f, HalfSize);
                
                AddLine(&CrossPrim, Cross->CenterP - HalfSizeX, Cross->CenterP + HalfSizeX, At->Color);
                AddLine(&CrossPrim, Cross->CenterP - HalfSizeY, Cross->CenterP + HalfSizeY, At->Color);
                AddLine(&CrossPrim, Cross->CenterP - HalfSizeZ, Cross->CenterP + HalfSizeZ, At->Color);
            }break;
            
            case DebugPrimitive_Circle:{
                
                debug_primitive_data_circle* Circle = &At->Circle;
                
                v3* SrcArray = 0;
                if(Circle->DirectionIndicator == 0){
                    SrcArray = DEBUGGlobalTable->CircleVerticesX;
                }
                else if(Circle->DirectionIndicator == 1){
                    SrcArray = DEBUGGlobalTable->CircleVerticesY;
                }
                else if(Circle->DirectionIndicator == 2){
                    SrcArray = DEBUGGlobalTable->CircleVerticesZ;
                }
                
                DEBUGPushFromPointsArray(State->Render, 
                                         Circle->CenterP, 
                                         DEBUG_CIRCLE_SEGMENTS, 
                                         SrcArray, Circle->Radius, 
                                         At->Color, At->DepthEnabled, 
                                         1.0f);
            }break;
            
            case DebugPrimitive_Axes:{
                debug_primitive_data_axes* Axes = &At->Axes;
                
                render_pushed_line_primitive Prim = BeginLinePrimitive(State->Render,
                                                                       3, 1.0f, At->DepthEnabled);
                
                f32 InternalSizing = 0.2f;
                
                v3 T = Axes->CenterP;
                f32 Size = Axes->Size;
                
                AddLine(&Prim, T, T + Axes->Left * Size * InternalSizing,
                        V3(1.0f, 0.0f, 0.0f));
                AddLine(&Prim, T, T + Axes->Up * Size * InternalSizing,
                        V3(0.0f, 1.0f, 0.0f));
                AddLine(&Prim, T, T + Axes->Front * Size * InternalSizing,
                        V3(0.0f, 0.0f, 1.0f));
            }break;
            
            case DebugPrimitive_Triangle:{
                debug_primitive_data_triangle* Tri = &At->Triangle;
                
                render_pushed_line_primitive Prim = BeginLinePrimitive(State->Render,
                                                                       3, 1.0f, At->DepthEnabled);
                
                AddLine(&Prim, Tri->P0, Tri->P1, At->Color);
                AddLine(&Prim, Tri->P1, Tri->P2, At->Color);
                AddLine(&Prim, Tri->P2, Tri->P0, At->Color);
            }break;
            
            case DebugPrimitive_AABB:{
                debug_primitive_data_aabb* AABB = &At->AABB;
                
                v3 Low0, Low1, Low2, Low3;
                v3 Hig0, Hig1, Hig2, Hig3;
                
                v3 Min = AABB->Min;
                v3 Max = AABB->Max;
                
                v3 Dim = GetRectDim(RcMinMax(Min, Max));
                
                Low0 = Low1 = Low2 = Low3 = Min;
                Low1.x = Max.x;
                Low2.z = Max.z;
                Low3.x = Max.x;
                Low3.z = Max.z;
                
                Hig0 = Low0;
                Hig1 = Low1;
                Hig2 = Low2;
                Hig3 = Low3;
                
                Hig0.y = Max.y;
                Hig1.y = Max.y;
                Hig2.y = Max.y;
                Hig3.y = Max.y;
                
                DEBUGPushBoxFromPoints(State->Render, 
                                       Low0, Low1, Low2, Low3,
                                       Hig0, Hig1, Hig2, Hig3,
                                       At->Color,
                                       At->DepthEnabled, 1.0f);
            }break;
            
            case DebugPrimitive_OBB:{
                debug_primitive_data_obb* OBB = &At->OBB;
                
                v3 Front = OBB->Front;
                v3 Up = OBB->Up;
                v3 Left = OBB->Left;
                v3 T = OBB->CenterP;
                v3 ScaleXYZ = OBB->ScaleXYZ;
                
                v3 Top0, Top1, Top2, Top3;
                v3 Bot0, Bot1, Bot2, Bot3;
                
                Top0 = T - Front * ScaleXYZ.z + Up * ScaleXYZ.y - Left * ScaleXYZ.x;
                Top1 = T - Front * ScaleXYZ.z + Up * ScaleXYZ.y + Left * ScaleXYZ.x;
                Top2 = T + Front * ScaleXYZ.z + Up * ScaleXYZ.y + Left * ScaleXYZ.x;
                Top3 = T + Front * ScaleXYZ.z + Up * ScaleXYZ.y - Left * ScaleXYZ.x;
                
                Bot0 = T - Front * ScaleXYZ.z - Up * ScaleXYZ.y - Left * ScaleXYZ.x;
                Bot1 = T - Front * ScaleXYZ.z - Up * ScaleXYZ.y + Left * ScaleXYZ.x;
                Bot2 = T + Front * ScaleXYZ.z - Up * ScaleXYZ.y + Left * ScaleXYZ.x;
                Bot3 = T + Front * ScaleXYZ.z - Up * ScaleXYZ.y - Left * ScaleXYZ.x;
                
                DEBUGPushBoxFromPoints(State->Render, 
                                       Bot0, Bot1, Bot2, Bot3,
                                       Top0, Top1, Top2, Top3,
                                       At->Color,
                                       At->DepthEnabled, 1.0f);
            }break;
        }
        
        At = At->Next;
    }
    
    At = Table->PrimitiveUse.Next;
    while(At != &Table->PrimitiveUse){
        debug_primitive* Next = At->Next; 
        
        // NOTE(Dima): Update lifetime
        if(At->Duration > 0.00001f){
            At->Duration -= DeltaTime;
        }
        else{
            DEBUGRemovePrimitive(At);
        }
        
        At = Next;
    }
}

INTERNAL_FUNCTION void DEBUGInitCircleVertices()
{
    debug_global_table* Table = DEBUGGlobalTable;
    
    int CountSegments = DEBUG_CIRCLE_SEGMENTS;
    
    f32 AnglePerSegment = JOY_TWO_PI / (f32)CountSegments;
    
    for(int SegmentIndex = 0;
        SegmentIndex < CountSegments;
        SegmentIndex++)
    {
        f32 AngleStart = (f32)SegmentIndex * AnglePerSegment;
        
        Table->CircleVerticesX[SegmentIndex] = V3(0.0f, Cos(AngleStart), Sin(AngleStart));
        Table->CircleVerticesY[SegmentIndex] = V3(Cos(AngleStart), 0.0f, Sin(AngleStart));
        Table->CircleVerticesZ[SegmentIndex] = V3(Cos(AngleStart), Sin(AngleStart), 0.0f);
    }
}

INTERNAL_FUNCTION void DEBUGInitMenu(debug_state* State, u32 Type, char* Name, 
                                     debug_menu_gui_func_callback* Func)
{
    debug_menu* Menu = &State->Menus[Type];
    
    CopyStringsSafe(Menu->Name, sizeof(Menu->Name), Name);
    Menu->Type = Type;
    Menu->Callback = Func;
}

INTERNAL_FUNCTION void DEBUGUpdateMenus(debug_state* State)
{
    FUNCTION_TIMING();
    
    if(KeyWentDown(State->Input, Key_F3)){
        State->ShowDebugOverlay = !State->ShowDebugOverlay;
    }
    
    if(KeyWentDown(State->Input, Key_F4)){
        State->ShowDebugMenus = !State->ShowDebugMenus;
    }
    
    if(State->ShowDebugOverlay){
        gui_state* Gui = State->Gui;
        
        debug_window* Window = &State->TestWindow;
        
        v2 Dim4But = ScaledAscDim(Gui, V2(8, 2));
        v2 DimNeeded = V2(Dim4But.x, Dim4But.y * (float)DebugMenu_Count * 1.1);
        
        if(State->ShowDebugMenus){
            
            BeginLayout(Gui, "TestWindow", 
                        GuiLayout_Window | GuiLayout_Resize, 
                        &Window->P, 
                        &DimNeeded);
            BeginColumn(Gui);
            // NOTE(Dima): Showing menu
            BeginDimension(Gui, BeginDimension_Both, Dim4But);
            
            BeginRadioGroup(Gui, "MenuRadioGroup", 
                            &State->ToShowMenuType, 
                            State->ToShowMenuType);
            
            for(int MenuIndex = 0;
                MenuIndex < DebugMenu_Count;
                MenuIndex++)
            {
                debug_menu* Menu = &State->Menus[MenuIndex];
                
                Menu->Data = DEBUGGlobalTable->MenuDataSources[MenuIndex];
                
                RadioButton(Gui, Menu->Name, Menu->Type);
            }
            
            EndRadioGroup(Gui);
            
            EndDimension(Gui);
            EndColumn(Gui);
            EndLayout(Gui);
        }
        
        
        Window = &State->MainWindow;
        BeginLayout(Gui, "DEBUG_MainWindow", 
                    GuiLayout_Window | GuiLayout_Resize, 
                    &Window->P, 
                    &Window->Dim);
        
        
        // NOTE(Dima): Calling gui callback for DEBUG menu
        debug_menu* ToShowMenu = &State->Menus[State->ToShowMenuType];
        PushGuiFont(Gui, GetFirst(Gui->Assets, GameAsset_LilitaOne));
        BeginDimension(Gui, BeginDimension_Height, V2(0.0f, GetScaledAscender(Gui) * 2.0f));
        ShowHeader(Gui, ToShowMenu->Name);
        EndDimension(Gui);
        PopGuiFont(Gui);
        if(ToShowMenu->Callback){
            ToShowMenu->Callback(Gui, ToShowMenu->Data);
        }
        else{
            ShowText(Gui, "GUI callback was NULL");
        }
        
        EndLayout(Gui);
    }
}

void DEBUGParseNameFromUnique(char* To, int ToSize,
                              char* From)
{
    if(To && From){
        char* At = From;
        
        int Counter = 0;
        while(*At){
            if((Counter >= ToSize - 1) || 
               (*At == '|'))
            {
                break;
            }
            
            To[Counter++] = *At;
            
            At++;
        }
        
        To[Counter] = 0;
    }
}

INTERNAL_FUNCTION inline
debug_profiled_tree_node* AllocateTreeNode(debug_state* State, 
                                           debug_thread_frame* Frame)
{
    DLIST_ALLOCATE_FUNCTION_BODY(debug_profiled_tree_node, State->Region,
                                 NextAlloc, PrevAlloc, 
                                 State->TreeNodeFree,
                                 Frame->RootTreeNodeUse,
                                 1024,
                                 Result);
    
    return(Result);
}

INTERNAL_FUNCTION inline 
void CreateSentinel4Element(debug_state* State, 
                            debug_thread_frame* Frame,
                            debug_profiled_tree_node* Element)
{
    Element->ChildSentinel = AllocateTreeNode(State, Frame);
    
    DLIST_REFLECT_POINTER_PTRS(Element->ChildSentinel, Next, Prev);
    Element->ChildSentinel->Parent = Element;
    Element->ChildSentinel->UniqueName = State->SentinelElementsName;
    Element->ChildSentinel->NameID = State->SentinelElementsNameHash;
    
    Element->ChildSentinel->ChildSentinel = 0;
}

INTERNAL_FUNCTION void ClearThreadsTable(debug_thread** Table,
                                         int TableSize)
{
    for(int Index = 0;
        Index < TableSize;
        Index++)
    {
        Table[Index] = 0;
    }
}

INTERNAL_FUNCTION void ClearStatsTable(debug_timing_stat** Table, 
                                       int TableSize)
{
    for(int Index = 0;
        Index < TableSize;
        Index++)
    {
        Table[Index] = 0;
    }
}

INTERNAL_FUNCTION inline void InitThreadFrame(debug_state* State, 
                                              debug_thread_frame* Frame)
{
    Frame->RootTreeNodeUse.UniqueName = State->RootNodesName;
    Frame->RootTreeNodeUse.NameID = State->RootNodesNameHash;
    
    
    DLIST_REFLECT_PTRS(Frame->RootTreeNodeUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Frame->RootTreeNodeUse, Next, Prev);
    DLIST_REFLECT_PTRS(Frame->StatUse, Next, Prev);
    
    ClearStatsTable(Frame->StatTable, DEBUG_STATS_TABLE_SIZE);
    ClearStatsTable(Frame->ToSortStats, DEBUG_STATS_TO_SORT_SIZE);
    Frame->ToSortStatsCount = 0;
    
    Frame->FrameUpdateNode = 0;
    
    CreateSentinel4Element(State, Frame, &Frame->RootTreeNodeUse);
    
    Frame->CurNode = &Frame->RootTreeNodeUse;
}

INTERNAL_FUNCTION inline void ClearThreadFrame(debug_state* State,
                                               debug_thread_frame* Frame)
{
    // NOTE(Dima): Freing profiled nodes
    DLIST_REMOVE_ENTIRE_LIST(&Frame->RootTreeNodeUse, 
                             &State->TreeNodeFree, 
                             NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Frame->RootTreeNodeUse, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Freing stats 
    DLIST_REMOVE_ENTIRE_LIST(&Frame->StatUse,
                             &State->StatFree,
                             Next, Prev);
    DLIST_REFLECT_PTRS(Frame->StatUse, Next, Prev);
    
    
    ClearStatsTable(Frame->StatTable, DEBUG_STATS_TABLE_SIZE);
    
    Frame->ToSortStatsCount = 0;
    Frame->FrameUpdateNode = 0;
    
    // NOTE(Dima): Recreating childs 
    CreateSentinel4Element(State, Frame, &Frame->RootTreeNodeUse);
    //DLIST_REFLECT_POINTER_PTRS(Frame->RootTreeNodeUse.ChildSentinel, Next, Prev);
    
    Frame->CurNode = &Frame->RootTreeNodeUse;
}

INTERNAL_FUNCTION inline void ClearThreadsIndexFrame(debug_state* State, int FrameIndex){
    // NOTE(Dima): Clearing corresponding frames in threads
    debug_thread* ThreadAt = State->ThreadSentinel.NextAlloc;
    while(ThreadAt != &State->ThreadSentinel){
        debug_thread_frame* Frame = &ThreadAt->Frames[FrameIndex];
        
        ClearThreadFrame(State, Frame);
        
        ThreadAt = ThreadAt->NextAlloc;
    }
    
}

INTERNAL_FUNCTION debug_profiled_tree_node*
RequestTreeNode(debug_state* State, 
                debug_thread_frame* Frame, 
                debug_profiled_tree_node* Current,
                char* UniqueName,
                b32* Allocated)
{
    u32 HashID = StringHashFNV(UniqueName);
    
    debug_profiled_tree_node* Found = 0;
    debug_profiled_tree_node* At = Current->ChildSentinel->Next;
    while(At != Current->ChildSentinel){
        if(StringHashFNV(At->UniqueName) == HashID){
            Found = At;
            
            break;
        }
        
        At = At->Next;
    }
    
    b32 IsAllocated = false;
    if(!Found){
        IsAllocated = true;
        
        Found = AllocateTreeNode(State, Frame);
        Found->NameID = HashID;
        Found->UniqueName = UniqueName;
        Found->Parent = Current;
        Found->TimingSnapshot = {};
        CreateSentinel4Element(State, Frame, Found);
        
        DLIST_INSERT_BEFORE(Found, Current->ChildSentinel, Next, Prev);
    }
    
    if(Allocated){
        *Allocated = IsAllocated;
    }
    
    return(Found);
}

INTERNAL_FUNCTION debug_thread* 
CreateOrFindThreadForID(debug_state* State, u16 ThreadID)
{
    u32 Key = ThreadID % DEBUG_THREADS_TABLE_SIZE;
    
    debug_thread* Found = 0;
    debug_thread* At = State->ThreadHashTable[Key];
    
    if(At){
        while(At){
            if(At->ThreadID == ThreadID){
                Found = At;
                break;
            }
            
            At = At->NextInHash;
        }
    }
    
    if(!Found){
        // NOTE(Dima): Allocate the thread
        Found = PushStruct(State->Region, debug_thread);
        
        // NOTE(Dima): Init the debug thread
        DLIST_INSERT_BEFORE_SENTINEL(Found, State->ThreadSentinel, 
                                     NextAlloc, PrevAlloc);
        Found->ThreadID = ThreadID;
        Found->WatchNodeUniqueName = 0;
        
        Found->NextInHash = State->ThreadHashTable[Key];
        State->ThreadHashTable[Key] = Found;
        
        Found->Frames = PushArray(State->Region, debug_thread_frame, 
                                  DEBUG_PROFILED_FRAMES_COUNT);
        for(int FrameIndex = 0;
            FrameIndex < DEBUG_PROFILED_FRAMES_COUNT;
            FrameIndex++)
        {
            debug_thread_frame* Frame = &Found->Frames[FrameIndex];
            
            InitThreadFrame(State, Frame);
        }
        
        // NOTE(Dima): Increasing profiled debug thread count
        State->ProfiledThreadsCount++;
    }
    
    return(Found);
}

INTERNAL_FUNCTION debug_timing_stat* AllocateTimingStat(debug_state* State,
                                                        debug_thread_frame* Frame)
{
    DLIST_ALLOCATE_FUNCTION_BODY(debug_timing_stat, State->Region,
                                 Next, Prev, 
                                 State->StatFree,
                                 Frame->StatUse,
                                 512,
                                 Result);
    
    return(Result);
}

INTERNAL_FUNCTION debug_timing_stat* 
CreateOrFindStatForUniqueName(debug_state* State, 
                              debug_thread_frame* Frame,
                              char* UniqueName)
{
    u32 NameID = StringHashFNV(UniqueName);
    
    u32 Key = NameID % DEBUG_STATS_TABLE_SIZE;
    
    debug_timing_stat* Found = 0;
    debug_timing_stat* StatAt = Frame->StatTable[Key];
    
    if(StatAt){
        while(StatAt){
            if(StatAt->NameID == NameID){
                Found = StatAt;
                
                break;
            }
            
            StatAt = StatAt->NextInHash;
        }
    }
    
    if(!Found){
        debug_timing_stat* New = AllocateTimingStat(State, Frame);
        
        New->UniqueName = UniqueName;
        New->NameID = NameID;
        New->Stat = {};
        // NOTE(Dima): Inserting to hash table
        New->NextInHash = Frame->StatTable[Key];
        Frame->StatTable[Key] = New;
        
        Found = New;
    }
    
    return(Found);
}

INTERNAL_FUNCTION void 
FillAndSortStats(debug_state* State, 
                 debug_thread_frame* Frame, 
                 b32 IncludingChildren)
{
    // NOTE(Dima): Filling to sort table
    debug_timing_stat* Stat = Frame->StatUse.Next;
    
    Frame->ToSortStatsCount = 0;
    
    for(int StatIndex = 0;
        StatIndex < DEBUG_STATS_TO_SORT_SIZE;
        StatIndex++)
    {
        if(Stat == &Frame->StatUse){
            break;
        }
        
        ++Frame->ToSortStatsCount;
        
        Frame->ToSortStats[StatIndex] = Stat;
        
        Stat = Stat->Next;
    }
    
    // NOTE(Dima): Sorting ToSort table by selection sort
    for(int i = 0; i < Frame->ToSortStatsCount - 1; i++){
        u64 MaxValue = GetClocksFromStat(Frame->ToSortStats[i], IncludingChildren);
        int MaxIndex = i;
        
        for(int j = i + 1; j < Frame->ToSortStatsCount; j++){
            u64 CurClocks = GetClocksFromStat(Frame->ToSortStats[j], IncludingChildren);
            if(CurClocks > MaxValue){
                MaxValue = CurClocks;
                MaxIndex = j;
            }
        }
        
        if(MaxIndex != i){
            debug_timing_stat* Temp = Frame->ToSortStats[i];
            Frame->ToSortStats[i] = Frame->ToSortStats[MaxIndex];
            Frame->ToSortStats[MaxIndex] = Temp;
        }
    }
}

INTERNAL_FUNCTION inline int IncrementFrameIndex(int Value){
    int Result = (Value + 1) % DEBUG_PROFILED_FRAMES_COUNT;
    
    return(Result);
}

INTERNAL_FUNCTION inline void
IncrementFrameIndices(debug_state* State){
    if(State->ViewFrameIndex != State->CollationFrameIndex){
        State->ViewFrameIndex = IncrementFrameIndex(State->ViewFrameIndex);
    }
    
    State->NewestFrameIndex = State->CollationFrameIndex;
    State->CollationFrameIndex = IncrementFrameIndex(State->CollationFrameIndex);
    
    if(State->CollationFrameIndex == State->OldestFrameIndex){
        State->OldestShouldBeIncremented = true;
    }
    
    if(State->OldestShouldBeIncremented){
        State->OldestFrameIndex = IncrementFrameIndex(State->OldestFrameIndex);
    }
}

INTERNAL_FUNCTION inline void
ProcessRecordsIndicesInc(debug_state* State){
    b32 ShouldIncrement = true;
    if(State->RecordingChangeRequested){
        State->RecordingChangeRequested = false;
        
        if(State->IsRecording){
            State->Filter = DEBUG_DEFAULT_FILTER_VALUE;
            ShouldIncrement = false;
        }
        else{
            State->Filter = DebugRecord_FrameBarrier;
        }
    }
    
    if(State->IsRecording && ShouldIncrement){
        IncrementFrameIndices(State);
    }
}

INTERNAL_FUNCTION void
FindFrameUpdateNode(debug_thread_frame* Frame){
    debug_profiled_tree_node* FrameUpdateNode = 0;
    
    debug_profiled_tree_node* At = Frame->RootTreeNodeUse.NextAlloc;
    
    while(At != &Frame->RootTreeNodeUse){
        char NameBuf[256];
        DEBUGParseNameFromUnique(NameBuf, 256, At->UniqueName);
        
        if(StringsAreEqual(NameBuf, FRAME_UPDATE_NODE_NAME))
        {
            FrameUpdateNode = At;
            break;
        }
        
        At = At->NextAlloc;
    }
    
    Frame->FrameUpdateNode = FrameUpdateNode;
}

INTERNAL_FUNCTION void DEBUGProcessRecords(debug_state* State){
    FUNCTION_TIMING();
    
    int RecordCount = DEBUGGlobalTable->RecordAndTableIndex & DEBUG_RECORD_INDEX_MASK;
    int TableIndex = (DEBUGGlobalTable->RecordAndTableIndex & DEBUG_TABLE_INDEX_MASK) >> 
        DEBUG_TABLE_INDEX_BITSHIFT;
    
    debug_record* RecordArray = DEBUGGlobalTable->RecordTables[TableIndex];
    
    u32 NewRecordAndTableIndex = 0;
    NewRecordAndTableIndex |= (!TableIndex) << DEBUG_TABLE_INDEX_BITSHIFT;
    
    DEBUGGlobalTable->RecordAndTableIndex.store(NewRecordAndTableIndex);
    
    for(int RecordIndex = 0; 
        RecordIndex <  RecordCount;
        RecordIndex++)
    {
        debug_record* Record = &RecordArray[RecordIndex];
        
        if(Record->Type & State->Filter){
            switch(Record->Type){
                case DebugRecord_BeginTiming:{
                    debug_thread* Thread = CreateOrFindThreadForID(State, Record->ThreadID);
                    
                    debug_thread_frame* Frame = GetThreadFrameByIndex(Thread, State->CollationFrameIndex);
                    
                    b32 NodeWasAllocated = false;
                    debug_profiled_tree_node* NewNode = RequestTreeNode(State, Frame, 
                                                                        Frame->CurNode,
                                                                        Record->UniqueName,
                                                                        &NodeWasAllocated);
                    
                    NewNode->TimingSnapshot.StartClock = Record->TimeStampCounter;
                    if(NodeWasAllocated){
                        NewNode->TimingSnapshot.StartClockFirstEntry = NewNode->TimingSnapshot.StartClock;
                    }
                    
                    Frame->CurNode = NewNode;
                }break;
                
                case DebugRecord_EndTiming:{
                    debug_thread* Thread = CreateOrFindThreadForID(State, Record->ThreadID);
                    
                    debug_thread_frame* Frame = GetThreadFrameByIndex(Thread, State->CollationFrameIndex);
                    debug_profiled_tree_node* CurNode = Frame->CurNode;
                    debug_timing_snapshot* Snapshot = &CurNode->TimingSnapshot;
                    
                    u64 ClocksElapsedThisFrame = (Record->TimeStampCounter - Snapshot->StartClock);
                    int PendingHitCount = 1;
                    
                    Snapshot->EndClock = Record->TimeStampCounter;
                    Snapshot->ClocksElapsed += ClocksElapsedThisFrame;
                    Snapshot->HitCount += PendingHitCount;
                    
                    // NOTE(Dima): Adding time to total parent's children elapsed 
                    debug_profiled_tree_node* ParentNode = CurNode->Parent;
                    if(ParentNode->Parent != 0){
                        debug_timing_stat* ParentStat = CreateOrFindStatForUniqueName(State, Frame,
                                                                                      ParentNode->UniqueName);
                        ParentStat->Stat.ClocksElapsedInChildren += ClocksElapsedThisFrame;
                    }
                    
                    // NOTE(Dima): Initializing statistic
                    debug_timing_stat* Stat = CreateOrFindStatForUniqueName(State, Frame,
                                                                            CurNode->UniqueName);
                    
                    Stat->Stat.ClocksElapsed += ClocksElapsedThisFrame;
                    Stat->Stat.HitCount += PendingHitCount;
                    
                    Frame->CurNode = CurNode->Parent;
                }break;
                
                case DebugRecord_FrameBarrier:{
                    debug_thread* MainThread = State->MainThread;
                    Assert(MainThread->ThreadID == Record->ThreadID);
                    
                    debug_thread_frame* OldFrame = GetThreadFrameByIndex(MainThread, 
                                                                         State->CollationFrameIndex);
                    debug_common_frame* OldFrameCommon = GetFrameByIndex(State, 
                                                                         State->CollationFrameIndex);
                    
                    // NOTE(Dima): Finding frame update node
                    FindFrameUpdateNode(OldFrame);
                    
                    // NOTE(Dima): Set frame time
                    OldFrameCommon->FrameTime = Record->Value.Float;
                    
                    Assert(OldFrame->CurNode == &OldFrame->RootTreeNodeUse);
                    
                    // NOTE(Dima): Incrementing frame indices when we needed
                    ProcessRecordsIndicesInc(State);
                    
                    // NOTE(Dima): Clearing frame
                    ClearThreadsIndexFrame(State, State->CollationFrameIndex);
                }break;
            }
        }
    }
}

void DEBUGInitGlobalTable(mem_region* Region){
    // NOTE(Dima): Initialize record arrays
    int RecordArrayCount = 1000000;
    DEBUGGlobalTable->TableMaxRecordCount = RecordArrayCount;
    DEBUGGlobalTable->RecordAndTableIndex = 0;
    DEBUGGlobalTable->RecordTables[0] = PushArray(Region, debug_record, RecordArrayCount);
    DEBUGGlobalTable->RecordTables[1] = PushArray(Region, debug_record, RecordArrayCount);
    
    // NOTE(Dima): Init memory region
    DEBUGGlobalTable->Region = Region;
    
    // NOTE(Dima): Set pointers on primitive sentinels
    DLIST_REFLECT_PTRS(DEBUGGlobalTable->PrimitiveUse, Next, Prev);
    DLIST_REFLECT_PTRS(DEBUGGlobalTable->PrimitiveFree, Next, Prev);
    
    // NOTE(Dima): Initialize DEBUG circle geometry
    DEBUGInitCircleVertices();
    
    // NOTE(Dima): Set all menu data sources to zero
    for(int MenuSourceIndex = 0;
        MenuSourceIndex < DebugMenu_Count;
        MenuSourceIndex++)
    {
        DEBUGGlobalTable->MenuDataSources[MenuSourceIndex] = 0;
    }
}

INTERNAL_FUNCTION void DEBUGInit(debug_state* State,
                                 render_state* Render,
                                 gui_state* Gui,
                                 input_state* Input)
{
    State->Render = Render;
    State->Gui = Gui;
    State->Input = Input;
    
    // NOTE(Dima): Init menus
    State->ShowDebugOverlay = false;
    State->ShowDebugMenus = true;
    State->MainWindow.P = V2(850, 50);
    State->MainWindow.Dim = V2(500, 700);
    CopyStringsSafe(State->MainWindow.Name, 
                    sizeof(State->MainWindow.Name), 
                    "DEBUGMainWindow");
    
    State->TestWindow.P = V2(100, 50);
    State->TestWindow.Dim = V2(300, 300);
    CopyStringsSafe(State->TestWindow.Name, 
                    sizeof(State->TestWindow.Name), 
                    "TestWindow");
    
    
    DEBUGInitMenu(State, DebugMenu_Profile, "Profile",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Profile));
    DEBUGInitMenu(State, DebugMenu_DEBUG, "DEBUG",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_DEBUG));
    DEBUGInitMenu(State, DebugMenu_Animation, "Animation",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Animation));
    DEBUGInitMenu(State, DebugMenu_Input, "Input",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Input));
    DEBUGInitMenu(State, DebugMenu_Assets, "Assets",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Assets));
    DEBUGInitMenu(State, DebugMenu_Game, "Game",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Game));
    DEBUGInitMenu(State, DebugMenu_Console, "Console",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Console));
    DEBUGInitMenu(State, DebugMenu_GUI, "GUI",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_GUI));
    DEBUGInitMenu(State, DebugMenu_Log, "Log",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Log));
    DEBUGInitMenu(State, DebugMenu_Platform, "Platform",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Platform));
    DEBUGInitMenu(State, DebugMenu_Render, "Render",
                  DEBUG_MENU_GUI_FUNC_NAME(DebugMenu_Render));
    
    State->ToShowMenuType = DebugMenu_Profile;
    
    // NOTE(Dima): Init profiler stuff
    State->CollationFrameIndex = 0;
    State->NewestFrameIndex = 0;
    State->OldestFrameIndex = 0;
    State->OldestShouldBeIncremented = false;
    State->IsRecording = DEBUG_DEFAULT_RECORDING;
    State->RecordingChangeRequested = false;
    State->Filter = DEBUG_DEFAULT_FILTER_VALUE;
    State->ToShowProfileMenuType = DebugProfileMenu_TopClockEx;
    
    CopyStrings(State->RootNodesName, "RootNode");
    CopyStrings(State->SentinelElementsName, "Sentinel");
    State->RootNodesNameHash = StringHashFNV(State->RootNodesName);
    State->SentinelElementsNameHash = StringHashFNV(State->SentinelElementsName);
    
    DLIST_REFLECT_PTRS(State->TreeNodeFree, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(State->TreeNodeFree, Next, Prev);
    DLIST_REFLECT_PTRS(State->StatFree, Next, Prev);
    
    State->ProfiledThreadsCount = 0;
    DLIST_REFLECT_PTRS(State->ThreadSentinel, NextAlloc, PrevAlloc);
    ClearThreadsTable(State->ThreadHashTable, DEBUG_THREADS_TABLE_SIZE);
    State->MainThread = CreateOrFindThreadForID(State, GetThreadID());
    State->WatchThread = State->MainThread;
}

PLATFORM_CALLBACK(DEBUGDummyThreadsWorkCallback){
    FUNCTION_TIMING();
    
    for(int i = 0; i < 100000; i++){
        u32 Res = 1000.0f / 234.0f * Sqrt(i) * (i + 1);
    }
}

INTERNAL_FUNCTION void DEBUGDummyThreadsWork(){
    int WorksCount = 100;
    
    platform_job_queue* JobQueue = &Platform.HighPriorityQueue;
    
    for(int i = 0; i < WorksCount; i++){
        Platform.AddEntry(JobQueue, DEBUGDummyThreadsWorkCallback, 0);
    }
    
    Platform.WaitForCompletion(JobQueue);
}

INTERNAL_FUNCTION void DEBUGUpdate(debug_state* State){
    FUNCTION_TIMING();
    
    DEBUGSetMenuDataSource(DebugMenu_Profile, State);
    DEBUGSetMenuDataSource(DebugMenu_DEBUG, State);
    DEBUGSetMenuDataSource(DebugMenu_Console, State);
    DEBUGSetMenuDataSource(DebugMenu_Log, State);
    
    DEBUGUpdatePrimitives(State, State->Input->DeltaTime);
    DEBUGUpdateMenus(State);
    
    //DEBUGDummyThreadsWork();
    
    DEBUGProcessRecords(State);
}

#endif