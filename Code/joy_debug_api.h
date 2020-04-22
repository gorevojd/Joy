#ifndef JOY_DEBUG_API_H
#define JOY_DEBUG_API_H

#if defined(JOY_DEBUG_BUILD)


void DEBUGAddLine(v3 From,
                  v3 To,
                  v3 Color,
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

void DEBUGAddCircleX(v3 CenterP,
                     v3 Color,
                     f32 Radius = 1.0f,
                     f32 Duration = 0.0f,
                     b32 DepthEnabled = true);

void DEBUGAddCircleY(v3 CenterP,
                     v3 Color,
                     f32 Radius = 1.0f,
                     f32 Duration = 0.0f,
                     b32 DepthEnabled = true);

void DEBUGAddCircleZ(v3 CenterP,
                     v3 Color,
                     f32 Radius = 1.0f,
                     f32 Duration = 0.0f,
                     b32 DepthEnabled = true);

void DEBUGAddAxes(const m44& Tfm,
                  f32 Size = 1.0f,
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddTriangle(v3 Point0,
                      v3 Point1,
                      v3 Point2,
                      v3 Color,
                      f32 Duration = 0.0f,
                      b32 DepthEnabled = true);

void DEBUGAddAABB(v3 Min,
                  v3 Max,
                  v3 Color,
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddOBB(const m44& CenterTransform,
                 v3 ScaleXYZ,
                 v3 Color,
                 f32 Duration = 0.0f,
                 b32 DepthEnabled = true);

void DEBUGAddString(v3 P,
                    char* Text,
                    v3 Color,
                    f32 Size = 1.0f,
                    f32 Duration = 0.0f,
                    b32 DepthEnabled = true);

#else

#define DEBUGAddLine(...)
#define DEBUGAddCross(...)
#define DEBUGAddSphere(...)
#define DEBUGAddCircleX(...)
#define DEBUGAddCircleY(...)
#define DEBUGAddCircleZ(...)
#define DEBUGAddAxes(...)
#define DEBUGAddTriangle(...)
#define DEBUGAddAABB(...)
#define DEBUGAddOBB(...)
#define DEBUGAddString(...)

// NOTE(Dima): Timing stuff
#define BEGIN_TIMING(...)
#define END_TIMING(...)

#endif // NOTE(Dima): JOY_DEBUG_BUILD
#endif // NOTE(Dima): JOY_DEBUG_API_H