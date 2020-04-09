#include "joy_debug.cpp"

inline debug_primitive* DEBUGAddPrimitive(v3 Color,
                                          f32 Duration,
                                          b32 DepthEnabled,
                                          u32 Type)
{
    if(DLIST_FREE_IS_EMPTY(DEBUGGlobalTable->PrimitiveFree)){
        const int CountToAlloc = 128;
        debug_primitive* Pool = PushArray(&DEBUGGlobalTable->Region,
                                          debug_primitive, CountToAlloc);
        
        for(int Index = 0;
            Index < CountToAlloc;
            Index++)
        {
            debug_primitive* Prim = &Pool[Index];
            
            DLIST_INSERT_BEFORE_SENTINEL(Prim, 
                                         DEBUGGlobalTable->PrimitiveFree,
                                         Next, Prev);
        }
        
        DEBUGGlobalTable->TotalAllocatedPrimitives += CountToAlloc;
    }
    
    debug_primitive* Result = DEBUGGlobalTable->PrimitiveFree.Next;
    
    DLIST_REMOVE_ENTRY(Result, Next, Prev);
    
    DLIST_INSERT_BEFORE_SENTINEL(Result, 
                                 DEBUGGlobalTable->PrimitiveUse,
                                 Next, Prev);
    
    Result->Color = Color;
    Result->Duration = Duration;
    Result->Type = Type;
    Result->DepthEnabled = DepthEnabled;
    
    return(Result);
}

inline debug_primitive* DEBUGRemovePrimitive(debug_primitive* Primitive){
    DLIST_REMOVE_ENTRY(Primitive, Next, Prev);
    
    DLIST_INSERT_BEFORE_SENTINEL(Primitive, 
                                 DEBUGGlobalTable->PrimitiveFree,
                                 Next, Prev);
    
    return(Result);
}

void DEBUGAddLine(v3 From,
                  v3 To,
                  v3 Color,
                  f32 LineWidth,
                  f32 Duration,
                  b32 DepthEnabled)
{
    debug_primitive* Primitive = DEBUGAddPrimitive(Color, Duration, DepthEnabled, 
                                                   DebugPrimitive_Line);
    
    Primitive->Line.From = From;
    Primitive->Line.To = To;
    Primitive->Line.LineWidth = LineWidth;
}

void DEBUGAddCross(v3 CenterP,
                   v3 Color,
                   f32 Size,
                   f32 Duration,
                   b32 DepthEnabled)
{
    f32 HalfSize = 0.5f * Size;
    v3 HalfSizeX = V3(HalfSize, 0.0f, 0.0f);
    v3 HalfSizeY = V3(0.0f, HalfSize, 0.0f);
    v3 HalfSizeZ = V3(0.0f, 0.0f, HalfSize);
    
    DEBUGAddLine(CenterP - HalfSizeX, 
                 CenterP + HalfSizeX, 
                 Color, 1.0f,
                 Duration,
                 DepthEnabled);
}

void DEBUGAddSphere(v3 CenterP,
                    v3 Color,
                    f32 Radius,
                    f32 Duration,
                    b32 DepthEnabled)
{
    
}

// TODO(Dima): SIMD this
void DEBUGAddCircle(v3 CenterP,
                    v3 PlaneNormal,
                    v3 Color,
                    f32 Radius,
                    f32 Duration,
                    b32 DepthEnabled)
{
    int CountSegments = 32;
    
    f32 AnglePerSegment = JOY_TWO_PI / (f32)CountSegments;
    
    for(int SegmentIndex = 0;
        SegmentIndex < CountSegments;
        SegmentIndex++)
    {
        f32 AngleStart = (f32)SegmentIndex * AnglePerSegment;
        f32 AngleEnd = AngleStart + AnglePerSegment;
        
        v3 PStartInitPlane = V3(Cos(AngleStart), 0.0f, Sin(AngleStart)) * Radius;
        v3 PEndInitPlane = V3(Cos(AngleEnd), 0.0f, Sin(AngleEnd)) * Radius;
        
        v3 Matrix = MatrixFromRows(V3(1.0f, 0.0f, 0.0f),
                                   V3(0.0f, 1.0f, 0.0f),
                                   V3(0.0f, 0.0f, 1.0f));
        
        v3 Left = Cross(PlaneNormal, V3(0.0f, 1.0f, 0.0f));
        if(LengthSq(Left) > 0.0f){
            Left = Normalize(Left);
            
            v3 Up = Normalize(Cross(PlaneNormal, Left));
            
            Matrix = MatrixFromRows(Left, PlaneNormal, NewUp);
        }
        
        v3 ResultStartP = PStartInitPlane * Matrix + CenterP;
        v3 ResultEndP = PEndInitPlane * Matrix + CenterP;
        
        DEBUGAddLine(ResultStartP, 
                     ResultEndP,
                     Color,
                     1.0f,
                     Duration,
                     DepthEnabled);
    }
}


void DEBUGAddAxes(const transform& Tfm,
                  v3 Color,
                  f32 Size,
                  f32 Duration,
                  b32 DepthEnabled)
{
    v3 Left = NOZ(GetQuatLeft(Tfm->R));
    v3 Up = NOZ(GetQuatUp(Tfm->R));
    v3 Front = NOZ(GetQuatFront(Tfm->R));
    
    DEBUGAddLine(Tfm->T, Tfm->T + Left * Size, 
                 V3(1.0f, 0.0f, 0.0f), 1.0f,
                 Duration, DepthEnabled);
    
    
    DEBUGAddLine(Tfm->T, Tfm->T + Up * Size, 
                 V3(0.0f, 1.0f, 0.0f), 1.0f,
                 Duration, DepthEnabled);
    
    
    DEBUGAddLine(Tfm->T, Tfm->T + Front * Size, 
                 V3(0.0f, 0.0f, 1.0f),1.0f,
                 Duration, DepthEnabled);
}

void DEBUGAddTriangle(v3 Point0,
                      v3 Point1,
                      v3 Point2,
                      v3 Color,
                      f32 LineWidth,
                      f32 Duration,
                      b32 DepthEnabled)
{
    DEBUGAddLine(Poin0, Point1,
                 Color, LineWidth,
                 Duration, DepthEnabled);
    DEBUGAddLine(Poin1, Point2,
                 Color, LineWidth,
                 Duration, DepthEnabled);
    DEBUGAddLine(Poin2, Point0,
                 Color, LineWidth,
                 Duration, DepthEnabled);
}

void DEBUGAddAABB(v3 Min,
                  v3 Max,
                  v3 Color,
                  f32 LineWidth,
                  f32 Duration,
                  b32 DepthEnabled)
{
    v3 Low0, Low1, Low2, Low3;
    v3 Hig0, Hig1, Hig2, Hig3;
    
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
    
    // NOTE(Dima): Lower lines
    DEBUGAddLine(Low0, Low1, Color, LineWidth,
                 Duration, DepthEnabled);
    
    DEBUGAddLine(Low1, Low2, Color, LineWidth,
                 Duration, DepthEnabled);
    
    DEBUGAddLine(Low2, Low3, Color, LineWidth,
                 Duration, DepthEnabled);
    
    
    DEBUGAddLine(Low3, Low0, Color, LineWidth,
                 Duration, DepthEnabled);
    
    // NOTE(Dima): Upper lines
    DEBUGAddLine(Hig0, Hig1, Color, LineWidth,
                 Duration, DepthEnabled);
    
    DEBUGAddLine(Hig1, Hig2, Color, LineWidth,
                 Duration, DepthEnabled);
    
    DEBUGAddLine(Hig2, Hig3, Color, LineWidth,
                 Duration, DepthEnabled);
    
    
    DEBUGAddLine(Hig3, Hig0, Color, LineWidth,
                 Duration, DepthEnabled);
    
    // NOTE(Dima): Wall lines
    DEBUGAddLine(Low0, Hig0, Color, LineWidth,
                 Duration, DepthEnabled);
    
    DEBUGAddLine(Low1, Hig1, Color, LineWidth,
                 Duration, DepthEnabled);
    
    DEBUGAddLine(Low2, Hig2, Color, LineWidth,
                 Duration, DepthEnabled);
    
    
    DEBUGAddLine(Low3, Hig3, Color, LineWidth,
                 Duration, DepthEnabled);
}

void DEBUGUpdatePrimitives(float DeltaTime){
    debug_primitive* At = DEBUGGlobalTable->PrimitiveUse.Next;
    while(At != &DEBUGGlobalTable->PrimitiveUse){
        debug_primitive* Next = At->Next;
        
        // NOTE(Dima): Push to render
        switch(At->Type){
            case DebugPrimitive_Line:{
                
            }break;
        }
        
        // NOTE(Dima): Update lifetime
        if(At->Duration > 0.00001f){
            At->Duration -= DeltaTime
        }
        else{
            
            
            DEBUGRemovePrimitive(At);
        }
        
        At = Next;
    }
}

void DEBUGInitTable(debug_table* Table)
{
    Table->Region = {};
    
    DLIST_REFLECT_PTRS(Table->PrimitiveUse);
    DLIST_REFLECT_PTRS(Table->PrimitiveFree);
}