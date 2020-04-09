#ifndef JOY_DEBUG_H
#define JOY_DEBUG_H

enum debug_primitive_type{
    DebugPrimitive_Line,
    DebugPrimitive_Cross,
    DebugPrimitive_Sphere,
    DebugPrimitive_Circle,
    DebugPrimitive_Axes,
    DebugPrimitive_Triangle,
    DebugPrimitive_AABB,
    DebugPrimitive_OBB,
    DebugPrimitive_String,
};

struct debug_primitive_data_line{
    v3 From;
    v3 To;
    f32 LineWidth;
};

struct debug_primitive{
    v3 Color;
    f32 Duration;
    b32 DepthEnabled;
    
    u32 Type;
    
    debug_primitive* Next;
    debug_primitive* Prev;
};

struct debug_table{
    mem_region Region;
    
    debug_primitive PrimitiveUse;
    debug_primitive PrimitiveFree;
    int TotalAllocatedPrimitives;
};

void DEBUGAddLine(v3 From,
                  v3 To,
                  v3 Color,
                  f32 LineWidth = 1.0f
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddCross(v3 CenterP,
                   v3 Color,
                   f32 Size = 1.0f,
                   f32 Duration = 0.0f,
                   b32 DepthEnabled = true);

void DEBUGAddSphere(v3 CenterP,
                    v3 Color,
                    f32 Radius = 1.0f,
                    f32 Duration = 0.0f,
                    b32 DepthEnabled = true);

void DEBUGAddCircle(v3 CenterP,
                    v3 PlaneNormal,
                    v3 Color,
                    f32 Radius = 1.0f,
                    f32 Duration = 0.0f,
                    b32 DepthEnabled = true);

void DEBUGAddAxes(const transform& Tfm,
                  v3 Color,
                  f32 Size = 1.0f,
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddTriangle(v3 Point0,
                      v3 Point1,
                      v3 Point2,
                      v3 Color,
                      f32 LineWidth = 1.0f,
                      f32 Duration = 0.0f,
                      b32 DepthEnabled = true);

void DEBUGAddAABB(v3 Min,
                  v3 Max,
                  v3 Color,
                  f32 LineWidth = 1.0f,
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddOBB(const m44& CenterTransform,
                 v3 ScaleXYZ,
                 v3 Color,
                 f32 LineWidth = 1.0f,
                 f32 Duration = 0.0f,
                 b32 DepthEnabled = true);

void DEBUGAddString(v3 P,
                    char* Text,
                    v3 Color,
                    f32 Size = 1.0f,
                    f32 Duration,
                    b32 Enabled);

#endif