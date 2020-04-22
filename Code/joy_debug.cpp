#include "joy_debug.h"

#if defined(JOY_DEBUG_BUILD)

inline debug_primitive* DEBUGAddPrimitive(v3 Color,
                                          f32 Duration,
                                          b32 DepthEnabled,
                                          u32 Type)
{
    DLIST_ALLOCATE_FUNCTION_BODY(debug_primitive, 
                                 DEBUGGlobalState->Region,
                                 Next, Prev,
                                 DEBUGGlobalState->PrimitiveFree,
                                 DEBUGGlobalState->PrimitiveUse,
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
                                   DEBUGGlobalState->PrimitiveFree);
    
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
    debug_primitive* At = State->PrimitiveUse.Next;
    
    // NOTE(Dima): Push to render loop
    while(At != &State->PrimitiveUse){
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
                    SrcArray = DEBUGGlobalState->CircleVerticesX;
                }
                else if(Circle->DirectionIndicator == 1){
                    SrcArray = DEBUGGlobalState->CircleVerticesY;
                }
                else if(Circle->DirectionIndicator == 2){
                    SrcArray = DEBUGGlobalState->CircleVerticesZ;
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
    
    At = State->PrimitiveUse.Next;
    while(At != &State->PrimitiveUse){
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

INTERNAL_FUNCTION void DEBUGInitCircleVertices(debug_state* State)
{
    
    int CountSegments = DEBUG_CIRCLE_SEGMENTS;
    
    f32 AnglePerSegment = JOY_TWO_PI / (f32)CountSegments;
    
    for(int SegmentIndex = 0;
        SegmentIndex < CountSegments;
        SegmentIndex++)
    {
        f32 AngleStart = (f32)SegmentIndex * AnglePerSegment;
        
        State->CircleVerticesX[SegmentIndex] = V3(0.0f, Cos(AngleStart), Sin(AngleStart));
        State->CircleVerticesY[SegmentIndex] = V3(Cos(AngleStart), 0.0f, Sin(AngleStart));
        State->CircleVerticesZ[SegmentIndex] = V3(Cos(AngleStart), Sin(AngleStart), 0.0f);
    }
}

void DEBUGInit(debug_state* State,
               render_state* Render)
{
    State->Render = Render;
    
    DLIST_REFLECT_PTRS(State->PrimitiveUse, Next, Prev);
    DLIST_REFLECT_PTRS(State->PrimitiveFree, Next, Prev);
    
    DEBUGInitCircleVertices(State);
}

void DEBUGUpdate(debug_state* State, f32 DeltaTime){
    DEBUGUpdatePrimitives(State, DeltaTime);
}

#endif