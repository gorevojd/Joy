#ifndef JOY_DEBUG_H
#define JOY_DEBUG_H

#include "joy_defines.h"
#include "joy_types.h"
#include "joy_math.h"
#include "joy_memory.h"
#include "joy_render.h"

#if defined(JOY_DEBUG_BUILD)

struct debug_primitive_data_line{
    v3 From;
    v3 To;
};

struct debug_primitive_data_cross{
    v3 CenterP;
    f32 Size;
};

struct debug_primitive_data_circle{
    v3 CenterP;
    f32 Radius;
    // NOTE(Dima): 0 - X, 1 - Y, 2 - Z
    u8 DirectionIndicator;
};

struct debug_primitive_data_axes{
    v3 CenterP;
    v3 Left;
    v3 Up;
    v3 Front;
    float Size;
};

struct debug_primitive_data_triangle{
    v3 P0;
    v3 P1;
    v3 P2;
};

struct debug_primitive_data_aabb{
    v3 Min;
    v3 Max;
};

struct debug_primitive_data_obb{
    v3 CenterP;
    v3 Left;
    v3 Up;
    v3 Front;
    v3 ScaleXYZ;
};

enum debug_primitive_type{
    DebugPrimitive_Line,
    DebugPrimitive_Cross,
    DebugPrimitive_Circle,
    DebugPrimitive_Axes,
    DebugPrimitive_Triangle,
    DebugPrimitive_AABB,
    DebugPrimitive_OBB,
    DebugPrimitive_String,
};

struct debug_primitive{
    v3 Color;
    f32 Duration;
    b32 DepthEnabled;
    f32 LineWidth;
    
    u32 Type;
    
    union{
        debug_primitive_data_line Line;
        debug_primitive_data_cross Cross;
        debug_primitive_data_circle Circle;
        debug_primitive_data_axes Axes;
        debug_primitive_data_triangle Triangle;
        debug_primitive_data_aabb AABB;
        debug_primitive_data_obb OBB;
    };
    
    debug_primitive* Next;
    debug_primitive* Prev;
};

struct debug_state{
    mem_region* Region;
    
    render_state* Render;
    
#define DEBUG_CIRCLE_SEGMENTS 16
    v3 CircleVerticesX[DEBUG_CIRCLE_SEGMENTS];
    v3 CircleVerticesY[DEBUG_CIRCLE_SEGMENTS];
    v3 CircleVerticesZ[DEBUG_CIRCLE_SEGMENTS];
    
    debug_primitive PrimitiveUse;
    debug_primitive PrimitiveFree;
    int TotalAllocatedPrimitives;
};

void DEBUGInit(debug_state* State,
               render_state* Render);

void DEBUGUpdate(debug_state* State, 
                 f32 DeltaTime);

extern debug_state* DEBUGGlobalState;

#else // NOTE(Dima): JOY_DEBUG_BUILD

#endif // NOTE(Dima): JOY_DEBUG_BUILD

#endif