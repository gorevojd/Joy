#include "joy_camera.h"

// NOTE(Dima): 
m44 UpdateCameraRotation(Game_Camera* camera,
                         float dPitch,
                         float dYaw,
                         float dRoll)
{
    Euler_Angles camAngles = Quat2Euler(camera->Rotation);
    
    float LockEdge = 89.0f * JOY_DEG2RAD;
    
    camAngles.Pitch += dPitch;
    camAngles.Yaw += dYaw;
    camAngles.Roll += dRoll;
    
    camAngles.Pitch = Clamp(camAngles.Pitch, -LockEdge, LockEdge);
    
    camera->Rotation = Normalize(Euler2Quat(camAngles));
    
    m44 Result = QuatToM44(camera->Rotation);
    return(Result);
}

m44 GetCameraMatrix(Game_Camera* camera){
#if 0
    m44 Result = LookAt(camera->P, GetQuatFront(camera->Rotation), V3(0.0f, 1.0f, 0.0f));
#else
    m44 Result = InverseTranslationMatrix(camera->P) * Transpose(QuatToM44(camera->Rotation));
#endif
    
    return(Result);
}