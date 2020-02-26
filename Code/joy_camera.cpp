#include "joy_camera.h"

// NOTE(Dima): 
void UpdateCameraRotation(game_camera* camera,
                          float dPitch,
                          float dYaw,
                          float dRoll)
{
    float LockEdge = 89.0f * JOY_DEG2RAD;
    
    camera->Angles.Pitch += dPitch * JOY_DEG2RAD;
    camera->Angles.Yaw += dYaw * JOY_DEG2RAD;
    camera->Angles.Roll += dRoll * JOY_DEG2RAD;
    
    camera->Angles.Pitch = Clamp(camera->Angles.Pitch, -LockEdge, LockEdge);
    
    v3 Front;
    Front.x = Sin(camera->Angles.Yaw) * Cos(camera->Angles.Pitch);
    Front.y = Sin(camera->Angles.Pitch);
    Front.z = Cos(camera->Angles.Yaw) * Cos(camera->Angles.Pitch);
    Front = NOZ(Front);
    
    camera->Rotation = QuatLookAt(Front, V3(0.0f, 1.0f, 0.0f));
}

// NOTE(Dima): Look at matrix
m44 GetCameraMatrix(game_camera* camera){
    m44 Result = InverseTranslationMatrix(camera->P) * Transpose(Quat2M44(camera->Rotation));
    
    return(Result);
}